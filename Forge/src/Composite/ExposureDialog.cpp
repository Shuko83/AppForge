// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "ExposureDialog.h"

#include <QDialogButtonBox>
#include <QFileInfo>
#include <QHeaderView>
#include <QLabel>
#include <QList>
#include <QMessageBox>
#include <QMetaProperty>
#include <QPushButton>
#include <QSet>
#include <QSignalBlocker>
#include <QStringList>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <QVBoxLayout>
#include <memory>
#include <optional>
#include <utility>

#include "AppForge/Application/ForgeApplication.h"
#include "AppForge/Component/ComponentFactory.h"
#include "AppForge/Settings/SettingsFile.h"

namespace
{

enum Column : std::uint8_t
{
    WhatColumn,
    AsColumn,
    TypeColumn,
    ColumnCount,
};

// The class a type of interface points to, as the dot of an interface tells it: QWidget for QWidget*.
QString pointedType(QMetaType type)
{
    QString name = QString::fromLatin1(type.name());
    if(name.endsWith(u'*'))
    {
        name.chop(1);
    }
    return name;
}

} // namespace

ExposureDialog::ExposureDialog(AppForge::Composite& composite, AppForge::ForgeApplication& application,
                               const AppForge::ComponentFactory& factory, QWidget* parent)
    : QDialog(parent), m_composite(composite), m_application(application)
{
    const QString name = QFileInfo(composite.filePath()).completeBaseName();
    setWindowTitle(tr("Exposed by %1").arg(name));

    auto* hint = new QLabel(tr("What is checked is what %1 shows to the scene it is placed in: its interfaces, each "
                               "through a gateway, and its properties, edited on it. Every instance of %2 exposes the "
                               "same. Double-click the name something is exposed as to change it.")
                                .arg(name, QFileInfo(composite.filePath()).fileName()));
    hint->setWordWrap(true);

    m_tree = new QTreeWidget;
    m_tree->setColumnCount(ColumnCount);
    m_tree->setHeaderLabels({tr("Interface or property"), tr("Exposed as"), tr("Type")});
    m_tree->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_tree->setEditTriggers(QAbstractItemView::NoEditTriggers); // The name only, see below
    fill(factory);
    connect(m_tree, &QTreeWidget::itemChanged, this, &ExposureDialog::edited);
    connect(m_tree, &QTreeWidget::itemDoubleClicked, this,
            [this](QTreeWidgetItem* row, int /*column*/)
            {
                if(m_choices.contains(row) && row->checkState(WhatColumn) == Qt::Checked)
                {
                    m_tree->editItem(row, AsColumn);
                }
            });

    m_problem = new QLabel;
    m_problem->setWordWrap(true);
    m_buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(m_buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(this, &QDialog::accepted, this, &ExposureDialog::apply);
    // Should the composite go meanwhile, with what it would have been given.
    connect(&m_composite, &QObject::destroyed, this, &QDialog::reject);

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(hint);
    layout->addWidget(m_tree, 1);
    layout->addWidget(m_problem);
    layout->addWidget(m_buttons);
    check();
    resize(620, 560);
}

void ExposureDialog::fill(const AppForge::ComponentFactory& factory)
{
    QList<AppForge::Gateway*> gateways;
    for(const std::unique_ptr<AppForge::Component>& component : m_composite.components())
    {
        if(auto* gateway = qobject_cast<AppForge::Gateway*>(component.get()))
        {
            gateways.append(gateway);
        }
    }
    QSet<const AppForge::Gateway*> listed; // Those a row is linked to

    for(const std::unique_ptr<AppForge::Component>& owned : m_composite.components())
    {
        AppForge::Component* component = owned.get();
        if(qobject_cast<AppForge::Gateway*>(component) != nullptr)
        {
            continue; // What exposes the others
        }
        auto* componentRow = new QTreeWidgetItem({component->objectName()});
        componentRow->setFlags(Qt::ItemIsEnabled);
        componentRow->setToolTip(WhatColumn, component->componentId());

        // Its interfaces, those exposed by the gateway linked to them.
        auto* interfacesRow = new QTreeWidgetItem(componentRow, {tr("Interfaces")});
        interfacesRow->setFlags(Qt::ItemIsEnabled);
        const QList<AppForge::InterfaceInfo> providedInterfaces = component->providedInterfaces();
        for(const AppForge::InterfaceInfo& provided : providedInterfaces)
        {
            Choice choice{
                .kind = Kind::ProvidedInterface, .component = component, .name = provided.name, .type = provided.type};
            for(AppForge::Gateway* gateway : std::as_const(gateways))
            {
                const AppForge::InterfaceBinding bound = gateway->binding(AppForge::Gateway::interfaceName());
                if(gateway->kind() == AppForge::GatewayKind::Provided && bound.provider == component &&
                   bound.providedName == provided.name)
                {
                    choice.gateway = gateway;
                    choice.exposedAs = gateway->objectName();
                    listed.insert(gateway);
                    break;
                }
            }
            addChoice(interfacesRow, provided.name, tr("Provides %1").arg(pointedType(provided.type)), choice);
        }
        const QList<AppForge::InterfaceInfo> consumedInterfaces = component->consumedInterfaces();
        for(const AppForge::InterfaceInfo& consumed : consumedInterfaces)
        {
            Choice choice{
                .kind = Kind::ConsumedInterface, .component = component, .name = consumed.name, .type = consumed.type};
            auto* gateway = qobject_cast<AppForge::Gateway*>(component->binding(consumed.name).provider);
            if(gateway != nullptr && gateway->kind() == AppForge::GatewayKind::Consumed)
            {
                choice.gateway = gateway;
                choice.exposedAs = gateway->objectName();
                listed.insert(gateway);
            }
            addChoice(interfacesRow, consumed.name, tr("Consumes %1").arg(pointedType(consumed.type)), choice);
        }
        if(interfacesRow->childCount() == 0)
        {
            delete interfacesRow;
            interfacesRow = nullptr;
        }

        // Its properties: those a composite exposes, which it exposes again; those a project holds otherwise, as the
        // property editor shows them, its own, then those of its objects, under their names.
        if(auto* inner = qobject_cast<AppForge::Composite*>(component))
        {
            for(const AppForge::ExposedProperty& exposed : inner->exposedProperties())
            {
                const AppForge::ExposedTarget target = inner->exposedProperty(exposed.name);
                if(target.isValid())
                {
                    addChoice(componentRow, exposed.name, QString::fromLatin1(target.property.typeName()),
                              Choice{.kind = Kind::Property,
                                     .component = inner,
                                     .object = inner,
                                     .name = exposed.name,
                                     .exposedAs = m_application.exposedName(*inner, inner, exposed.name)});
                }
            }
        }
        else
        {
            // The properties of object a project holds, under parent.
            const auto list =
                [this, component](QTreeWidgetItem* parent, QObject* object, const QList<QMetaProperty>& properties)
            {
                for(const QMetaProperty& property : properties)
                {
                    const QString name = QString::fromLatin1(property.name());
                    if(AppForge::SettingsFile::canHold(*component, object, name))
                    {
                        addChoice(parent, name, QString::fromLatin1(property.typeName()),
                                  Choice{.kind = Kind::Property,
                                         .component = component,
                                         .object = object,
                                         .name = name,
                                         .exposedAs = m_application.exposedName(*component, object, name)});
                    }
                }
            };
            if(const std::optional<AppForge::ComponentInfo> info = factory.component(component->componentId()))
            {
                list(componentRow, component, info->properties);
            }
            const QList<AppForge::PropertyObject> objects = component->propertyObjects();
            QStringList names{QString()};
            for(const AppForge::PropertyObject& added : objects)
            {
                if(!names.contains(added.name))
                {
                    names.append(added.name);
                }
            }
            for(const QString& name : std::as_const(names))
            {
                auto* group = name.isEmpty() ? componentRow : new QTreeWidgetItem({name});
                group->setFlags(Qt::ItemIsEnabled);
                for(const AppForge::PropertyObject& added : objects)
                {
                    if(added.name == name)
                    {
                        list(group, added.object, added.properties());
                    }
                }
                if(group != componentRow)
                {
                    if(group->childCount() > 0)
                    {
                        componentRow->addChild(group);
                    }
                    else
                    {
                        delete group; // Nothing to expose
                    }
                }
            }
        }

        if(componentRow->childCount() > 0)
        {
            m_tree->addTopLevelItem(componentRow);
            componentRow->setExpanded(true);
            if(interfacesRow != nullptr) // Open: they are few
            {
                interfacesRow->setExpanded(true);
            }
        }
        else
        {
            delete componentRow;
        }
    }

    // The gateways linked to nothing inside: exposed all the same, until they are unchecked.
    auto* unlinked = new QTreeWidgetItem({tr("Linked to nothing")});
    unlinked->setFlags(Qt::ItemIsEnabled);
    for(AppForge::Gateway* gateway : std::as_const(gateways))
    {
        if(listed.contains(gateway))
        {
            continue;
        }
        const bool provided = gateway->kind() == AppForge::GatewayKind::Provided;
        addChoice(unlinked, gateway->objectName(),
                  (provided ? tr("Provides %1") : tr("Consumes %1")).arg(pointedType(gateway->interfaceType())),
                  Choice{.kind = provided ? Kind::ProvidedInterface : Kind::ConsumedInterface,
                         .name = gateway->objectName(),
                         .type = gateway->interfaceType(),
                         .exposedAs = gateway->objectName(),
                         .gateway = gateway});
    }
    if(unlinked->childCount() > 0)
    {
        m_tree->addTopLevelItem(unlinked);
        unlinked->setExpanded(true);
    }
    else
    {
        delete unlinked;
    }

    // The groups of the objects, which can have many properties, a widget say, are only open when one is exposed.
    for(QTreeWidgetItemIterator row(m_tree); *row != nullptr; ++row)
    {
        const bool exposed = m_choices.contains(*row) && (*row)->checkState(WhatColumn) == Qt::Checked;
        for(QTreeWidgetItem* parent = (*row)->parent(); exposed && parent != nullptr; parent = parent->parent())
        {
            parent->setExpanded(true);
        }
    }
}

void ExposureDialog::addChoice(QTreeWidgetItem* parent, const QString& text, const QString& about, const Choice& choice)
{
    auto* row = new QTreeWidgetItem(parent, {text, choice.exposedAs, about});
    row->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable | Qt::ItemIsEditable);
    row->setCheckState(WhatColumn, choice.exposedAs.isEmpty() ? Qt::Unchecked : Qt::Checked);
    m_choices.insert(row, choice);
}

