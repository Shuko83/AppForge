#include "PluginViewer.h"

#include <QDir>
#include <QFormLayout>
#include <QLabel>
#include <QLocale>
#include <QPushButton>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <optional>

#include "Plugin/PluginManager.h"

namespace
{

// A row of form whose value can be selected and wraps.
QLabel* addField(QFormLayout& form, const QString& name)
{
    auto* value = new QLabel;
    value->setTextInteractionFlags(Qt::TextSelectableByMouse);
    value->setWordWrap(true);
    form.addRow(name, value);
    return value;
}

QString buildDateText(const AppForge::PluginInfo& info, bool loaded)
{
    if(!loaded)
    {
        return PluginViewer::tr("Known once loaded");
    }
    if(!info.buildDate.isValid())
    {
        return PluginViewer::tr("Unknown");
    }
    return QLocale().toString(info.buildDate, QLocale::ShortFormat);
}

} // namespace

PluginViewer::PluginViewer(const AppForge::PluginManager& plugins, QWidget* parent)
    : QWidget(parent), m_plugins(plugins)
{
    auto* placeholder = new QLabel(tr("Select a plugin"));
    placeholder->setAlignment(Qt::AlignCenter);
    m_placeholder = placeholder;

    m_page = new QWidget;
    auto* form = new QFormLayout;
    m_name = addField(*form, tr("Name"));
    m_id = addField(*form, tr("Id"));
    m_version = addField(*form, tr("Version"));
    m_description = addField(*form, tr("Description"));
    m_coreVersion = addField(*form, tr("Core version"));
    m_buildDate = addField(*form, tr("Build date"));
    m_state = addField(*form, tr("State"));
    m_file = addField(*form, tr("File"));
    m_loadButton = new QPushButton(tr("Load"));
    connect(m_loadButton, &QPushButton::clicked, this, [this] { emit loadRequested(m_pluginId); });

    auto* pageLayout = new QVBoxLayout(m_page);
    pageLayout->addLayout(form);
    pageLayout->addWidget(m_loadButton, 0, Qt::AlignLeft);
    pageLayout->addStretch();

    m_stack = new QStackedWidget;
    m_stack->addWidget(m_placeholder);
    m_stack->addWidget(m_page);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins({});
    layout->addWidget(m_stack);
}

void PluginViewer::setPluginId(const QString& pluginId)
{
    m_pluginId = pluginId;
    refresh();
}

void PluginViewer::refresh()
{
    const std::optional<AppForge::PluginInfo> info = m_plugins.plugin(m_pluginId);
    if(!info)
    {
        m_stack->setCurrentWidget(m_placeholder);
        return;
    }
    const bool loaded = m_plugins.isLoaded(info->id);
    m_name->setText(info->name);
    m_id->setText(info->id);
    m_version->setText(info->version.toString());
    m_description->setText(info->description);
    m_coreVersion->setText(info->coreVersion.toString());
    m_buildDate->setText(buildDateText(*info, loaded));
    m_state->setText(loaded ? tr("Loaded") : tr("Not loaded"));
    m_file->setText(QDir::toNativeSeparators(info->filePath));
    m_loadButton->setEnabled(!loaded);
    m_stack->setCurrentWidget(m_page);
}
