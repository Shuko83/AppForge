#pragma once

#include <QAbstractTableModel>
#include <QList>
#include <cstdint>

#include "Plugin/PluginInfo.h"

namespace AppForge
{
class PluginManager;
}

// The plugins found by a PluginManager, one row per plugin, sorted by id.
class PluginModel : public QAbstractTableModel
{
    Q_OBJECT

  public:
    enum Column : std::uint8_t
    {
        Name,
        Version,
        State,
        ColumnCount,
    };

    // The plugin id, in every column.
    static constexpr int IdRole = Qt::UserRole;

    explicit PluginModel(const AppForge::PluginManager& plugins, QObject* parent = nullptr);

    [[nodiscard]] int rowCount(const QModelIndex& parent = {}) const override;
    [[nodiscard]] int columnCount(const QModelIndex& parent = {}) const override;
    [[nodiscard]] QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    [[nodiscard]] QVariant headerData(int section, Qt::Orientation orientation,
                                      int role = Qt::DisplayRole) const override;

    // The first column of pluginId; invalid when it is not listed.
    [[nodiscard]] QModelIndex indexOf(const QString& pluginId) const;

    // Reads the plugins of the manager again: inserts the ones found since, and updates the others, e.g. once loaded.
    void refresh();

  private:
    const AppForge::PluginManager& m_plugins;
    QList<AppForge::PluginInfo> m_infos; // Sorted by id, as PluginManager::plugins()
};
