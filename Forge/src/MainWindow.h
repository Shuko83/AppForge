// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QHash>
#include <QJsonObject>
#include <QList>
#include <QMainWindow>
#include <QString>
#include <QStringList>

#include "AppForge/Application/ForgeApplication.h"

class ComponentModel;
class ComponentViewer;
class EditionScene;
class PluginModel;
class PluginViewer;
class PropertyEditor;
class QCommandLineParser;
class QGraphicsView;
class QHBoxLayout;
class QTreeView;

// Lists the plugins of <application directory>/plugins, and of the directories given on its command line, and the
// components of the loaded ones, each with a viewer of the one selected; a plugin is loaded from its viewer, or opened
// from anywhere on the disk, and the components are searched for by a part of their name, of their description or of
// their category (Ctrl+F). The composites it knows are listed with them: those of the project, and those made or opened
// since it started. Between them, the edition zone, where a component dropped from its list is instantiated, above the
// editor of the properties of the one selected there; a composite opened there shows its own scene in place of it,
// and the path above it leads back. Its File menu saves what is in the edition zone to a project file, with the
// composite files changed, and opens it again; its Run menu starts the application assembled there, and stops it.
class MainWindow : public QMainWindow
{
    Q_OBJECT

  public:
    // commandLine, when there is one, is the command line of Forge, processed by a parser it was given to
    // ForgeApplication::addCommandLineOptions(): the plugins of its directories are listed with those of plugins/,
    // and its plugin root is the one of the project files.
    explicit MainWindow(const QCommandLineParser* commandLine = nullptr, QWidget* parent = nullptr);
    ~MainWindow() override;

    // Replaces the application with the one of the project file filePath, whose plugins it loads, and tells what
    // could not be restored. Tells why, changes nothing and returns false when the file cannot be read.
    bool openProjectFile(const QString& filePath);
    // Writes the application to the project file filePath, from then on its file, once the composite files changed are
    // written; tells why and returns false when one cannot be written.
    bool saveProjectFile(const QString& filePath);

  protected:
    // Not closed when the changes to save are neither saved nor given up.
    void closeEvent(QCloseEvent* event) override;

  private:
    // Loads pluginId and shows its components; tells why when it cannot be loaded.
    void load(const QString& pluginId);
    // Unloads pluginId and no longer shows its components. Those of the application that are its own are removed
    // with it, once that is confirmed: nothing is done otherwise. Tells why when it cannot be unloaded.
    void unload(const QString& pluginId);
    // Asks for a plugin file, then selects and loads it.
    void openPlugin();

    // The scene shown: the one of the last composite opened, or the one of the application.
    [[nodiscard]] EditionScene* currentScene() const;
    // Has scene, the one of the application or of a composite, tell what it refuses or does, open the composites it
    // shows and show the properties of what is selected in it.
    void connectScene(EditionScene& scene);
    // Shows the scene of composite in place of the one shown, whose component it is.
    void openComposite(AppForge::Composite& composite);
    // Closes the scenes of the composites opened, but the first depth ones, keeping how their composite files are laid
    // out, then shows the last one left: none, the scene of the application, for 0.
    void closeComposites(qsizetype depth);
    // Shows currentScene(), and the path from the application to it.
    void showScene();

    // Keeps a record of the composite files the application read since it last did, reading how each is laid out from
    // its file, and forgets those the application forgot. They are among those it knows from then on.
    void syncComposites();
    // What the composite file filePath, one the application read, holds now: what ForgeApplication::saveComposite()
    // writes, laid out as its scene shows it, or as it was last.
    [[nodiscard]] QJsonObject compositeContent(const QString& filePath) const;
    // The composite files changed since they were read or written: those whose reading met no problem only.
    [[nodiscard]] QStringList changedComposites() const;
    // Writes the composite files changed; tells why and returns false when one cannot be written.
    bool saveComposites();
    // Asks for a composite file, which the application reads, and lists it with the composites.
    void openCompositeFile();

    // The directory of the file of the application, which the files of its composites are relative to; empty when it
    // has none: they are written whole then.
    [[nodiscard]] QString projectDirectory() const;
    // filePath is the file of the application, none when empty, which holds it as it is now.
    void setProjectFile(const QString& filePath);
    // Asks, when the application changed since its file holds it, whether to save it. Returns false when what was
    // about to replace it is given up: the answer was to cancel, or it could not be saved.
    bool saveChanges();
    // Empties the application, once its changes are saved or given up.
    void newProject();
    // Asks for a project file and opens it, once the changes of the application are saved or given up.
    void openProject();
    // Saves the application to its file, asked for when it has none; false when it is not saved.
    bool saveProject();
    // Asks for the file to save the application to; false when it is not saved.
    bool saveProjectAs();
    // Starts the application; tells why when it cannot be.
    void start();

    // A composite file the application read.
    struct CompositeFile
    {
        QJsonObject laidOut; // As its scene last showed it, or as its file holds it
        QJsonObject saved;   // What compositeContent() was once it was last read or written
    };

    AppForge::ForgeApplication m_application; // What is assembled in the edition zone, with the plugins it is made from
    QString m_openDirectory;       // Where openPlugin() starts browsing: the directory of the last plugin opened
    QString m_projectFile;         // Empty until the application is saved to a file, or opened from one
    QJsonObject m_savedProject;    // What m_scene saved when m_projectFile was set
    QStringList m_knownComposites; // The composite files listed with the components, since it started
    QHash<QString, CompositeFile> m_composites; // Those the application read, by file
    PluginModel* m_pluginModel = nullptr;
    QTreeView* m_pluginView = nullptr;
    EditionScene* m_scene = nullptr;        // Of m_application
    QList<EditionScene*> m_compositeScenes; // Of the composites opened, each one of the scene before it
    QGraphicsView* m_editionView = nullptr; // Shows currentScene()
    QHBoxLayout* m_path = nullptr;          // From the application to the composite shown, above m_editionView
    PluginViewer* m_pluginViewer = nullptr;
    ComponentModel* m_componentModel = nullptr;
    ComponentViewer* m_componentViewer = nullptr;
    PropertyEditor* m_propertyEditor = nullptr; // Of the component selected in m_editionView
};
