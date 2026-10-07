#include "Plugin/PluginManager.h"

#include <QCoreApplication>
#include <QDir>
#include <QJsonObject>
#include <QLocale>
#include <QLoggingCategory>
#include <QPluginLoader>
#include <algorithm>
#include <ranges>
#include <utility>

#include "Component/ComponentFactory.h"
#include "Core_info.h"
#include "Plugin/IPlugin.h"

namespace AppForge
{

namespace
{

Q_LOGGING_CATEGORY(lcPlugins, "appforge.plugins")

// Reads the object appforge_add_plugin passes to Q_PLUGIN_METADATA; nullopt when its id or name is missing.
std::optional<PluginInfo> readInfo(const QJsonObject& metaData, const QString& filePath)
{
    PluginInfo info;
    info.id = metaData.value(u"id").toString();
    info.name = metaData.value(u"name").toString();
    if(info.id.isEmpty() || info.name.isEmpty())
    {
        return std::nullopt;
    }
    info.description = metaData.value(u"description").toString();
    info.version = QVersionNumber::fromString(metaData.value(u"version").toString());
    info.coreVersion = QVersionNumber::fromString(metaData.value(u"coreVersion").toString());
    info.filePath = filePath;
    return info;
}

// __DATE__ " " __TIME__, e.g. "Oct  6 2026 21:14:03", in the local time of the build machine.
QDateTime fromCompilerDate(std::string_view dateTime)
{
    const QString text = QString::fromLatin1(dateTime.data(), static_cast<qsizetype>(dateTime.size())).simplified();
    return QLocale::c().toDateTime(text, QStringLiteral("MMM d yyyy HH:mm:ss"));
}

// A plugin runs on the Core it was built against, or on a newer one with the same major version.
bool isCompatibleCore(const QVersionNumber& pluginCoreVersion)
{
    const QVersionNumber coreVersion = QVersionNumber::fromString(CoreInfo::version).normalized();
    return pluginCoreVersion.majorVersion() == coreVersion.majorVersion() &&
           pluginCoreVersion.normalized() <= coreVersion;
}

// Reads the metadata of filePath without loading it; nullopt, with a warning, when the plugin cannot be used.
std::optional<PluginInfo> readPlugin(const QPluginLoader& loader, const QString& filePath)
{
    const QJsonObject metaData = loader.metaData();
    if(metaData.isEmpty())
    {
        qCWarning(lcPlugins).noquote() << "Skipping" << filePath << "-" << loader.errorString();
        return std::nullopt;
    }
    const QString iid = metaData.value(u"IID").toString();
    if(iid != QLatin1StringView(APPFORGE_PLUGIN_IID))
    {
        qCWarning(lcPlugins).noquote() << "Skipping" << filePath << "- built against another plugin interface:" << iid;
        return std::nullopt;
    }
    std::optional<PluginInfo> info = readInfo(metaData.value(u"MetaData").toObject(), filePath);
    if(!info)
    {
        qCWarning(lcPlugins).noquote() << "Skipping" << filePath << "- no id or name in its metadata";
        return std::nullopt;
    }
    if(!isCompatibleCore(info->coreVersion))
    {
        qCWarning(lcPlugins).noquote() << "Skipping" << filePath << "- built against Core"
                                       << info->coreVersion.toString() << "instead of" << CoreInfo::version;
        return std::nullopt;
    }
    return info;
}

} // namespace

PluginManager::PluginManager() = default;

// Destroying the loaders does not unload the plugins.
PluginManager::~PluginManager() = default;

QString PluginManager::defaultDirectory()
{
    // The directory appforge_add_plugin writes the plugins to (Core/cmake/AppForgePlugin.cmake).
    return QCoreApplication::applicationDirPath() + QStringLiteral("/plugins");
}

QList<PluginInfo> PluginManager::scan(const QString& directory)
{
    QList<PluginInfo> found;
    // The extension appforge_add_plugin gives to the plugins (Core/cmake/AppForgePlugin.cmake).
    const QFileInfoList files = QDir(directory).entryInfoList({QStringLiteral("*.afplugin")}, QDir::Files, QDir::Name);
    for(const QFileInfo& file : files)
    {
        const QString filePath = file.canonicalFilePath();
        if(std::ranges::any_of(m_entries | std::views::values,
                               [&](const Entry& entry) { return entry.info.filePath == filePath; }))
        {
            continue;
        }

        auto loader = std::make_unique<QPluginLoader>(filePath);
        std::optional<PluginInfo> info = readPlugin(*loader, filePath);
        if(!info)
        {
            continue;
        }
        if(const auto known = m_entries.find(info->id); known != m_entries.end())
        {
            qCWarning(lcPlugins).noquote()
                << "Skipping" << filePath << "-" << info->id << "is already provided by" << known->second.info.filePath;
            continue;
        }

        found.append(*info);
        const QString pluginId = info->id;
        m_entries.emplace(pluginId, Entry{.info = std::move(*info), .loader = std::move(loader)});
    }
    return found;
}

QList<PluginInfo> PluginManager::plugins() const
{
    QList<PluginInfo> plugins;
    plugins.reserve(static_cast<qsizetype>(m_entries.size()));
    for(const Entry& entry : m_entries | std::views::values)
    {
        plugins.append(entry.info);
    }
    return plugins;
}

std::optional<PluginInfo> PluginManager::plugin(const QString& pluginId) const
{
    const auto entry = m_entries.find(pluginId);
    if(entry == m_entries.end())
    {
        return std::nullopt;
    }
    return entry->second.info;
}

bool PluginManager::load(const QString& pluginId)
{
    const auto entry = m_entries.find(pluginId);
    if(entry == m_entries.end())
    {
        m_errorString = QStringLiteral("Unknown plugin %1").arg(pluginId);
        return false;
    }
    // instance() loads the library and creates its root object, generated by appforge_add_plugin.
    QPluginLoader& loader = *entry->second.loader;
    IPlugin* root = qobject_cast<IPlugin*>(loader.instance());
    if(root == nullptr)
    {
        m_errorString = loader.errorString();
        return false;
    }
    entry->second.info.buildDate = fromCompilerDate(root->buildDate());
    ComponentFactory::instance().add(pluginId, root->plugin());
    m_errorString.clear();
    return true;
}

bool PluginManager::isLoaded(const QString& pluginId) const
{
    const auto entry = m_entries.find(pluginId);
    return entry != m_entries.end() && entry->second.loader->isLoaded();
}

QString PluginManager::errorString() const
{
    return m_errorString;
}

} // namespace AppForge
