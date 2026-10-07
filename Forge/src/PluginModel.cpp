#include "PluginModel.h"

#include <algorithm>

#include "Plugin/PluginManager.h"

PluginModel::PluginModel(const AppForge::PluginManager& plugins, QObject* parent)
    : QAbstractTableModel(parent), m_plugins(plugins)
{
}

int PluginModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(m_infos.size());
}

int PluginModel::columnCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : ColumnCount;
}

QVariant PluginModel::data(const QModelIndex& index, int role) const
{
    if(!checkIndex(index, CheckIndexOption::IndexIsValid | CheckIndexOption::ParentIsInvalid))
    {
        return {};
    }
    const AppForge::PluginInfo& info = m_infos.at(index.row());
    if(role == IdRole || role == Qt::ToolTipRole)
    {
        return info.id;
    }
    if(role != Qt::DisplayRole)
    {
        return {};
    }
    switch(index.column())
    {
    case Name:
        return info.name;
    case Version:
        return info.version.toString();
    case State:
        return m_plugins.isLoaded(info.id) ? tr("Loaded") : tr("Not loaded");
    default:
        return {};
    }
}

QVariant PluginModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(orientation != Qt::Horizontal || role != Qt::DisplayRole)
    {
        return {};
    }
    switch(section)
    {
    case Name:
        return tr("Name");
    case Version:
        return tr("Version");
    case State:
        return tr("State");
    default:
        return {};
    }
}

QModelIndex PluginModel::indexOf(const QString& pluginId) const
{
    const auto info = std::ranges::find(m_infos, pluginId, &AppForge::PluginInfo::id);
    return info == m_infos.end() ? QModelIndex() : index(static_cast<int>(info - m_infos.begin()), 0);
}

void PluginModel::refresh()
{
    // PluginManager only adds plugins, and lists them by id: the new ones are inserted where they belong.
    const QList<AppForge::PluginInfo> plugins = m_plugins.plugins();
    for(qsizetype row = 0; row < plugins.size(); ++row)
    {
        if(row < m_infos.size() && m_infos.at(row).id == plugins.at(row).id)
        {
            m_infos[row] = plugins.at(row);
            continue;
        }
        beginInsertRows({}, static_cast<int>(row), static_cast<int>(row));
        m_infos.insert(row, plugins.at(row));
        endInsertRows();
    }
    if(!m_infos.isEmpty())
    {
        emit dataChanged(index(0, 0), index(rowCount() - 1, ColumnCount - 1));
    }
}
