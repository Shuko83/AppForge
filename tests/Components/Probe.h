// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QDate>
#include <QDateTime>
#include <QPoint>
#include <QString>
#include <QStringList>
#include <QTime>
#include <QTimeZone>
#include <map>

#include "Nebula/Component/Component.h"

class QTimer;

// A component the test drives: it provides and consumes what the test tells it to, with the protected calls of
// Component made public, and logs what Component calls of it, in events(). Its properties are of each type a project
// holds, and of one it does not, point.
class Probe : public Nebula::Component
{
    Q_OBJECT
    Q_CLASSINFO("description", "Provides and consumes what the test tells it to")
    Q_CLASSINFO("category", "Test")
    // Whether onStart() refuses.
    Q_PROPERTY(bool refusesToStart MEMBER m_refusesToStart NOTIFY refusesToStartChanged)
    Q_PROPERTY(int count MEMBER m_count NOTIFY countChanged)
    Q_PROPERTY(double ratio MEMBER m_ratio NOTIFY ratioChanged)
    Q_PROPERTY(QString text MEMBER m_text NOTIFY textChanged)
    Q_PROPERTY(QDate date MEMBER m_date NOTIFY dateChanged)
    Q_PROPERTY(QTime time MEMBER m_time NOTIFY timeChanged)
    Q_PROPERTY(QDateTime moment MEMBER m_moment NOTIFY momentChanged)
    // JSON holds no point: neither a project nor a settings file holds it.
    Q_PROPERTY(QPoint point MEMBER m_point NOTIFY pointChanged)
    // How many times it started: read only, a project does not hold it.
    Q_PROPERTY(int starts READ starts NOTIFY startsChanged)

  public:
    Probe();

    // provideInterface() of a QObject, or of a QTimer.
    bool provideObject(const QString& name, QObject* object = nullptr);
    bool provideTimer(const QString& name, QTimer* timer = nullptr);
    // consumeInterface() of a QObject, or of a QTimer, which consumed() returns: its callbacks log "available <name>"
    // and "unavailable <name>", the latter with " (set)" while the member still has its pointer, as it should.
    bool consumeObject(const QString& name,
                       Nebula::InterfaceRequirement requirement = Nebula::InterfaceRequirement::Optional);
    bool consumeTimer(const QString& name,
                      Nebula::InterfaceRequirement requirement = Nebula::InterfaceRequirement::Optional);
    // The member of name, which it consumes; nullptr while it has no pointer, or when it does not consume name.
    [[nodiscard]] QObject* consumed(const QString& name) const;
    // addPropertiesForQObject().
    void addObject(QObject* object, const QString& name = {});

    [[nodiscard]] int starts() const;
    // What was called since it was built, or since clearEvents(): "start", "stop" and the callbacks of consumeObject()
    // and consumeTimer().
    [[nodiscard]] QStringList events() const;
    void clearEvents();

  signals:
    void refusesToStartChanged();
    void countChanged();
    void ratioChanged();
    void textChanged();
    void dateChanged();
    void timeChanged();
    void momentChanged();
    void pointChanged();
    void startsChanged();

  protected:
    bool onStart() override;
    void onStop() override;

  private:
    // The callbacks of name, consumed through member.
    template <typename T> bool consume(const QString& name, T*& member, Nebula::InterfaceRequirement requirement);

    bool m_refusesToStart = false;
    int m_count = 0;
    double m_ratio = 0;
    QString m_text;
    // Valid: JSON does not hold a null date or time, which a project and a settings file would then leave out.
    QDate m_date{2026, 1, 1};
    QTime m_time{12, 0};
    QDateTime m_moment{QDate(2026, 1, 1), QTime(0, 0), QTimeZone::UTC};
    QPoint m_point;
    int m_starts = 0;
    QStringList m_events;
    // By name; std::map, whose elements do not move: Component writes them through a reference.
    std::map<QString, QObject*> m_objects;
    std::map<QString, QTimer*> m_timers;
};
