#include "PluginManager.h"

#include <QPluginLoader>
#include <QJsonObject>
#include <QDir>

#include "afCore/IPlugin.h"
#include "afCore/IComponent.h"
#include "ComponentDescriptor.h"

using namespace appforge::core;

PluginManager::PluginManager(const QString& id)
	: _id(id)
{
}

PluginManager::~PluginManager()
{
	for (auto plugin : _plugins)
	{
		delete plugin;
	}
}

void PluginManager::loadPlugins(const QString& directory)
{
	for (auto& fileInfo : QDir(directory).entryInfoList(QDir::Files))
	{
		loadPlugin(fileInfo.absoluteFilePath());
	}
}

bool PluginManager::loadPlugin(const QString& path)
{
	QPluginLoader loader(path);
	auto metadata = loader.metaData().value("MetaData").toObject();
	QString version = metadata.value("version").toString();
	if (version.isEmpty())
	{
		version = metadata.value("Version").toString();
		return false;
	}
	QObject* instance = loader.instance();
	if (!instance)
	{
		return false;
	}
	IPlugin* plugin = qobject_cast<IPlugin*>(instance);
	if (!plugin)
	{
		loader.unload();
		return false;
	}
	_plugins.append(plugin);
	return true;
}

void PluginManager::registerComponent(IComponent& component)
{
	QString fullId = _id + component.getDescriptor().id();
	_components.insert(fullId, &component);
}
