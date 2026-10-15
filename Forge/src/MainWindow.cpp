// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "MainWindow.h"

#include <QAction>
#include <QCloseEvent>
#include <QComboBox>
#include <QCoreApplication>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QGraphicsView>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QSaveFile>
#include <QShortcut>
#include <QSortFilterProxyModel>
#include <QSplitter>
#include <QStatusBar>
#include <QTreeView>
#include <QVBoxLayout>
#include <array>
#include <cstddef>
#include <optional>

#include "AppForge/Component/ComponentFactory.h"
#include "ComponentModel.h"
#include "ComponentViewer.h"
#include "EditionScene.h"
#include "PluginModel.h"
#include "PluginViewer.h"
#include "PropertyEditor.h"

namespace
{

// How long the status bar tells what the edition zone refused or did, in milliseconds.
constexpr int MessageDuration = 8000;

// What the components can be searched by: a column of ComponentModel, and the role of its data the text is looked
// for in. The description of a component is its tool tip, in every column.
struct SearchField
{
    const char* name; // Translated in the context of MainWindow
    int column;
    int role;
};

constexpr std::array SearchFields{
    SearchField{QT_TRANSLATE_NOOP("MainWindow", "Name"), ComponentModel::Name, Qt::DisplayRole},
    SearchField{QT_TRANSLATE_NOOP("MainWindow", "Description"), ComponentModel::Name, Qt::ToolTipRole},
    SearchField{QT_TRANSLATE_NOOP("MainWindow", "Category"), ComponentModel::Category, Qt::DisplayRole},
};

// Where the project files are saved and looked for unless told otherwise: scene/ next to Forge.
QString defaultProjectDirectory()
{
    return QCoreApplication::applicationDirPath() + QStringLiteral("/scene");
}

// A list of the rows of model, one selected at a time.
QTreeView* createView(QAbstractItemModel& model)
{
    auto* view = new QTreeView;
    view->setModel(&model);
    view->setRootIsDecorated(false);
    view->setUniformRowHeights(true);
    view->setAllColumnsShowFocus(true);
    view->setSelectionMode(QAbstractItemView::SingleSelection);
    view->header()->setStretchLastSection(false);
    view->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    view->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    return view;
}

// view above viewer, under title.
QWidget* createPanel(const QString& title, QWidget* view, QWidget* viewer)
{
    auto* splitter = new QSplitter(Qt::Vertical);
    splitter->addWidget(view);
    splitter->addWidget(viewer);
    splitter->setStretchFactor(1, 1);

    auto* panel = new QGroupBox(title);
    auto* layout = new QVBoxLayout(panel);
    layout->addWidget(splitter);
    return panel;
}

} // namespace

