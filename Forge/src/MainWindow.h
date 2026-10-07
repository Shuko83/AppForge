#pragma once

#include <QMainWindow>
#include <QString>

#include "Plugin/PluginManager.h"

class ComponentModel;
class ComponentViewer;
class PluginModel;
class PluginViewer;
class QTreeView;

// Lists the plugins of <application directory>/plugins and the components of the loaded ones, each with a viewer of
// the one selected; a plugin is loaded from its viewer, or opened from anywhere on the disk.
class MainWindow : public QMainWindow
{
    Q_OBJECT

  public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

  private:
    // Loads pluginId and shows its components; tells why when it cannot be loaded.
    void load(const QString& pluginId);
    // Asks for a plugin file, then selects and loads it.
    void openPlugin();

    AppForge::PluginManager m_plugins;
    QString m_openDirectory; // Where openPlugin() starts browsing: the directory of the last plugin opened
    PluginModel* m_pluginModel = nullptr;
    QTreeView* m_pluginView = nullptr;
    PluginViewer* m_pluginViewer = nullptr;
    ComponentModel* m_componentModel = nullptr;
    ComponentViewer* m_componentViewer = nullptr;
};
