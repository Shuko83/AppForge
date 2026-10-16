// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "PropertyEditor.h"

#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QMetaMethod>
#include <QSignalBlocker>
#include <QStackedWidget>
#include <QStringList>
#include <QTableWidget>
#include <QVBoxLayout>
#include <cstdint>
#include <optional>
#include <utility>

#include "AppForge/Application/ForgeApplication.h"
#include "AppForge/Component/Component.h"
#include "AppForge/Component/ComponentFactory.h"

namespace
{

enum Column : std::uint8_t
{
    NameColumn,
    ValueColumn,
    ColumnCount,
};

// Whether a cell edits property: it can be written, and its type is one the default editors of Qt take.
bool isEditable(const QMetaProperty& property)
{
    if(!property.isWritable())
    {
        return false;
    }
    switch(property.typeId())
    {
    case QMetaType::Bool:
    case QMetaType::Int:
    case QMetaType::UInt:
    case QMetaType::Double:
    case QMetaType::QString:
    case QMetaType::QDate:
    case QMetaType::QTime:
    case QMetaType::QDateTime:
        return true;
    default:
        return false;
    }
}

} // namespace

PropertyEditor::PropertyEditor(AppForge::ForgeApplication& application, const AppForge::ComponentFactory& factory,
                               QWidget* parent)
    : QWidget(parent), m_application(application), m_factory(factory)
{
    // Which properties are in italics.
    connect(&m_application, &AppForge::ForgeApplication::settingsFilesChanged, this, &PropertyEditor::rebuild);

    m_placeholder = new QLabel;
    m_placeholder->setAlignment(Qt::AlignCenter);

    m_table = new QTableWidget(0, ColumnCount);
    m_table->setHorizontalHeaderLabels({tr("Property"), tr("Value")});
    m_table->verticalHeader()->hide();
    m_table->horizontalHeader()->setSectionResizeMode(NameColumn, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::AllEditTriggers); // A click on a value edits it
    connect(m_table, &QTableWidget::itemChanged, this, &PropertyEditor::write);
    m_table->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_table, &QTableWidget::customContextMenuRequested, this, &PropertyEditor::showMenu);

    m_stack = new QStackedWidget;
    m_stack->addWidget(m_placeholder);
    m_stack->addWidget(m_table);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins({});
    layout->addWidget(m_stack);

    setComponent(nullptr);
}

void PropertyEditor::setComponent(AppForge::Component* component)
{
    if(m_component != nullptr)
    {
        m_component->disconnect(this);
    }
    if(m_composite != nullptr)
    {
        m_composite->disconnect(this);
    }
    m_component = component;
    m_composite = component != nullptr ? m_application.compositeOf(*component) : nullptr;
    if(component != nullptr)
    {
        connect(component, &QObject::objectNameChanged, this, &PropertyEditor::refreshPlaceholder);
        connect(component, &AppForge::Component::propertyObjectsChanged, this, &PropertyEditor::rebuild);
        // What it exposes, or what its composite exposes of it.
        if(auto* composite = qobject_cast<AppForge::Composite*>(component))
        {
            connect(composite, &AppForge::Composite::exposedPropertiesChanged, this, &PropertyEditor::rebuild);
        }
    }
    if(m_composite != nullptr)
    {
        connect(m_composite, &AppForge::Composite::exposedPropertiesChanged, this, &PropertyEditor::rebuild);
    }
    rebuild();
}

void PropertyEditor::refreshValues()
{
    const QSignalBlocker blocker(m_table);
    for(int row = 0; row < m_table->rowCount(); ++row)
    {
        const Row& shown = m_rows.at(row);
        if(shown.object != nullptr && shown.property.isValid())
        {
            // The value itself rather than its text: its type is what gives the cell its editor.
            m_table->item(row, ValueColumn)->setData(Qt::EditRole, shown.property.read(shown.object));
        }
    }
}

