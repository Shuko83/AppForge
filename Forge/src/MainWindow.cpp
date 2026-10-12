#include "MainWindow.h"

#include <QCloseEvent>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QGraphicsView>
#include <QGroupBox>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QSaveFile>
#include <QSplitter>
#include <QStatusBar>
#include <QTreeView>
#include <QVBoxLayout>
#include <map>
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

// The version of the project files written, the only one read.
constexpr int ProjectVersion = 1;

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

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), m_assembly(AppForge::ComponentFactory::instance()),
      m_pluginModel(new PluginModel(m_plugins, this)),
      m_pluginViewer(new PluginViewer(m_plugins)),
      m_componentModel(new ComponentModel(AppForge::ComponentFactory::instance(), this)),
      m_componentViewer(new ComponentViewer(AppForge::ComponentFactory::instance())),
      m_propertyEditor(new PropertyEditor(AppForge::ComponentFactory::instance()))
{
    QMenu* fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(tr("&New"), QKeySequence::New, this, &MainWindow::newProject);
    fileMenu->addAction(tr("&Open..."), QKeySequence::Open, this, &MainWindow::openProject);
    fileMenu->addAction(tr("&Save"), QKeySequence::Save, this, &MainWindow::saveProject);
    fileMenu->addAction(tr("Save &As..."), QKeySequence::SaveAs, this, &MainWindow::saveProjectAs);

    m_openDirectory = AppForge::PluginManager::defaultDirectory();
    m_plugins.scan();
    m_pluginModel->refresh();
    m_componentModel->refresh();

    m_pluginView = createView(*m_pluginModel);
    connect(m_pluginView->selectionModel(), &QItemSelectionModel::currentRowChanged, this,
            [this](const QModelIndex& current)
            { m_pluginViewer->setPluginId(current.data(PluginModel::IdRole).toString()); });
    connect(m_pluginViewer, &PluginViewer::loadRequested, this, &MainWindow::load);

    auto* openButton = new QPushButton(tr("Open plugin..."));
    openButton->setToolTip(tr("Load a plugin from anywhere on the disk"));
    connect(openButton, &QPushButton::clicked, this, &MainWindow::openPlugin);
    auto* pluginList = new QWidget;
    auto* pluginListLayout = new QVBoxLayout(pluginList);
    pluginListLayout->setContentsMargins({});
    pluginListLayout->addWidget(openButton, 0, Qt::AlignLeft);
    pluginListLayout->addWidget(m_pluginView);

    QTreeView* componentView = createView(*m_componentModel);
    componentView->setDragEnabled(true);
    componentView->setDragDropMode(QAbstractItemView::DragOnly);
    connect(componentView->selectionModel(), &QItemSelectionModel::currentRowChanged, this,
            [this](const QModelIndex& current)
            { m_componentViewer->setComponentId(current.data(ComponentModel::IdRole).toString()); });

    m_scene = new EditionScene(m_assembly, AppForge::ComponentFactory::instance(), this);
    m_editionView = new QGraphicsView(m_scene);
    m_editionView->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    m_editionView->setRenderHint(QPainter::Antialiasing);
    m_editionView->setDragMode(QGraphicsView::RubberBandDrag);
    m_editionView->setAcceptDrops(true);
    connect(m_scene, &QGraphicsScene::selectionChanged, this,
            [this] { m_propertyEditor->setComponent(m_scene->selectedComponent()); });

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
    splitter->addWidget(createPanel(tr("Components"), componentView, m_componentViewer));
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
    // Its items, and the property editor, refer to the components of m_assembly, destroyed after this body, before the
    // children of the window. The scene is no longer listened to: deleting a selected item changes its selection.
    m_scene->disconnect(this);
    m_propertyEditor->setComponent(nullptr);
    delete m_scene;
}

