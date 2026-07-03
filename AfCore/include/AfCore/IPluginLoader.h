#pragma once

#include <QString>

namespace appforge::core {

	class ComponentRegistry;
	class IComponent;

	class IPluginLoader
	{
	public:
		virtual ~IPluginLoader() = default;

		virtual void loadPlugins(const QString& directory) = 0;
		virtual bool loadPlugin(const QString& path) = 0;
	};
}