void PropertyEditor::rebuild()
{
    // The objects shown until now are no longer listened to. Not the component, should it still be the one shown:
    // more than its properties is followed.
    for(const Row& shown : std::as_const(m_rows))
    {
        if(shown.object != nullptr && shown.object != m_component)
        {
            shown.object->disconnect(this);
        }
    }
    m_rows.clear();

    // Only what can be edited here: the other properties of a component are in the viewer of its component.
    const auto append = [this](QObject* object, const QList<QMetaProperty>& properties)
    {
        for(const QMetaProperty& property : properties)
        {
            if(isEditable(property))
            {
                m_rows.append(
                    Row{.object = object, .property = property, .title = {}, .component = m_component, .exposed = {}});
            }
        }
    };
    if(auto* composite = qobject_cast<AppForge::Composite*>(m_component.data()))
    {
        // Those it exposes, where they are.
        for(const AppForge::ExposedProperty& exposed : composite->exposedProperties())
        {
            const AppForge::ExposedTarget target = composite->exposedProperty(exposed.name);
            if(target.isValid() && isEditable(target.property))
            {
                m_rows.append(Row{.object = target.object,
                                  .property = target.property,
                                  .title = {},
                                  .component = target.component,
                                  .exposed = exposed.name});
            }
        }
    }
    else if(m_component != nullptr)
    {
        if(const std::optional<AppForge::ComponentInfo> info = m_factory.component(m_component->componentId()))
        {
            append(m_component, info->properties);
        }
        const QList<AppForge::PropertyObject> objects = m_component->propertyObjects();
        // No name first, which follows the properties of the component, then each name once, where it first came.
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
            const qsizetype title = m_rows.size();
            for(const AppForge::PropertyObject& added : objects)
            {
                if(added.name == name)
                {
                    append(added.object, added.properties());
                }
            }
            // No title for a group with nothing to edit.
            if(!name.isEmpty() && m_rows.size() > title)
            {
                m_rows.insert(title, Row{.object = nullptr, .property = {}, .title = name});
            }
        }
    }

    const QSignalBlocker blocker(m_table); // itemChanged is for the values edited in the table
    m_table->setRowCount(0);               // Closes the editor of a value shown until now
    m_table->clearSpans();
    m_table->setRowCount(static_cast<int>(m_rows.size()));
    const QMetaMethod refresh = metaObject()->method(metaObject()->indexOfSlot("refreshValues()"));
    for(int row = 0; row < m_table->rowCount(); ++row)
    {
        const Row& shown = m_rows.at(row);
        if(!shown.property.isValid())
        {
            auto* title = new QTableWidgetItem(shown.title);
            title->setFlags(Qt::ItemIsEnabled);
            QFont font = title->font();
            font.setBold(true);
            title->setFont(font);
            title->setBackground(palette().alternateBase());
            m_table->setItem(row, NameColumn, title);
            m_table->setSpan(row, NameColumn, 1, ColumnCount);
            continue;
        }

        const QString property = QString::fromLatin1(shown.property.name());
        auto* name = new QTableWidgetItem(shown.exposed.isEmpty() ? property : shown.exposed);
        name->setFlags(Qt::ItemIsEnabled);
        name->setToolTip(QString::fromLatin1(shown.property.typeName()));
        if(!shown.exposed.isEmpty() && shown.component != nullptr)
        {
            // Where it is: in the composite shown, from it.
            name->setToolTip(
                tr("%1\n%2 of %3")
                    .arg(name->toolTip(), property,
                         m_application.path(*shown.component).mid(m_application.path(*m_component).size() + 1)));
        }
        // One its composite exposes, with what as: in the scene of the composite.
        const QString exposedAs =
            m_composite != nullptr
                ? m_application.exposedName(*m_component,
                                            shown.exposed.isEmpty() ? shown.object.data() : m_component.data(),
                                            shown.exposed.isEmpty() ? property : shown.exposed)
                : QString();
        if(!exposedAs.isEmpty())
        {
            name->setText(QStringLiteral("%1 → %2").arg(name->text(), exposedAs));
            name->setToolTip(tr("%1\nExposed by %2 as %3").arg(name->toolTip(), m_composite->objectName(), exposedAs));
        }
        const AppForge::SettingsEntry entry{.component = shown.component, .object = shown.object, .property = property};
        if(const AppForge::SettingsFile* file = m_application.settingsFile(entry))
        {
            QFont font = name->font();
            font.setItalic(true);
            name->setFont(font);
            name->setToolTip(tr("%1\nDefault value: the user keeps the one they give it in %2")
                                 .arg(name->toolTip(), file->fileName()));
        }
        m_table->setItem(row, NameColumn, name);
        m_table->setItem(row, ValueColumn, new QTableWidgetItem); // Editable, as an item is by default

        if(shown.property.hasNotifySignal())
        {
            connect(shown.object, shown.property.notifySignal(), this, refresh, Qt::UniqueConnection);
        }
    }
    refreshValues();

    refreshPlaceholder();
    m_stack->setCurrentWidget(m_rows.isEmpty() ? static_cast<QWidget*>(m_placeholder) : m_table);
}

