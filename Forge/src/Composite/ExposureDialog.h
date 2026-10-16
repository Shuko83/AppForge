// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QDialog>
#include <QHash>
#include <QMetaType>
#include <QPointer>
#include <QString>
#include <cstdint>

#include "AppForge/Composite/Composite.h"
#include "AppForge/Composite/Gateway.h"

class QDialogButtonBox;
class QLabel;
class QTreeWidget;
class QTreeWidgetItem;

namespace AppForge
{
class ComponentFactory;
class ForgeApplication;
} // namespace AppForge

// Chooses what a composite exposes, among what its components have, as the dialog of a settings file chooses what it
// holds: under each component, its interfaces, each exposed through a gateway, then its properties, those a project
// holds, and those a composite among them exposes, which it exposes again. Next to what is checked, the name it is
// exposed as, unique in the composite: the one of the interface or of the property until another one is typed. Two
// interfaces consumed, of the same type, checked under the same name, are given what the composite consumes by the same
// gateway. What was chosen is given to the composite, so to every instance of its file, once the dialog is accepted.
class ExposureDialog : public QDialog
{
    Q_OBJECT

  public:
    // composite, one of those of application, outlives the dialog, but for its components, which can go meanwhile.
    ExposureDialog(AppForge::Composite& composite, AppForge::ForgeApplication& application,
                   const AppForge::ComponentFactory& factory, QWidget* parent = nullptr);

  private:
    enum class Kind : std::uint8_t
    {
        ProvidedInterface, // Exposed by a gateway the composite provides through
        ConsumedInterface, // Exposed by a gateway the composite consumes through
        Property,
    };

    // What a row of the tree exposes.
    struct Choice
    {
        Kind kind = Kind::Property;
        QPointer<AppForge::Component> component; // nullptr for a gateway linked to nothing
        QPointer<QObject> object;                // Of a property: the component, or one of its objects
        QString name;                            // Of the interface, or of the property
        QMetaType type;                          // Of an interface
        QString exposedAs;                       // What it is exposed as now; empty when it is not
        QPointer<AppForge::Gateway> gateway;     // The one of an interface exposed now
    };

    // Lists what the components of the composite have to expose.
    void fill(const AppForge::ComponentFactory& factory);
    // Adds the row of choice, under parent, checked when it is exposed.
    void addChoice(QTreeWidgetItem* parent, const QString& text, const QString& about, const Choice& choice);
    // The row of a checked choice is given its name, the one of the interface or of the property, when it has none, and
    // the rows of the same gateway keep its name; then what is checked is checked.
    void edited(QTreeWidgetItem* row, int column);
    // Tells why it cannot be accepted, when it cannot: a name is empty, or not unique.
    void check();
    // Gives the composite what was chosen.
    void apply();

    AppForge::Composite& m_composite;
    AppForge::ForgeApplication& m_application;
    QHash<const QTreeWidgetItem*, Choice> m_choices; // What each row of the tree exposes
    QTreeWidget* m_tree = nullptr;
    QLabel* m_problem = nullptr;
    QDialogButtonBox* m_buttons = nullptr;
};
