// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QObject>
#include <QString>

// An object whose properties a component adds to its own, see Component::addPropertiesForQObject(): two of them
// are edited and saved, the others are not meant to be set from outside.
class Options : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int level MEMBER m_level NOTIFY levelChanged)
    Q_PROPERTY(bool enabled MEMBER m_enabled NOTIFY enabledChanged)
    Q_PROPERTY(QString hint MEMBER m_hint DESIGNABLE false)
    Q_PROPERTY(int cache MEMBER m_cache STORED false)

  public:
    explicit Options(QObject* parent = nullptr) : QObject(parent) {}

  signals:
    void levelChanged();
    void enabledChanged();

  private:
    int m_level = 0;
    bool m_enabled = false;
    QString m_hint;
    int m_cache = 0;
};
