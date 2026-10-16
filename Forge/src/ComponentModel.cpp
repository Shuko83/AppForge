// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "ComponentModel.h"

#include <QDir>
#include <QFileInfo>
#include <QMimeData>
#include <algorithm>

#include "AppForge/Component/ComponentFactory.h"

namespace
{

// The category of the composites, the one Composite declares.
const QString CompositeCategory = QStringLiteral("Composite");

// Whether the composite file first is listed before second: by name, then by directory.
bool isBefore(const QString& first, const QString& second)
{
    const int byName =
        QFileInfo(first).completeBaseName().compare(QFileInfo(second).completeBaseName(), Qt::CaseInsensitive);
    return byName != 0 ? byName < 0 : first < second;
}

} // namespace

ComponentModel::ComponentModel(const AppForge::ComponentFactory& factory, QObject* parent)
    : QAbstractTableModel(parent), m_factory(factory)
{
}

int ComponentModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(m_components.size() + m_composites.size());
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
    if(index.row() >= m_components.size())
    {
        const QString& file = m_composites.at(index.row() - m_components.size());
        switch(role)
        {
        case FileRole:
            return file;
        case Qt::ToolTipRole:
            return QDir::toNativeSeparators(file);
        case Qt::DisplayRole:
            if(index.column() == Name)
            {
                return QFileInfo(file).completeBaseName();
            }
            return index.column() == Category ? CompositeCategory : QVariant();
        default:
            return {};
        }
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

Qt::ItemFlags ComponentModel::flags(const QModelIndex& index) const
{
    const Qt::ItemFlags flags = QAbstractTableModel::flags(index);
    return index.isValid() ? flags | Qt::ItemIsDragEnabled : flags;
}

QStringList ComponentModel::mimeTypes() const
{
    return {MimeType, CompositeMimeType};
}

QMimeData* ComponentModel::mimeData(const QModelIndexList& indexes) const
{
    // The view selects a single row, one index per column: they all hold the same id, or file.
    if(indexes.isEmpty())
    {
        return nullptr;
    }
    auto* data = new QMimeData;
    const QString file = indexes.first().data(FileRole).toString();
    if(file.isEmpty())
    {
        data->setData(MimeType, indexes.first().data(IdRole).toString().toUtf8());
    }
    else
    {
        data->setData(CompositeMimeType, file.toUtf8());
    }
    return data;
}

void ComponentModel::refresh()
{
    QList<AppForge::ComponentInfo> components = m_factory.components();
    // Not those Core provides: a composite is made from its file.
    components.removeIf([](const AppForge::ComponentInfo& component)
                        { return component.pluginId == AppForge::ComponentFactory::corePluginId(); });
    // Those of a plugin unloaded since, the last one first: the rows above it keep their number.
    for(qsizetype row = m_components.size() - 1; row >= 0; --row)
    {
        if(std::ranges::find(components, m_components.at(row).id, &AppForge::ComponentInfo::id) == components.end())
        {
            beginRemoveRows({}, static_cast<int>(row), static_cast<int>(row));
            m_components.removeAt(row);
            endRemoveRows();
        }
    }
    // ComponentFactory lists them by id: the new ones are inserted where they belong.
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

void ComponentModel::setCompositeFiles(const QStringList& files)
{
    QStringList sorted = files;
    sorted.removeDuplicates();
    std::ranges::sort(sorted, isBefore);
    // As refresh() does, after the rows of the components.
    const auto first = static_cast<int>(m_components.size());
    for(qsizetype index = m_composites.size() - 1; index >= 0; --index)
    {
        if(!sorted.contains(m_composites.at(index)))
        {
            beginRemoveRows({}, first + static_cast<int>(index), first + static_cast<int>(index));
            m_composites.removeAt(index);
            endRemoveRows();
        }
    }
    for(qsizetype index = 0; index < sorted.size(); ++index)
    {
        if(index < m_composites.size() && m_composites.at(index) == sorted.at(index))
        {
            continue;
        }
        beginInsertRows({}, first + static_cast<int>(index), first + static_cast<int>(index));
        m_composites.insert(index, sorted.at(index));
        endInsertRows();
    }
}
