#include "ComponentDescriptor.h"

#include "JsonHelper.h"

#include <nlohmann/json.hpp>

#include <QJsonArray>
#include <QRegularExpression>

#include <cstring>

using namespace appforge::core;

namespace JsonKeys {
	const std::string PLUGIN_ID = "pluginId";
	const std::string COMPONENT_ID = "componentId";
	const std::string FULL_ID = "fullId";
	const std::string DISPLAY_NAME = "displayName";
	const std::string DESCRIPTION = "description";
	const std::string VERSION = "version";
	const std::string CATEGORY = "category";
	const std::string TAGS = "tags";
}

ComponentDescriptor::ComponentDescriptor(QString pluginId,
    QString componentId,
    QString displayName,
    Version version)
    : _pluginId(std::move(pluginId))
    , _componentId(std::move(componentId))
    , _displayName(std::move(displayName))
    , _version(version)
{
}

QString ComponentDescriptor::id() const
{
    return _componentId;
}

void ComponentDescriptor::setId(const QString& componentId)
{
    _componentId = componentId;
}

QString ComponentDescriptor::displayName() const
{
    return _displayName;
}

void ComponentDescriptor::setDisplayName(const QString& displayName)
{
    _displayName = displayName;
}

QString ComponentDescriptor::description() const
{
    return _description;
}

void ComponentDescriptor::setDescription(const QString& description)
{
    _description = description;
}

Version ComponentDescriptor::version() const
{
    return _version;
}

void ComponentDescriptor::setVersion(const Version& version)
{
    _version = version;
}

QString ComponentDescriptor::category() const
{
    return _category;
}

void ComponentDescriptor::setCategory(const QString& category)
{
    _category = category;
}

QStringList ComponentDescriptor::tags() const
{
    return _tags;
}

void ComponentDescriptor::setTags(const QStringList& tags)
{
    _tags = tags;
}

bool ComponentDescriptor::isValid() const
{
    return errors().isEmpty();
}

QMap<QString, QString> ComponentDescriptor::errors() const
{
    QMap<QString, QString> errors;
    if (_pluginId.isEmpty())
    {
		errors.insert("pluginId", "Plugin id is empty");
    }
    if (!isValidPluginId(_pluginId))
    {
        errors.insert("pluginIdInvalid", "Plugin id is invalid : " + _pluginId);
    }
    if (_componentId.isEmpty())
    {
        errors.insert("componentId", "Component id is empty");
    }
    if (!isValidPluginId(_pluginId))
    {
        errors.insert("componentIdInvalid", "Component id is invalid : " + _pluginId);
    }
    return errors;
}

bool ComponentDescriptor::sameIdentity(const ComponentDescriptor& other) const
{
    return _pluginId == other._pluginId
        && _componentId == other._componentId;
}

bool ComponentDescriptor::operator==(const ComponentDescriptor& other) const
{
    return sameIdentity(other);
}

bool ComponentDescriptor::operator!=(const ComponentDescriptor& other) const
{
    return !(*this == other);
}

void ComponentDescriptor::fromJson(const QJsonObject& obj)
{
	NJson j = json_helper::toJson(obj);
    _pluginId = QString::fromStdString(j.value("pluginId", ""));
    _componentId = QString::fromStdString(j.value("componentId", ""));
    _displayName = QString::fromStdString(j.value("displayName", ""));
    _description = QString::fromStdString(j.value("description", ""));
    _category = QString::fromStdString(j.value("category", ""));

    if (j.contains("version"))
        _version.fromJson(j["version"]);

    _tags.clear();
    if (j.contains("tags")) {
        for (const auto& t : j["tags"])
            _tags.append(QString::fromStdString(t.get<std::string>()));
    }
}

bool ComponentDescriptor::isValidPluginId(const QString& pluginId)
{
    static const QRegularExpression regex(
        R"(^[a-z][a-z0-9_]*(\.[a-z][a-z0-9_]*)+$)"
    );

    return regex.match(pluginId).hasMatch();
}

bool ComponentDescriptor::isValidComponentId(const QString& componentId)
{
    static const QRegularExpression regex(
        R"(^[a-z][a-z0-9_]*$)"
    );

    return regex.match(componentId).hasMatch();
}

QJsonObject ComponentDescriptor::toJson() const
{
    NJson j{
            {JsonKeys::PLUGIN_ID, _pluginId.toStdString()},
            {JsonKeys::COMPONENT_ID, _componentId.toStdString()},
            {JsonKeys::DISPLAY_NAME, _displayName.toStdString()},
            {JsonKeys::DESCRIPTION, _description.toStdString()},
            {JsonKeys::VERSION, _version.toJson()},
            {JsonKeys::CATEGORY, _category.toStdString()},
            {JsonKeys::TAGS, NJson::array()}
    };
	return json_helper::fromJson(j);

}