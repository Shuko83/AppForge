// QtJsonConverters.hpp

#pragma once

#include <QDateTime>
#include <QList>
#include <QMap>
#include <QSet>
#include <QString>

#include <nlohmann/json.hpp>

// -----------------------------------------------------------------------------
// QString
// -----------------------------------------------------------------------------

inline void to_json(nlohmann::json& j, const QString& value)
{
    j = value.toStdString();
}

inline void from_json(const nlohmann::json& j, QString& value)
{
    value = QString::fromStdString(j.get<std::string>());
}

// -----------------------------------------------------------------------------
// QDateTime
// Stockage ISO 8601 avec millisecondes
// -----------------------------------------------------------------------------

inline void to_json(nlohmann::json& j, const QDateTime& value)
{
    j = value.toString(Qt::ISODateWithMs).toStdString();
}

inline void from_json(const nlohmann::json& j, QDateTime& value)
{
    value = QDateTime::fromString(
        QString::fromStdString(j.get<std::string>()),
        Qt::ISODateWithMs);
}

// -----------------------------------------------------------------------------
// QList<T>
// -----------------------------------------------------------------------------

template<typename T>
inline void to_json(nlohmann::json& j, const QList<T>& list)
{
    j = nlohmann::json::array();

    for (const auto& item : list)
    {
        j.push_back(item);
    }
}

template<typename T>
inline void from_json(const nlohmann::json& j, QList<T>& list)
{
    list.clear();

    for (const auto& item : j)
    {
        list.append(item.template get<T>());
    }
}

// -----------------------------------------------------------------------------
// QSet<T>
// -----------------------------------------------------------------------------

template<typename T>
inline void to_json(nlohmann::json& j, const QSet<T>& set)
{
    j = nlohmann::json::array();

    for (const auto& item : set)
    {
        j.push_back(item);
    }
}

template<typename T>
inline void from_json(const nlohmann::json& j, QSet<T>& set)
{
    set.clear();

    for (const auto& item : j)
    {
        set.insert(item.template get<T>());
    }
}

// -----------------------------------------------------------------------------
// QMap<QString, TValue>
// JSON n'accepte que des clés textuelles
// -----------------------------------------------------------------------------

template<typename TValue>
inline void to_json(
    nlohmann::json& j,
    const QMap<QString, TValue>& map)
{
    j = nlohmann::json::object();

    for (auto it = map.cbegin(); it != map.cend(); ++it)
    {
        j[it.key().toStdString()] = it.value();
    }
}

template<typename TValue>
inline void from_json(
    const nlohmann::json& j,
    QMap<QString, TValue>& map)
{
    map.clear();

    for (auto it = j.begin(); it != j.end(); ++it)
    {
        map.insert(
            QString::fromStdString(it.key()),
            it.value().template get<TValue>());
    }
}