void PropertyEditor::refreshPlaceholder()
{
    m_placeholder->setText(m_component == nullptr ? tr("Select a component of the application to edit its properties")
                                                  : tr("%1 has no editable property").arg(m_component->objectName()));
}

void PropertyEditor::write(const QTableWidgetItem* item)
{
    const Row& edited = m_rows.at(item->row());
    if(edited.object == nullptr || item->column() != ValueColumn)
    {
        return;
    }
    edited.property.write(edited.object, item->data(Qt::EditRole));
    refreshValues(); // What the object made of it: it can adjust a value, or refuse it
}

void PropertyEditor::showMenu(const QPoint& position)
{
    const int row = m_table->rowAt(position.y());
    // In the scene of a composite only, which exposes the properties of its components.
    if(row < 0 || m_composite == nullptr || m_component == nullptr || !m_rows.at(row).property.isValid())
    {
        return;
    }
    const Row& shown = m_rows.at(row);
    // A property of the component, or of one of its objects; one a composite exposes in turn, as it exposes it.
    QObject* object = shown.exposed.isEmpty() ? shown.object.data() : m_component.data();
    const QString property = shown.exposed.isEmpty() ? QString::fromLatin1(shown.property.name()) : shown.exposed;
    const QString exposedAs = m_application.exposedName(*m_component, object, property);

    QMenu menu(this);
    QAction* expose = menu.addAction(tr("Expose"));
    expose->setEnabled(exposedAs.isEmpty());
    QAction* unexpose = menu.addAction(tr("Unexpose"));
    unexpose->setEnabled(!exposedAs.isEmpty());
    QAction* rename = menu.addAction(tr("Rename exposed property..."));
    rename->setEnabled(!exposedAs.isEmpty());
    const QAction* chosen = menu.exec(m_table->viewport()->mapToGlobal(position));
    // What the menu was about can be gone once it is closed.
    if(chosen == nullptr || m_composite == nullptr || m_component == nullptr)
    {
        return;
    }
    if(chosen == expose)
    {
        if(m_application.exposeProperty(*m_component, object, property).isEmpty())
        {
            QMessageBox::warning(this, tr("Expose"), m_application.errorString());
        }
    }
    else if(chosen == unexpose)
    {
        m_application.unexposeProperty(*m_composite, exposedAs);
    }
    else if(chosen == rename)
    {
        const QString newName = QInputDialog::getText(
            this, tr("Rename exposed property"), tr("Name %1 exposes %2 as:").arg(m_composite->objectName(), property),
            QLineEdit::Normal, exposedAs);
        if(!newName.isEmpty() && newName != exposedAs && m_composite != nullptr &&
           !m_application.renameExposedProperty(*m_composite, exposedAs, newName))
        {
            QMessageBox::warning(this, tr("Rename exposed property"),
                                 tr("%1 is not renamed: %2 exposes another property as %3")
                                     .arg(exposedAs, m_composite->objectName(), newName));
        }
    }
}