MainWindow::MainWindow(const QString& pluginRoot, QWidget* parent)
    : QMainWindow(parent), m_pluginModel(new PluginModel(m_application.plugins(), this)),
      m_pluginViewer(new PluginViewer(m_application.plugins())),
      m_componentModel(new ComponentModel(AppForge::ComponentFactory::instance(), this)),
      m_componentViewer(new ComponentViewer(AppForge::ComponentFactory::instance())),
      m_propertyEditor(new PropertyEditor(m_application, AppForge::ComponentFactory::instance()))
{
    QMenu* fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(tr("&New"), QKeySequence::New, this, &MainWindow::newProject);
    fileMenu->addAction(tr("&Open..."), QKeySequence::Open, this, &MainWindow::openProject);
    fileMenu->addAction(tr("&Save"), QKeySequence::Save, this, &MainWindow::saveProject);
    fileMenu->addAction(tr("Save &As..."), QKeySequence::SaveAs, this, &MainWindow::saveProjectAs);

    // Start stays enabled while it runs: it starts what was dropped meanwhile.
    QMenu* runMenu = menuBar()->addMenu(tr("&Run"));
    runMenu->addAction(tr("&Start"), QKeySequence(Qt::Key_F5), this, &MainWindow::start);
    QAction* stopAction = runMenu->addAction(tr("S&top"), QKeySequence(Qt::SHIFT | Qt::Key_F5), &m_application,
                                             &AppForge::ForgeApplication::stop);
    stopAction->setEnabled(false);
    connect(&m_application, &AppForge::ForgeApplication::runningChanged, this,
            [this, stopAction](bool running)
            {
                stopAction->setEnabled(running);
                m_pluginViewer->setUnloadAllowed(!running); // What runs keeps its plugin
            });

    if(!pluginRoot.isEmpty())
    {
        m_application.setPluginRoot(pluginRoot);
    }
    m_openDirectory = AppForge::PluginManager::defaultDirectory();
    m_pluginModel->refresh();
    m_componentModel->refresh();

    m_pluginView = createView(*m_pluginModel);
    connect(m_pluginView->selectionModel(), &QItemSelectionModel::currentRowChanged, this,
            [this](const QModelIndex& current)
            { m_pluginViewer->setPluginId(current.data(PluginModel::IdRole).toString()); });
    connect(m_pluginViewer, &PluginViewer::loadRequested, this, &MainWindow::load);
    connect(m_pluginViewer, &PluginViewer::unloadRequested, this, &MainWindow::unload);

    auto* openButton = new QPushButton(tr("Open plugin..."));
    openButton->setToolTip(tr("Load a plugin from anywhere on the disk"));
    connect(openButton, &QPushButton::clicked, this, &MainWindow::openPlugin);
    auto* pluginList = new QWidget;
    auto* pluginListLayout = new QVBoxLayout(pluginList);
    pluginListLayout->setContentsMargins({});
    pluginListLayout->addWidget(openButton, 0, Qt::AlignLeft);
    pluginListLayout->addWidget(m_pluginView);

    // The components whose name, description or category, as chosen next to the search bar, holds what is typed there,
    // whatever its case. The proxy hands what is dragged over to the model, which makes the data.
    auto* componentFilter = new QSortFilterProxyModel(this);
    componentFilter->setSourceModel(m_componentModel);
    componentFilter->setFilterCaseSensitivity(Qt::CaseInsensitive);
    auto* search = new QLineEdit;
    search->setPlaceholderText(tr("Search components"));
    search->setClearButtonEnabled(true);
    connect(search, &QLineEdit::textChanged, componentFilter, &QSortFilterProxyModel::setFilterFixedString);
    auto* searchField = new QComboBox;
    for(const SearchField& field : SearchFields)
    {
        searchField->addItem(tr(field.name));
    }
    searchField->setToolTip(tr("Where the text is looked for"));
    const auto searchBy = [componentFilter](int index)
    {
        const SearchField& field = SearchFields.at(static_cast<std::size_t>(index));
        componentFilter->setFilterKeyColumn(field.column);
        componentFilter->setFilterRole(field.role);
    };
    searchBy(searchField->currentIndex());
    connect(searchField, &QComboBox::currentIndexChanged, componentFilter, searchBy);
    auto* findShortcut = new QShortcut(QKeySequence::Find, this);
    connect(findShortcut, &QShortcut::activated, search,
            [search]
            {
                search->setFocus(Qt::ShortcutFocusReason);
                search->selectAll(); // Even when it already had the focus: what is typed replaces the last search
            });

    QTreeView* componentView = createView(*componentFilter);
    componentView->setDragEnabled(true);
    componentView->setDragDropMode(QAbstractItemView::DragOnly);
    // Also when the search hides the selected component: the view selects one of those left, none when none is.
    connect(componentView->selectionModel(), &QItemSelectionModel::currentRowChanged, this,
            [this](const QModelIndex& current)
            { m_componentViewer->setComponentId(current.data(ComponentModel::IdRole).toString()); });

    auto* searchBar = new QHBoxLayout;
    searchBar->addWidget(search, 1);
    searchBar->addWidget(searchField);
    auto* componentList = new QWidget;
    auto* componentListLayout = new QVBoxLayout(componentList);
    componentListLayout->setContentsMargins({});
    componentListLayout->addLayout(searchBar);
    componentListLayout->addWidget(componentView);

    m_scene = new EditionScene(m_application, AppForge::ComponentFactory::instance(), this);
    m_editionView = new QGraphicsView(m_scene);
    m_editionView->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    m_editionView->setRenderHint(QPainter::Antialiasing);
    m_editionView->setDragMode(QGraphicsView::RubberBandDrag);
    m_editionView->setAcceptDrops(true);
    connect(m_scene, &QGraphicsScene::selectionChanged, this,
            [this] { m_propertyEditor->setComponent(m_scene->selectedComponent()); });
    // In the status bar rather than in a box, which would open in the middle of the click that ends an edition.
    const auto tell = [this](const QString& message) { statusBar()->showMessage(message, MessageDuration); };
    connect(m_scene, &EditionScene::refused, this, tell);
    connect(m_scene, &EditionScene::told, this, tell);
    // Nor in the middle of a run, which goes on without the file.
    connect(&m_application, &AppForge::ForgeApplication::settingsFileFailed, this, tell);

    // The properties of the component selected in the edition zone, under it; the zone takes the room.
    auto* editionSplitter = new QSplitter(Qt::Vertical);
    editionSplitter->addWidget(m_editionView);
    editionSplitter->addWidget(m_propertyEditor);
    editionSplitter->setStretchFactor(0, 1);
    auto* edition = new QGroupBox(tr("Application"));
    auto* editionLayout = new QVBoxLayout(edition);
    editionLayout->addWidget(editionSplitter);

    // The edition zone between the plugins and the components, next to the list a component is dragged from.
    auto* splitter = new QSplitter(Qt::Horizontal);
    splitter->addWidget(createPanel(tr("Plugins"), pluginList, m_pluginViewer));
    splitter->addWidget(edition);
    splitter->addWidget(createPanel(tr("Components"), componentList, m_componentViewer));
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({320, 760, 360});
    setCentralWidget(splitter);

    statusBar()->addWidget(new QLabel(
        tr("Plugins of %1").arg(QDir::toNativeSeparators(AppForge::PluginManager::defaultDirectory()))));
    m_pluginView->setCurrentIndex(m_pluginModel->index(0, 0));
    setProjectFile({});
    resize(1440, 800);
}

