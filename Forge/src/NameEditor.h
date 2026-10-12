#pragma once

#include <QGraphicsTextItem>
#include <QRectF>
#include <QString>
#include <functional>

// What an item is named, edited in place: a child of the item, shown over where it paints its name, in bold as it is
// painted there, while it is edited. The edition lasts until Enter is pressed or anything else is clicked; Escape
// gives it up.
class NameEditor : public QGraphicsTextItem
{
  public:
    // named is called with what was typed, on one line, when an edition ends without being given up.
    NameEditor(QGraphicsItem& parent, std::function<void(const QString&)> named);

    // Edits name, written in color where its item paints it: in rect, in the middle of its height, from its left or,
    // with Qt::AlignHCenter, around its center. Its item, which must be in a scene, no longer paints its name until
    // the edition ends: it is updated then, as it is now.
    void edit(const QString& name, const QColor& color, const QRectF& rect, Qt::Alignment alignment);

  protected:
    void keyPressEvent(QKeyEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;

  private:
    // Puts what is typed where the name is painted, whatever its length.
    void place();
    // Ends the edition; named tells whether what was typed is the name, rather than given up.
    void end(bool named);

    std::function<void(const QString&)> m_named;
    QRectF m_rect; // Where the name is painted, in its item
    Qt::Alignment m_alignment;
};
