#pragma once

#include <QAbstractTableModel>
#include <QList>
#include <cstdint>

#include "Component/ComponentInfo.h"

namespace AppForge
{
class ComponentFactory;
}

// The components registered in a ComponentFactory, one row per component, sorted by id.
class ComponentModel : public QAbstractTableModel
{
    Q_OBJECT

  public:
    enum Column : std::uint8_t
    {
        Name,
        Category,
        Plugin,
        ColumnCount,
    };

    // The component id, in every column.
    static constexpr int IdRole = Qt::UserRole;

    explicit ComponentModel(const AppForge::ComponentFactory& factory, QObject* parent = nullptr);

    [[nodiscard]] int rowCount(const QModelIndex& parent = {}) const override;
    [[nodiscard]] int columnCount(const QModelIndex& parent = {}) const override;
    [[nodiscard]] QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    [[nodiscard]] QVariant headerData(int section, Qt::Orientation orientation,
                                      int role = Qt::DisplayRole) const override;

    // Reads the components of the factory again: inserts the ones registered since, e.g. by a plugin just loaded.
    void refresh();

  private:
    const AppForge::ComponentFactory& m_factory;
    QList<AppForge::ComponentInfo> m_components; // Sorted by id, as ComponentFactory::components()
};