void ExposureDialog::edited(QTreeWidgetItem* row, int column)
{
    const auto choice = m_choices.constFind(row);
    if(choice == m_choices.cend())
    {
        return;
    }
    const QSignalBlocker blocker(m_tree); // What is set here is not edited
    if(column == WhatColumn && row->checkState(WhatColumn) == Qt::Checked && row->text(AsColumn).trimmed().isEmpty())
    {
        row->setText(AsColumn, choice->name);
    }
    // Those of the same gateway, which has one name.
    if(column == AsColumn && choice->gateway != nullptr)
    {
        for(auto other = m_choices.cbegin(); other != m_choices.cend(); ++other)
        {
            if(other.key() != row && other->gateway == choice->gateway)
            {
                const_cast<QTreeWidgetItem*>(other.key())->setText(AsColumn, row->text(AsColumn));
            }
        }
    }
    check();
}

void ExposureDialog::check()
{
    QString problem;
    if(!m_application.isComplete(m_composite.filePath()))
    {
        problem = tr("%1 could not be entirely read: what it exposes is not changed")
                      .arg(QFileInfo(m_composite.filePath()).fileName());
    }
    QHash<QString, const Choice*> interfaces; // By the name they are exposed as: the first one
    QStringList properties;
    for(QTreeWidgetItemIterator iterator(m_tree); *iterator != nullptr && problem.isEmpty(); ++iterator)
    {
        const QTreeWidgetItem* row = *iterator;
        const auto choice = m_choices.constFind(row);
        if(choice == m_choices.cend() || row->checkState(WhatColumn) != Qt::Checked)
        {
            continue;
        }
        const QString name = row->text(AsColumn).trimmed();
        if(name.isEmpty())
        {
            problem = tr("%1 needs a name to be exposed as").arg(row->text(WhatColumn));
        }
        else if(choice->kind == Kind::Property)
        {
            if(properties.contains(name))
            {
                problem = tr("Two properties are exposed as %1").arg(name);
            }
            properties.append(name);
        }
        else if(name.contains(u'/'))
        {
            problem = tr("%1 holds a /, which separates the names of a path").arg(name);
        }
        else if(const AppForge::Component* named = m_composite.component(name);
                named != nullptr && qobject_cast<const AppForge::Gateway*>(named) == nullptr)
        {
            problem = tr("%1 is the name of a component of %2").arg(name, m_composite.objectName());
        }
        else if(const Choice* other = interfaces.value(name))
        {
            // Those consumed, of the same type, are given it by the same gateway; any other is passed by its own.
            const bool shared = other->kind == Kind::ConsumedInterface && choice->kind == Kind::ConsumedInterface &&
                                other->type == choice->type;
            if(!shared)
            {
                problem = tr("Two interfaces are exposed as %1: only those consumed, of the same type, can share it")
                              .arg(name);
            }
        }
        else
        {
            interfaces.insert(name, &*choice);
        }
    }
    m_problem->setText(problem);
    m_problem->setVisible(!problem.isEmpty());
    m_buttons->button(QDialogButtonBox::Ok)->setEnabled(problem.isEmpty());
}