bool MainWindow::openProjectFile(const QString& filePath)
{
    // Told before anything is changed.
    const auto refuse = [&](const QString& reason)
    {
        QMessageBox::warning(this, tr("Open project"),
                             tr("%1 cannot be opened:\n%2").arg(QDir::toNativeSeparators(filePath), reason));
        return false;
    };
    QFile file(filePath);
    if(!file.open(QIODevice::ReadOnly))
    {
        return refuse(file.errorString());
    }
    QJsonParseError parsed;
    const QJsonObject project = QJsonDocument::fromJson(file.readAll(), &parsed).object();
    if(parsed.error != QJsonParseError::NoError)
    {
        return refuse(parsed.errorString());
    }
    if(project.value(u"version").toInt() != ProjectVersion)
    {
        return refuse(tr("It is not an AppForge project of version %1").arg(ProjectVersion));
    }

    QStringList problems;
    const QJsonArray plugins = project.value(u"plugins").toArray();
    for(const QJsonValue& value : plugins)
    {
        const QJsonObject plugin = value.toObject();
        const QString pluginId = plugin.value(u"id").toString();
        // Opened from its file when it is not found already, as the ones of plugins/ are.
        if((!m_plugins.plugin(pluginId) && !m_plugins.addFile(plugin.value(u"file").toString())) ||
           (!m_plugins.isLoaded(pluginId) && !m_plugins.load(pluginId)))
        {
            problems.append(tr("The plugin %1 is not loaded: %2").arg(pluginId, m_plugins.errorString()));
        }
    }
    m_pluginModel->refresh();
    m_pluginViewer->refresh();
    m_componentModel->refresh();

    problems.append(m_scene->load(project));
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
    m_scene->setFocusItem(nullptr); // The title of a zone is no longer edited: it is saved as it was typed
    QSaveFile file(filePath);
    if(!file.open(QIODevice::WriteOnly) || file.write(QJsonDocument(project()).toJson()) < 0 || !file.commit())
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
    if(!m_plugins.load(pluginId))
    {
        QMessageBox::warning(this, tr("Load plugin"),
                             tr("%1 cannot be loaded:\n%2").arg(pluginId, m_plugins.errorString()));
        return;
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

    const std::optional<AppForge::PluginInfo> info = m_plugins.addFile(filePath);
    if(!info)
    {
        QMessageBox::warning(this, tr("Open plugin"),
                             tr("%1 cannot be opened:\n%2")
                                 .arg(QDir::toNativeSeparators(filePath), m_plugins.errorString()));
        return;
    }
    m_pluginModel->refresh();
    m_pluginView->setCurrentIndex(m_pluginModel->indexOf(info->id));
    if(!m_plugins.isLoaded(info->id))
    {
        load(info->id);
    }
}

QJsonObject MainWindow::project() const
{
    // The plugins of its components, each once: those to load before it is opened again.
    std::map<QString, QString> files; // By plugin id
    for(const std::unique_ptr<AppForge::Component>& component : m_assembly.components())
    {
        const std::optional<AppForge::ComponentInfo> info =
            AppForge::ComponentFactory::instance().component(component->componentId());
        const std::optional<AppForge::PluginInfo> plugin = info ? m_plugins.plugin(info->pluginId) : std::nullopt;
        if(plugin)
        {
            files.emplace(plugin->id, plugin->filePath);
        }
    }
    QJsonArray plugins;
    for(const auto& [pluginId, filePath] : files)
    {
        QJsonObject plugin;
        plugin.insert(u"id", pluginId);
        plugin.insert(u"file", filePath);
        plugins.append(plugin);
    }

    QJsonObject project = m_scene->save();
    project.insert(u"version", ProjectVersion);
    project.insert(u"plugins", plugins);
    return project;
}

void MainWindow::setProjectFile(const QString& filePath)
{
    m_projectFile = filePath;
    m_savedProject = project();
    setWindowTitle(filePath.isEmpty() ? tr("Forge") : tr("%1 - Forge").arg(QFileInfo(filePath).fileName()));
}

bool MainWindow::saveChanges()
{
    if(project() == m_savedProject)
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