MainWindow::~MainWindow()
{
    // Its items, and the property editor, refer to the components of m_application, destroyed after this body, before
    // the children of the window. The scene is no longer listened to: deleting a selected item changes its selection.
    // Neither is the application, stopped when it is destroyed.
    m_application.disconnect(this);
    m_scene->disconnect(this);
    m_propertyEditor->setComponent(nullptr);
    delete m_scene;
}

bool MainWindow::openProjectFile(const QString& filePath)
{
    QString error;
    const std::optional<QJsonObject> project = AppForge::ForgeApplication::read(filePath, error);
    if(!project)
    {
        // Told before anything is changed.
        QMessageBox::warning(this, tr("Open project"),
                             tr("%1 cannot be opened:\n%2").arg(QDir::toNativeSeparators(filePath), error));
        return false;
    }
    const QStringList problems = m_scene->load(*project);
    // The plugins of the project, loaded with it, and their components.
    m_pluginModel->refresh();
    m_pluginViewer->refresh();
    m_componentModel->refresh();
    // No longer its file when part of it is missing: saving would write over what could not be restored.
    setProjectFile(problems.isEmpty() ? filePath : QString());
    if(!problems.isEmpty())
    {
        QMessageBox::warning(this, tr("Open project"),
                             tr("%1 could not be entirely opened:\n%2")
                                 .arg(QDir::toNativeSeparators(filePath), problems.join(u'\n')));
    }
    return true;
}

bool MainWindow::saveProjectFile(const QString& filePath)
{
    // The title of a zone, or the name of a component, is no longer edited: what was typed is taken before it is saved.
    m_scene->setFocusItem(nullptr);
    QSaveFile file(filePath);
    if(!file.open(QIODevice::WriteOnly) || file.write(QJsonDocument(m_scene->save()).toJson()) < 0 || !file.commit())
    {
        QMessageBox::warning(this, tr("Save project"),
                             tr("%1 cannot be saved:\n%2").arg(QDir::toNativeSeparators(filePath), file.errorString()));
        return false;
    }
    setProjectFile(filePath);
    return true;
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    event->setAccepted(saveChanges());
}

void MainWindow::load(const QString& pluginId)
{
    if(!m_application.plugins().load(pluginId))
    {
        QMessageBox::warning(this, tr("Load plugin"),
                             tr("%1 cannot be loaded:\n%2").arg(pluginId, m_application.plugins().errorString()));
        return;
    }
    m_pluginModel->refresh();
    m_pluginViewer->refresh();
    m_componentModel->refresh();
}

