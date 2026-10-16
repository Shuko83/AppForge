// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QAbstractTableModel>
#include <QList>
#include <QStringList>
#include <cstdint>

#include "AppForge/Component/ComponentInfo.h"

namespace AppForge
{
class ComponentFactory;
}

// The components registered in a ComponentFactory, one row per component, sorted by id, but those Core provides, then
// composite files, sorted by name: each is listed under the name of its file, in the category Composite. The
// description of a component, and the file of a composite, is the tool tip of its row, in every column.
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

    // The component id, in every column; empty for a composite.
    static constexpr int IdRole = Qt::UserRole;
    // The file of a composite, absolute, in every column; empty for a component.
    static constexpr int FileRole = Qt::UserRole + 1;
    // Format of the component id, in UTF-8, in the data dragged from the model.
    static constexpr QLatin1StringView MimeType{"application/x-appforge-component"};
    // Format of the file of a composite, in UTF-8, in the data dragged from the model.
    static constexpr QLatin1StringView CompositeMimeType{"application/x-appforge-composite"};

    explicit ComponentModel(const AppForge::ComponentFactory& factory, QObject* parent = nullptr);

    [[nodiscard]] int rowCount(const QModelIndex& parent = {}) const override;
    [[nodiscard]] int columnCount(const QModelIndex& parent = {}) const override;
    [[nodiscard]] QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    [[nodiscard]] QVariant headerData(int section, Qt::Orientation orientation,
                                      int role = Qt::DisplayRole) const override;

    // A component, or a composite, can be dragged, to be instantiated where it is dropped: see EditionScene.
    [[nodiscard]] Qt::ItemFlags flags(const QModelIndex& index) const override;
    [[nodiscard]] QStringList mimeTypes() const override;
    [[nodiscard]] QMimeData* mimeData(const QModelIndexList& indexes) const override;

    // Reads the components of the factory again: inserts the ones registered since, e.g. by a plugin just loaded, and
    // removes the ones it no longer has, those of a plugin just unloaded.
    void refresh();
    // Lists files, absolute, as the composites: inserts those it did not list, removes those it no longer has.
    void setCompositeFiles(const QStringList& files);

  private:
    const AppForge::ComponentFactory& m_factory;
    QList<AppForge::ComponentInfo> m_components; // Sorted by id, as ComponentFactory::components()
    QStringList m_composites;                    // After them, sorted by name
};
