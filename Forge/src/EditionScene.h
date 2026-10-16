// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QColor>
#include <QGraphicsScene>
#include <QJsonObject>
#include <QList>
#include <QStringList>
#include <cstddef>
#include <memory>
#include <vector>

class ComponentItem;
class InterfaceItem;
class QGraphicsPathItem;
class SettingsItem;
class ZoneItem;

namespace AppForge
{
class Component;
class ComponentFactory;
class Composite;
class ForgeApplication;
class SettingsFile;
} // namespace AppForge

// The zone where the application is assembled: a component dragged from a ComponentModel and dropped here is
// instantiated in the application, and shown where it was dropped; a composite is placed there from its file. Dragging
// from the dot of an interface to the dot of one it can be bound to, a provided and a consumed one of the same type,
// binds them and draws a link between them; meanwhile, the interfaces it cannot be bound to are dimmed. Its context
// menu adds a zone, which groups components, see ZoneItem, unless it is opened on a component, and renames, recolors or
// removes the one under the cursor; it renames and recolors the component under the cursor too, see ComponentItem. A
// component keeps a name no other one of the application has: see refused().
// The same menu removes the component under the cursor from the application, with its links, and the Delete key
// removes the selected ones. While the application runs, it also starts that component, unless it runs already. It
// opens a composite as well, see openRequested(), and groups the selected components into one, written to a file of
// its own, which takes their place.
// The menu adds a settings file of the application as well, see SettingsItem, whose properties are chosen at once, and
// again when it is double-clicked or from its own menu, which writes its default values or removes it.
// The scene of a composite shows what it is made of, and is worked on alike, but for the settings files and the
// grouping, which are those of the application: what is placed, bound, renamed or removed there changes the structure
// of its file, and so every instance of it, see ForgeApplication. What the composite exposes is a single item there,
// see ExposureItem, chosen when it is double-clicked or from its menu, see ExposureDialog: its gateways are its dots,
// rather than components of their own.
class EditionScene : public QGraphicsScene
{
    Q_OBJECT

  public:
    // The scene of application, or of composite, one of its composites, when it is given.
    EditionScene(AppForge::ForgeApplication& application, const AppForge::ComponentFactory& factory,
                 AppForge::Composite* composite = nullptr, QObject* parent = nullptr);

    // The composite it is the scene of; nullptr for the scene of the application.
    [[nodiscard]] AppForge::Composite* composite() const;
    // The component selected, when a single one is; nullptr otherwise. See selectionChanged().
    [[nodiscard]] AppForge::Component* selectedComponent() const;
    // Where Group into composite... proposes to write the composite file.
    void setDirectory(const QString& directory);

    // The application as a project file in directory holds it, see README.md: what ForgeApplication::save() writes,
    // with where its components and its settings files are in the edition zone, the color its components were given,
    // and its zones. For the scene of a composite, its file as it is now: what ForgeApplication::saveComposite()
    // writes, with where its components are, their color and its zones.
    [[nodiscard]] QJsonObject save(const QString& directory) const;
    // Replaces the application with the one project, a file of directory, holds, as save() wrote it, see
    // ForgeApplication::load(), and shows it as it was. Returns what could not be restored, a sentence each: none when
    // everything was. For the scene of the application.
    QStringList load(const QJsonObject& project, const QString& directory);
    // Shows its composite as document, its file as save() wrote it, places them: where its components are, their
    // color and its zones. For the scene of a composite.
    void show(const QJsonObject& document);
    // Empties the edition zone, and the application of its components.
    void reset();
    // Removes components, some of the application, from it and from the edition zone, with their links.
    void remove(const QList<AppForge::Component*>& components);

  signals:
    // What was just asked of it is not done; why is a sentence to show.
    void refused(const QString& why);
    // What was just asked of it is done, which is worth telling; what is a sentence to show.
    void told(const QString& what);
    // composite, one of the components it shows, is to be opened: its own scene is to be shown in place of this one.
    void openRequested(AppForge::Composite& composite);
    // A composite was placed, or made of components: the composite files the application read can have changed.
    void compositesChanged();