void MainWindow::unload(const QString& pluginId)
{
    const QList<AppForge::Component*> used = m_application.componentsOf(pluginId);
    if(!used.isEmpty())
    {
        QStringList names;
        for(const AppForge::Component* component : used)
        {
            names.append(component->objectName());
        }
        const QMessageBox::StandardButton answer =
            QMessageBox::warning(this, tr("Unload plugin"),
                                 tr("The application has components of %1:\n%2\n\n"
                                    "Unloading the plugin removes them.\nDo you want to unload it?")
                                     .arg(pluginId, names.join(QStringLiteral(", "))),
                                 QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if(answer != QMessageBox::Yes)
        {
            return;
        }
        m_scene->remove(used); // With what shows them, before the application would remove them itself
    }
    if(!m_application.unloadPlugin(pluginId))
    {
        QMessageBox::warning(this, tr("Unload plugin"),
                             tr("%1 cannot be unloaded:\n%2").arg(pluginId, m_application.errorString()));
    }
    m_pluginModel->refresh();
    m_pluginViewer->refresh();
    m_componentModel->refresh();
}

void MainWindow::openPlugin()
{
    const QString filePath = QFileDialog::getOpenFileName(this, tr("Open plugin"), m_openDirectory,
                                                          tr("AppForge plugins (*.afdl);;All files (*)"));
    if(filePath.isEmpty())
    {
        return;
    }
    m_openDirectory = QFileInfo(filePath).absolutePath();

    const std::optional<AppForge::PluginInfo> info = m_application.plugins().addFile(filePath);
    if(!info)
    {
        QMessageBox::warning(this, tr("Open plugin"),
                             tr("%1 cannot be opened:\n%2")
                                 .arg(QDir::toNativeSeparators(filePath), m_application.plugins().errorString()));
        return;
    }
    m_pluginModel->refresh();
    m_pluginView->setCurrentIndex(m_pluginModel->indexOf(info->id));
    if(!m_application.plugins().isLoaded(info->id))
    {
        load(info->id);
    }
}

void MainWindow::setProjectFile(const QString& filePath)
{
    m_projectFile = filePath;
    m_savedProject = m_scene->save();
    setWindowTitle(filePath.isEmpty() ? tr("Forge") : tr("%1 - Forge").arg(QFileInfo(filePath).fileName()));
}

bool MainWindow::saveChanges()
{
    if(m_scene->save() == m_savedProject)
    {
        return true;
    }
    const QMessageBox::StandardButton answer =
        QMessageBox::warning(this, tr("Forge"), tr("The application has been modified.\nDo you want to save it?"),
                             QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Save);
    return answer == QMessageBox::Save ? saveProject() : answer == QMessageBox::Discard;
}

void MainWindow::newProject()
{
    if(saveChanges())
    {
        m_scene->reset();
        setProjectFile({});
    }
}

void MainWindow::openProject()
{
    if(!saveChanges())
    {
        return;
    }
    // From the directory of its file, when it has one; from the default one otherwise.
    const QString filePath =
        QFileDialog::getOpenFileName(this, tr("Open project"),
                                     m_projectFile.isEmpty() ? defaultProjectDirectory() : m_projectFile,
                                     tr("AppForge projects (*.afp);;All files (*)"));
    if(!filePath.isEmpty())
    {
        openProjectFile(filePath);
    }
}

bool MainWindow::saveProject()
{
    return m_projectFile.isEmpty() ? saveProjectAs() : saveProjectFile(m_projectFile);
}

bool MainWindow::saveProjectAs()
{
    QString proposed = m_projectFile;
    if(proposed.isEmpty())
    {
        proposed = defaultProjectDirectory();
        QDir().mkpath(proposed); // There the first time a project is saved: the dialog would start elsewhere
    }
    const QString filePath =
        QFileDialog::getSaveFileName(this, tr("Save project"), proposed, tr("AppForge projects (*.afp)"));
    return !filePath.isEmpty() && saveProjectFile(filePath);
}

void MainWindow::start()
{
    const bool wasRunning = m_application.isRunning(); // Then what ran goes on, whatever the others do
    if(!m_application.start())
    {
        const QString message = wasRunning ? tr("The components dropped since the start cannot be started:\n%1")
                                           : tr("The application cannot be started:\n%1");
        QMessageBox::warning(this, tr("Start"), message.arg(m_application.errorString()));
    }
}
