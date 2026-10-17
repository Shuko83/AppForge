// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QGraphicsView>

// Shows the edition zone, at a zoom level that Ctrl and the mouse wheel change around the point under the cursor, and
// the + and - keys around the middle of the view, unless the name of an item is edited there: what is typed is then
// for it. The zoom only changes what is seen, not the scene: it is not saved with the project.
class EditionView : public QGraphicsView
{
    Q_OBJECT

  public:
    explicit EditionView(QGraphicsScene* scene, QWidget* parent = nullptr);

    // How much the scene is enlarged: 1 shows it at its own size.
    [[nodiscard]] qreal zoom() const;
    // Shows the scene enlarged by zoom, kept between the smallest and the largest zoom levels, around the middle of the
    // view.
    void setZoom(qreal zoom);
    // One step closer, or further, around the middle of the view.
    void zoomIn();
    void zoomOut();
    // Shows the scene at its own size.
    void resetZoom();

  signals:
    void zoomChanged(qreal zoom);

  protected:
    void wheelEvent(QWheelEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
};