  protected:
    void dragEnterEvent(QGraphicsSceneDragDropEvent* event) override;
    void dragMoveEvent(QGraphicsSceneDragDropEvent* event) override;
    void dropEvent(QGraphicsSceneDragDropEvent* event) override;
    // Pressing the dot of an interface starts a link, which follows the cursor until the button is released.
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
    // Double-clicking a settings file edits it.
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) override;
    void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override;
    // Delete removes the selected components and settings files, unless a name is edited: the key is for what is
    // typed then.
    void keyPressEvent(QKeyEvent* event) override;
    // A grid, and a hint while nothing is in the edition zone.
    void drawBackground(QPainter* painter, const QRectF& rect) override;

  private:
    // Those of its composite, or of the application.
    [[nodiscard]] const std::vector<std::unique_ptr<AppForge::Component>>& components() const;
    // Whether what it shows can be changed: tells why through refused() when it cannot, its composite file not having
    // been read entirely.
    bool canChange();
    // Shows its components, and their links, where document, as save() wrote it, places them, then its settings files
    // and its zones.
    void showDocument(const QJsonObject& document);
    // Asks for a composite file, writes the selected components to it, with their links, then shows the composite that
    // takes their place, where they were. Dialogs are of parent.
    void groupSelection(QWidget* parent);
    // The interface whose dot is at position; nullptr when there is none.
    [[nodiscard]] InterfaceItem* interfaceAt(const QPointF& position) const;
    // The interface the link being drawn would be bound to if released at position; nullptr when there is none.
    [[nodiscard]] InterfaceItem* linkTargetAt(const QPointF& position) const;
    // Dims the interfaces a link drawn from source cannot be bound to; none when source is nullptr.
    void dimLinkTargets(const InterfaceItem* source);
    // Shows component, one of the application, at position.
    ComponentItem* showComponent(AppForge::Component& component, const QPointF& position);
    // Draws the links between the dots it shows, those of components and of what its composite exposes, as they are
    // bound now: those that are not drawn yet, and no longer those that are no longer bound.
    void showLinks();
    // Has what its composite exposes chosen in a dialog of parent, see ExposureDialog, which is left open once this
    // returns.
    void editExposure(QWidget* parent);
    // The component shown at position, or whose interface is; nullptr when there is none.
    [[nodiscard]] ComponentItem* componentAt(const QPointF& position) const;
    // Names component name, as typed in place of its name; tells why through refused() when it cannot be.
    void rename(AppForge::Component& component, const QString& name);
    // Removes item, with the links of its interfaces, and its component from the application.
    void remove(ComponentItem& item);
    // Empties the edition zone, while the components it shows still exist.
    void removeItems();
    // Binds the consumed one to the provided one, whichever is which, and draws their link, in place of the one the
    // consumed one had; does nothing when they cannot be bound.
    void link(InterfaceItem& source, InterfaceItem& target);
    // The zone position is in, the smallest one when several are; nullptr when there is none.
    [[nodiscard]] ZoneItem* zoneAt(const QPointF& position) const;
    // Adds a zone, its top left corner at position; of the next color when color is not valid.
    ZoneItem* addZone(const QPointF& position, const QSizeF& size, const QString& title, const QColor& color = {});
    // Shows file, one of the application, at position.
    SettingsItem* showSettings(AppForge::SettingsFile& file, const QPointF& position);
    // The settings file at position, unless something is shown over it there; nullptr when there is none.
    [[nodiscard]] SettingsItem* settingsAt(const QPointF& position) const;
    // Edits the file of item in a dialog of parent, see SettingsDialog, which is left open once this returns.
    void editSettings(const SettingsItem& item, QWidget* parent);
    // Writes the default values of the file of item to it, and tells whether it could.
    void writeDefaults(const SettingsItem& item);
    // Removes item, with its links, and its file from the application.
    void remove(SettingsItem& item);

    AppForge::ForgeApplication& m_application;
    const AppForge::ComponentFactory& m_factory;
    AppForge::Composite* m_composite = nullptr; // nullptr for the scene of the application
    QString m_directory;                        // Where Group into composite... starts
    InterfaceItem* m_linkSource = nullptr;      // Where the link being drawn starts; nullptr when none is
    QGraphicsPathItem* m_pendingLink = nullptr; // The link being drawn
    std::size_t m_zoneCount = 0;                // The zones added so far: each takes the next color
};
