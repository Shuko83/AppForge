// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "NameEditor.h"

#include <QKeyEvent>
#include <QTextCursor>
#include <QTextDocument>
#include <utility>

#include "Theme.h"

NameEditor::NameEditor(QGraphicsItem& parent, std::function<void(const QString&)> named)
    : QGraphicsTextItem(&parent), m_named(std::move(named))
{
    setFont(titleFont(font())); // The one the name is painted in
    setTextInteractionFlags(Qt::TextEditorInteraction);
    hide(); // Until a name is edited
    connect(document(), &QTextDocument::contentsChanged, this, [this] { place(); });
}

void NameEditor::edit(const QString& name, const QColor& color, const QRectF& rect, Qt::Alignment alignment)
{
    m_rect = rect;
    m_alignment = alignment;
    setDefaultTextColor(color);
    setPlainText(name);
    place();
    show();
    setFocus();
    QTextCursor cursor = textCursor();
    cursor.select(QTextCursor::Document); // What is typed replaces it
    setTextCursor(cursor);
    parentItem()->update(); // Its name is no longer painted
}

void NameEditor::keyPressEvent(QKeyEvent* event)
{
    switch(event->key())
    {
    case Qt::Key_Escape:
        end(false);
        break;
    case Qt::Key_Return:
    case Qt::Key_Enter:
        end(true);
        break;
    default:
        QGraphicsTextItem::keyPressEvent(event);
    }
}

void NameEditor::focusOutEvent(QFocusEvent* event)
{
    QGraphicsTextItem::focusOutEvent(event);
    end(true);
}

void NameEditor::place()
{
    // Its text is inside the margin of its document.
    const QSizeF size = document()->size();
    const qreal left = m_alignment.testFlag(Qt::AlignHCenter) ? m_rect.center().x() - size.width() / 2
                                                              : m_rect.left() - document()->documentMargin();
    setPos(left, m_rect.center().y() - size.height() / 2);
}

void NameEditor::end(bool named)
{
    if(!isVisible())
    {
        return; // Ended already: hiding it takes its focus, which ends it too
    }
    hide();
    parentItem()->update(); // Its name is painted again
    if(named)
    {
        m_named(toPlainText().simplified()); // On one line, whatever was pasted
    }
}
