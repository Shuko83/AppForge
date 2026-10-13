#pragma once

#include <QString>
#include <QWidget>

class QLabel;
class QPushButton;
class QStackedWidget;

namespace AppForge
{
class PluginManager;
}

// Shows a plugin of a PluginManager, with a button to load it and another to unload it.
class PluginViewer : public QWidget
{
    Q_OBJECT

  public:
    explicit PluginViewer(const AppForge::PluginManager& plugins, QWidget* parent = nullptr);

    // Shows pluginId; nothing when it is empty or unknown.
    void setPluginId(const QString& pluginId);
    // Reads the plugin shown again, e.g. once loaded.
    void refresh();
    // Whether a loaded plugin can be unloaded now: it can until told otherwise. Its Unload button tells why not, in
    // its tooltip.
    void setUnloadAllowed(bool allowed);

  signals:
    // The Load button was clicked: PluginViewer does not load the plugin itself.
    void loadRequested(const QString& pluginId);
    // The Unload button was clicked: PluginViewer does not unload the plugin itself.
    void unloadRequested(const QString& pluginId);

  private:
    const AppForge::PluginManager& m_plugins;
    QString m_pluginId;
    bool m_unloadAllowed = true;

    QStackedWidget* m_stack = nullptr;
    QWidget* m_placeholder = nullptr; // Shown when there is no plugin
    QWidget* m_page = nullptr;        // Shown otherwise
    QLabel* m_name = nullptr;
    QLabel* m_id = nullptr;
    QLabel* m_version = nullptr;
    QLabel* m_description = nullptr;
    QLabel* m_coreVersion = nullptr;
    QLabel* m_buildDate = nullptr;
    QLabel* m_state = nullptr;
    QLabel* m_file = nullptr;
    QPushButton* m_loadButton = nullptr;
    QPushButton* m_unloadButton = nullptr;
};
