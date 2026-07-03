#pragma once

#include "afCore/IPluginLoader.h"
#include "afCore/IPluginBank.h"

#include <QList>
#include <QString>

namespace appforge::core {

	class ComponentRegistry;
	class IComponent;

	class PluginManager : public IPluginLoader
	{
	public:
		explicit PluginManager(const QString& id);
		~PluginManager();

		// Hérité via IPluginLoader
		void loadPlugins(const QString& directory) override;
		bool loadPlugin(const QString& path) override;
		
		void registerComponent(IComponent& component);

	private:
		QList<IPlugin*> _plugins;
		QMap<QString, IComponent*> _components;
		QString _id;
	};
}