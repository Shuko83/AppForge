#include "MainWindow.h"

#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QGraphicsView>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSplitter>
#include <QStatusBar>
#include <QTreeView>
#include <QVBoxLayout>
#include <optional>

#include "Component/ComponentFactory.h"
#include "ComponentModel.h"
#include "ComponentViewer.h"
#include "EditionScene.h"
#include "PluginModel.h"
#include "PluginViewer.h"

namespace
{

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
      m_componentViewer(new ComponentViewer(AppForge::ComponentFactory::instance()))
{
    setWindowTitle(tr("Forge"));

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

    m_editionView = new QGraphicsView(new EditionScene(m_assembly, AppForge::ComponentFactory::instance(), this));
    m_editionView->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    m_editionView->setRenderHint(QPainter::Antialiasing);
    m_editionView->setDragMode(QGraphicsView::RubberBandDrag);
    m_editionView->setAcceptDrops(true);
    auto* edition = new QGroupBox(tr("Application"));
    auto* editionLayout = new QVBoxLayout(edition);
    editionLayout->addWidget(m_editionView);

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
    resize(1440, 800);
}

MainWindow::~MainWindow()
{
    // Its items refer to the components of m_assembly, destroyed after this body, before the children of the window.
    delete m_editionView->scene();
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
                                                          tr("AppForge plugins (*.afplugin);;All files (*)"));
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
