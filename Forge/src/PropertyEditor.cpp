// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "PropertyEditor.h"

#include <QHeaderView>
#include <QLabel>
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

PropertyEditor::PropertyEditor(const AppForge::ForgeApplication& application, const AppForge::ComponentFactory& factory,
                               QWidget* parent)
    : QWidget(parent), m_application(application), m_factory(factory)
{
    // Which properties are in italics, and whether they are edited.
    connect(&m_application, &AppForge::ForgeApplication::settingsFilesChanged, this, &PropertyEditor::rebuild);
    connect(&m_application, &AppForge::ForgeApplication::runningChanged, this, &PropertyEditor::rebuild);

    m_placeholder = new QLabel;
    m_placeholder->setAlignment(Qt::AlignCenter);

    m_table = new QTableWidget(0, ColumnCount);
    m_table->setHorizontalHeaderLabels({tr("Property"), tr("Value")});
    m_table->verticalHeader()->hide();
    m_table->horizontalHeader()->setSectionResizeMode(NameColumn, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(m_table, &QTableWidget::itemChanged, this, &PropertyEditor::write);

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
    m_component = component;
    if(component != nullptr)
    {
        connect(component, &QObject::objectNameChanged, this, &PropertyEditor::refreshPlaceholder);
        connect(component, &AppForge::Component::propertyObjectsChanged, this, &PropertyEditor::rebuild);
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
                m_rows.append(Row{.object = object, .property = property, .title = {}});
            }
        }
    };
    if(m_component != nullptr)
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
    // A click on a value edits it, while the application is edited.
    m_table->setEditTriggers(m_application.isRunning() ? QAbstractItemView::NoEditTriggers
                                                       : QAbstractItemView::AllEditTriggers);
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

        auto* name = new QTableWidgetItem(QString::fromLatin1(shown.property.name()));
        name->setFlags(Qt::ItemIsEnabled);
        name->setToolTip(QString::fromLatin1(shown.property.typeName()));
        const AppForge::SettingsEntry entry{.component = m_component, .object = shown.object, .property = name->text()};
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
