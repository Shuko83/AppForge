#include "PropertyEditor.h"

#include <QHeaderView>
#include <QLabel>
#include <QMetaMethod>
#include <QSignalBlocker>
#include <QStackedWidget>
#include <QTableWidget>
#include <QVBoxLayout>
#include <algorithm>
#include <cstdint>
#include <iterator>
#include <optional>

#include "Component/ComponentFactory.h"

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

PropertyEditor::PropertyEditor(const AppForge::ComponentFactory& factory, QWidget* parent)
    : QWidget(parent), m_factory(factory)
{
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
    const std::optional<AppForge::ComponentInfo> info =
        component != nullptr ? m_factory.component(component->componentId()) : std::nullopt;
    // Only what can be edited here: the others are in the viewer of its component.
    m_properties.clear();
    if(info)
    {
        std::ranges::copy_if(info->properties, std::back_inserter(m_properties), isEditable);
    }

    const QSignalBlocker blocker(m_table); // itemChanged is for the values edited in the table
    m_table->setRowCount(0);               // Closes the editor of a value of the previous component
    m_table->setRowCount(static_cast<int>(m_properties.size()));
    const QMetaMethod refresh = metaObject()->method(metaObject()->indexOfSlot("refreshValues()"));
    for(int row = 0; row < m_table->rowCount(); ++row)
    {
        const QMetaProperty& property = m_properties.at(row);

        auto* name = new QTableWidgetItem(QString::fromLatin1(property.name()));
        name->setFlags(Qt::ItemIsEnabled);
        name->setToolTip(QString::fromLatin1(property.typeName()));
        m_table->setItem(row, NameColumn, name);
        m_table->setItem(row, ValueColumn, new QTableWidgetItem); // Editable, as an item is by default

        if(property.hasNotifySignal())
        {
            connect(component, property.notifySignal(), this, refresh, Qt::UniqueConnection);
        }
    }
    refreshValues();

    m_placeholder->setText(component == nullptr ? tr("Select a component of the application to edit its properties")
                                                : tr("%1 has no editable property").arg(component->objectName()));
    m_stack->setCurrentWidget(m_properties.isEmpty() ? static_cast<QWidget*>(m_placeholder) : m_table);
}

void PropertyEditor::refreshValues()
{
    if(m_component == nullptr)
    {
        return;
    }
    const QSignalBlocker blocker(m_table);
    for(int row = 0; row < m_table->rowCount(); ++row)
    {
        // The value itself rather than its text: its type is what gives the cell its editor.
        m_table->item(row, ValueColumn)->setData(Qt::EditRole, m_properties.at(row).read(m_component));
    }
}

void PropertyEditor::write(const QTableWidgetItem* item)
{
    if(m_component == nullptr || item->column() != ValueColumn)
    {
        return;
    }
    m_properties.at(item->row()).write(m_component, item->data(Qt::EditRole));
    refreshValues(); // What the component made of it: it can adjust a value, or refuse it
}
