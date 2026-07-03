#include "JsonHelper.h"

namespace appforge::core::json_helper{

    // -----------------------------
    // QJsonValue → nlohmann::json
    // -----------------------------
    NJson toJsonValue(const QJsonValue& value)
    {
        switch (value.type())
        {
        case QJsonValue::Null:
            return nullptr;

        case QJsonValue::Bool:
            return value.toBool();

        case QJsonValue::Double:
            return value.toDouble();

        case QJsonValue::String:
            return value.toString().toStdString();

        case QJsonValue::Array:
        {
            NJson arr = NJson::array();
            QJsonArray qtArray = value.toArray();

            for (const QJsonValue& v : qtArray)
                arr.push_back(toJsonValue(v));

            return arr;
        }

        case QJsonValue::Object:
        {
            return toJson(value.toObject());
        }

        case QJsonValue::Undefined:
        default:
            return nullptr;
        }
    }


    // -----------------------------
    // nlohmann::json → QJsonValue
    // -----------------------------
    QJsonValue fromJsonValue(const NJson& j)
    {
        if (j.is_null())
            return QJsonValue();

        if (j.is_boolean())
            return QJsonValue(j.get<bool>());

        if (j.is_number_integer())
            return QJsonValue(j.get<int>());

        if (j.is_number_unsigned())
            return QJsonValue(static_cast<qint64>(j.get<unsigned long long>()));

        if (j.is_number_float())
            return QJsonValue(j.get<double>());

        if (j.is_string())
            return QJsonValue(QString::fromStdString(j.get<std::string>()));

        if (j.is_array())
        {
            QJsonArray arr;
            for (const auto& v : j)
                arr.append(fromJsonValue(v));

            return arr;
        }

        if (j.is_object())
        {
            QJsonObject obj;

            for (auto it = j.begin(); it != j.end(); ++it)
            {
                obj[QString::fromStdString(it.key())] =
                    fromJsonValue(it.value());
            }

            return obj;
        }

        return QJsonValue();
    }


    // -----------------------------
    // QJsonObject → nlohmann
    // -----------------------------
    NJson toJson(const QJsonObject& object)
    {
        NJson j = NJson::object();

        for (auto it = object.begin(); it != object.end(); ++it)
        {
            j[it.key().toStdString()] = toJsonValue(it.value());
        }

        return j;
    }


    // -----------------------------
    // nlohmann → QJsonObject
    // -----------------------------
    QJsonObject fromJson(const NJson& j)
    {
        QJsonObject obj;

        if (!j.is_object())
            return obj;

        for (auto it = j.begin(); it != j.end(); ++it)
        {
            obj[QString::fromStdString(it.key())] =
                fromJsonValue(it.value());
        }

        return obj;
    }


    // -----------------------------
    // Serialize → QByteArray
    // -----------------------------
    QByteArray toByteArray(const QJsonObject& object, int indent)
    {
        NJson j = toJson(object);
        std::string dumped = j.dump(indent);

        return QByteArray::fromStdString(dumped);
    }


    // -----------------------------
    // Deserialize ← QByteArray
    // -----------------------------
    QJsonObject fromByteArray(const QByteArray& data)
    {
        try
        {
            NJson j = NJson::parse(data.toStdString());
            return fromJson(j);
        }
        catch (...)
        {
            return QJsonObject();
        }
    }

}