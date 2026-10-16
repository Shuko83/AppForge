// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "ComponentViewer.h"

#include <QDir>
#include <QFileInfo>
#include <QFormLayout>
#include <QHeaderView>
#include <QJsonArray>
#include <QLabel>
#include <QStackedWidget>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <optional>

#include "AppForge/Component/ComponentFactory.h"

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
    m_form = new QFormLayout(m_page);
    m_name = addField(*m_form, tr("Name"));
    m_id = addField(*m_form, tr("Id"));
    m_plugin = addField(*m_form, tr("Plugin"));
    m_category = addField(*m_form, tr("Category"));
    m_description = addField(*m_form, tr("Description"));
    m_properties = new QTreeWidget;
    m_properties->setRootIsDecorated(false);
    m_properties->setUniformRowHeights(true);
    m_properties->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_form->addRow(tr("Properties"), m_properties);

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
    showProperties(true);
    for(const QMetaProperty& property : component->properties)
    {
        m_properties->addTopLevelItem(
            new QTreeWidgetItem({QString::fromLatin1(property.name()), QString::fromLatin1(property.typeName()),
                                 property.isWritable() ? tr("Read/write") : tr("Read-only")}));
    }
    m_stack->setCurrentWidget(m_page);
}

void ComponentViewer::setComposite(const QString& filePath, const QJsonObject& document)
{
    m_name->setText(QFileInfo(filePath).completeBaseName());
    m_id->setText(QDir::toNativeSeparators(filePath));
    QStringList plugins;
    const QJsonArray pluginEntries = document.value(u"plugins").toArray();
    for(const QJsonValue& plugin : pluginEntries)
    {
        plugins.append(plugin.toObject().value(u"id").toString());
    }
    m_plugin->setText(plugins.join(u'\n'));
    m_category->setText(QStringLiteral("Composite"));
    m_description->setText(tr("A composite file: its components are placed together, as a component is"));
    showProperties(false);
    const QJsonArray components = document.value(u"components").toArray();
    for(const QJsonValue& value : components)
    {
        const QJsonObject component = value.toObject();
        // What it is an instance of: the name of its component, after the id of its plugin, or its composite file.
        QString type = component.value(u"type").toString().section(u'.', -1);
        if(component.contains(u"composite"))
        {
            type = QFileInfo(component.value(u"composite").toString()).fileName();
        }
        m_properties->addTopLevelItem(new QTreeWidgetItem({component.value(u"name").toString(), type}));
    }
    m_stack->setCurrentWidget(m_page);
}

void ComponentViewer::showProperties(bool properties)
{
    m_properties->clear();
    m_properties->setHeaderLabels(properties ? QStringList{tr("Name"), tr("Type"), tr("Access")}
                                             : QStringList{tr("Name"), tr("Type")});
    if(auto* label = qobject_cast<QLabel*>(m_form->labelForField(m_properties)))
    {
        label->setText(properties ? tr("Properties") : tr("Components"));
    }
}
