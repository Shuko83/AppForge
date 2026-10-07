#include "ComponentViewer.h"

#include <QFormLayout>
#include <QHeaderView>
#include <QLabel>
#include <QStackedWidget>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <optional>

#include "Component/ComponentFactory.h"

namespace
{

// A row of form whose value can be selected and wraps.
QLabel* addField(QFormLayout& form, const QString& name)
{
    auto* value = new QLabel;
    value->setTextInteractionFlags(Qt::TextSelectableByMouse);
    value->setWordWrap(true);
    form.addRow(name, value);
    return value;
}

} // namespace

ComponentViewer::ComponentViewer(const AppForge::ComponentFactory& factory, QWidget* parent)
    : QWidget(parent), m_factory(factory)
{
    auto* placeholder = new QLabel(tr("Select a component"));
    placeholder->setAlignment(Qt::AlignCenter);
    m_placeholder = placeholder;

    m_page = new QWidget;
    auto* form = new QFormLayout(m_page);
    m_name = addField(*form, tr("Name"));
    m_id = addField(*form, tr("Id"));
    m_plugin = addField(*form, tr("Plugin"));
    m_category = addField(*form, tr("Category"));
    m_description = addField(*form, tr("Description"));
    m_properties = new QTreeWidget;
    m_properties->setHeaderLabels({tr("Name"), tr("Type"), tr("Access")});
    m_properties->setRootIsDecorated(false);
    m_properties->setUniformRowHeights(true);
    m_properties->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    form->addRow(tr("Properties"), m_properties);

    m_stack = new QStackedWidget;
    m_stack->addWidget(m_placeholder);
    m_stack->addWidget(m_page);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins({});
    layout->addWidget(m_stack);
}

void ComponentViewer::setComponentId(const QString& componentId)
{
    const std::optional<AppForge::ComponentInfo> component = m_factory.component(componentId);
    if(!component)
    {
        m_stack->setCurrentWidget(m_placeholder);
        return;
    }
    m_name->setText(component->name);
    m_id->setText(component->id);
    m_plugin->setText(component->pluginId);
    m_category->setText(component->category);
    m_description->setText(component->description);
    m_properties->clear();
    for(const QMetaProperty& property : component->properties)
    {
        m_properties->addTopLevelItem(new QTreeWidgetItem(
            {QString::fromLatin1(property.name()), QString::fromLatin1(property.typeName()),
             property.isWritable() ? tr("Read/write") : tr("Read-only")}));
    }
    m_stack->setCurrentWidget(m_page);
}
