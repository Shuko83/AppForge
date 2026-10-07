#include "ComponentModel.h"

#include "Component/ComponentFactory.h"

ComponentModel::ComponentModel(const AppForge::ComponentFactory& factory, QObject* parent)
    : QAbstractTableModel(parent), m_factory(factory)
{
}

int ComponentModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(m_components.size());
}

int ComponentModel::columnCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : ColumnCount;
}

QVariant ComponentModel::data(const QModelIndex& index, int role) const
{
    if(!checkIndex(index, CheckIndexOption::IndexIsValid | CheckIndexOption::ParentIsInvalid))
    {
        return {};
    }
    const AppForge::ComponentInfo& component = m_components.at(index.row());
    if(role == IdRole)
    {
        return component.id;
    }
    if(role == Qt::ToolTipRole)
    {
        return component.description;
    }
    if(role != Qt::DisplayRole)
    {
        return {};
    }
    switch(index.column())
    {
    case Name:
        return component.name;
    case Category:
        return component.category;
    case Plugin:
        return component.pluginId;
    default:
        return {};
    }
}

QVariant ComponentModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(orientation != Qt::Horizontal || role != Qt::DisplayRole)
    {
        return {};
    }
    switch(section)
    {
    case Name:
        return tr("Name");
    case Category:
        return tr("Category");
    case Plugin:
        return tr("Plugin");
    default:
        return {};
    }
}

void ComponentModel::refresh()
{
    // ComponentFactory only adds components, and lists them by id: the new ones are inserted where they belong.
    const QList<AppForge::ComponentInfo> components = m_factory.components();
    for(qsizetype row = 0; row < components.size(); ++row)
    {
        if(row < m_components.size() && m_components.at(row).id == components.at(row).id)
        {
            continue;
        }
        beginInsertRows({}, static_cast<int>(row), static_cast<int>(row));
        m_components.insert(row, components.at(row));
        endInsertRows();
    }
}
