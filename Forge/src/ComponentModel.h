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
    // Format of the component id, in UTF-8, in the data dragged from the model.
    static constexpr QLatin1StringView MimeType{"application/x-appforge-component"};

    explicit ComponentModel(const AppForge::ComponentFactory& factory, QObject* parent = nullptr);

    [[nodiscard]] int rowCount(const QModelIndex& parent = {}) const override;
    [[nodiscard]] int columnCount(const QModelIndex& parent = {}) const override;
    [[nodiscard]] QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    [[nodiscard]] QVariant headerData(int section, Qt::Orientation orientation,
                                      int role = Qt::DisplayRole) const override;

    // A component can be dragged, to be instantiated where it is dropped: see EditionScene.
    [[nodiscard]] Qt::ItemFlags flags(const QModelIndex& index) const override;
    [[nodiscard]] QStringList mimeTypes() const override;
    [[nodiscard]] QMimeData* mimeData(const QModelIndexList& indexes) const override;

    // Reads the components of the factory again: inserts the ones registered since, e.g. by a plugin just loaded.
    void refresh();

  private:
    const AppForge::ComponentFactory& m_factory;
    QList<AppForge::ComponentInfo> m_components; // Sorted by id, as ComponentFactory::components()
};