void ExposureDialog::apply()
{
    // In the order of the tree, as it was edited.
    struct Edited
    {
        Choice choice;
        bool checked = false;
        QString name; // What it is to be exposed as
    };
    QList<Edited> rows;
    for(QTreeWidgetItemIterator iterator(m_tree); *iterator != nullptr; ++iterator)
    {
        const auto choice = m_choices.constFind(*iterator);
        if(choice != m_choices.cend())
        {
            rows.append(Edited{.choice = *choice,
                               .checked = (*iterator)->checkState(WhatColumn) == Qt::Checked,
                               .name = (*iterator)->text(AsColumn).trimmed()});
        }
    }
    QSet<const AppForge::Gateway*> kept; // Those a checked row is linked to
    for(const Edited& row : std::as_const(rows))
    {
        if(row.checked && row.choice.gateway != nullptr)
        {
            kept.insert(row.choice.gateway);
        }
    }
    QStringList failures;

    // What is no longer exposed first: its names are free then.
    for(const Edited& row : std::as_const(rows))
    {
        if(row.checked || row.choice.exposedAs.isEmpty())
        {
            continue;
        }
        if(row.choice.kind == Kind::Property)
        {
            m_application.unexposeProperty(m_composite, row.choice.exposedAs);
        }
        else if(row.choice.gateway == nullptr)
        {
            continue; // Gone already, with another row of it
        }
        else if(!kept.contains(row.choice.gateway))
        {
            m_application.remove(*row.choice.gateway);
        }
        else if(row.choice.component != nullptr)
        {
            // Of a gateway that still gives what the composite consumes to others.
            m_application.unbind(*row.choice.component, row.choice.name);
        }
    }

    // Then what is exposed under another name.
    for(const Edited& row : std::as_const(rows))
    {
        if(!row.checked || row.choice.exposedAs.isEmpty() || row.name == row.choice.exposedAs)
        {
            continue;
        }
        const bool renamed = row.choice.kind == Kind::Property
                                 ? m_application.renameExposedProperty(m_composite, row.choice.exposedAs, row.name)
                                 : row.choice.gateway == nullptr || row.choice.gateway->objectName() == row.name ||
                                       m_application.rename(*row.choice.gateway, row.name);
        if(!renamed)
        {
            failures.append(tr("%1 is not exposed as %2: the name is taken").arg(row.choice.exposedAs, row.name));
        }
    }

    // Then what is exposed from now on.
    for(const Edited& row : std::as_const(rows))
    {
        if(!row.checked || !row.choice.exposedAs.isEmpty() || row.choice.component == nullptr)
        {
            continue;
        }
        if(row.choice.kind == Kind::Property)
        {
            if(m_application.exposeProperty(*row.choice.component, row.choice.object, row.choice.name, row.name)
                   .isEmpty())
            {
                failures.append(m_application.errorString());
            }
            continue;
        }
        // A gateway of that name the composite consumes through already, of that type, gives it to this one too.
        AppForge::Gateway* shared = nullptr;
        if(row.choice.kind == Kind::ConsumedInterface)
        {
            auto* named = qobject_cast<AppForge::Gateway*>(m_composite.component(row.name));
            if(named != nullptr && named->kind() == AppForge::GatewayKind::Consumed &&
               named->interfaceType() == row.choice.type)
            {
                shared = named;
            }
        }
        const bool exposed = shared != nullptr
                                 ? m_application.bind(*row.choice.component, row.choice.name, *shared,
                                                      AppForge::Gateway::interfaceName())
                                 : m_application.expose(*row.choice.component, row.choice.name, row.name) != nullptr;
        if(!exposed)
        {
            failures.append(m_application.errorString());
        }
    }

    if(!failures.isEmpty())
    {
        QMessageBox::warning(parentWidget(), windowTitle(), failures.join(u'\n'));
    }
}
