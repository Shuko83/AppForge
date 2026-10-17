// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "ProblemList.h"

#include <QHeaderView>
#include <QIcon>
#include <QLabel>
#include <QMetaObject>
#include <QStackedWidget>
#include <QStringList>
#include <QStyle>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <cstdint>
#include <memory>

#include "InterfaceItem.h"
#include "Nebula/Application/Universe.h"
#include "Nebula/Component/Component.h"

namespace
{

enum Column : std::uint8_t
{
    ComponentColumn,
    ProblemColumn,
    FixColumn,
    ColumnCount,
};

// The interfaces of type the components of application provide, other than consumer: <component>.<interface> each.
QStringList providersOf(const Nebula::Universe& application, QMetaType type, const Nebula::Component& consumer)
{
    QStringList providers;
    for(const std::unique_ptr<Nebula::Component>& component : application.components())
    {
        if(component.get() == &consumer)
        {
            continue;
        }
        const QList<Nebula::InterfaceInfo> provided = component->providedInterfaces();
        for(const Nebula::InterfaceInfo& interface : provided)
        {
            if(interface.type == type)
            {
                providers.append(QStringLiteral("%1.%2").arg(component->objectName(), interface.name));
            }
        }
    }
    return providers;
}

} // namespace

ProblemList::ProblemList(const Nebula::Universe& application, QWidget* parent)
    : QWidget(parent), m_application(application)
{
    connect(&m_application, &Nebula::Universe::componentsChanged, this, &ProblemList::scheduleRefresh);

    m_placeholder = new QLabel(tr("Every component of the application has the interfaces it needs"));
    m_placeholder->setAlignment(Qt::AlignCenter);
    m_placeholder->setForegroundRole(QPalette::PlaceholderText);

    m_tree = new QTreeWidget;
    m_tree->setColumnCount(ColumnCount);
    m_tree->setHeaderLabels({tr("Component"), tr("Problem"), tr("How to fix it")});
    m_tree->setRootIsDecorated(false);
    m_tree->setAllColumnsShowFocus(true);
    m_tree->header()->setStretchLastSection(true);
    m_tree->header()->setSectionResizeMode(ComponentColumn, QHeaderView::ResizeToContents);
    m_tree->header()->setSectionResizeMode(ProblemColumn, QHeaderView::ResizeToContents);
    connect(m_tree, &QTreeWidget::itemActivated, this,
            [this](const QTreeWidgetItem* item) { emit componentActivated(item->text(ComponentColumn)); });

    m_stack = new QStackedWidget;
    m_stack->addWidget(m_placeholder);
    m_stack->addWidget(m_tree);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins({});
    layout->addWidget(m_stack);

    refresh();
}

void ProblemList::scheduleRefresh()
{
    if(!m_refreshScheduled)
    {
        m_refreshScheduled = true;
        QMetaObject::invokeMethod(this, &ProblemList::refresh, Qt::QueuedConnection);
    }
}

void ProblemList::refresh()
{
    m_refreshScheduled = false;
    m_tree->clear();
    const QIcon warning = style()->standardIcon(QStyle::SP_MessageBoxWarning);
    for(const std::unique_ptr<Nebula::Component>& component : m_application.components())
    {
        // Once each, those already followed included: the ones instantiated since are followed from now on.
        connect(component.get(), &Nebula::Component::interfacesChanged, this, &ProblemList::scheduleRefresh,
                Qt::UniqueConnection);
        connect(component.get(), &QObject::objectNameChanged, this, &ProblemList::scheduleRefresh,
                Qt::UniqueConnection);

        const QStringList unbound = component->unboundMandatoryInterfaces();
        const QList<Nebula::InterfaceInfo> consumed = component->consumedInterfaces();
        for(const Nebula::InterfaceInfo& interface : consumed)
        {
            if(!unbound.contains(interface.name))
            {
                continue;
            }
            const QString type = InterfaceItem::typeName(interface.type);
            const QStringList providers = providersOf(m_application, interface.type, *component);
            QString fix;
            if(providers.isEmpty())
            {
                fix = tr("Add a component that provides a %1, then drag from the dot of %2 to the dot of its %1")
                          .arg(type, interface.name);
            }
            else if(providers.size() == 1)
            {
                fix = tr("Drag from the dot of %1 to the dot of %2").arg(interface.name, providers.constFirst());
            }
            else
            {
                fix = tr("Drag from the dot of %1 to the dot of one of %2")
                          .arg(interface.name, providers.join(QStringLiteral(", ")));
            }
            const QString problem = tr("%1 (%2) is bound to nothing").arg(interface.name, type);

            auto* item = new QTreeWidgetItem(m_tree);
            item->setIcon(ComponentColumn, warning);
            item->setText(ComponentColumn, component->objectName());
            item->setText(ProblemColumn, problem);
            item->setText(FixColumn, fix);
            // The whole of it, should the columns be narrow.
            item->setToolTip(ProblemColumn, tr("%1: the component does not work without it").arg(problem));
            item->setToolTip(FixColumn, fix);
        }
    }
    m_stack->setCurrentWidget(m_tree->topLevelItemCount() > 0 ? static_cast<QWidget*>(m_tree) : m_placeholder);
}
