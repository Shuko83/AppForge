#pragma once

#include "CoreConstants.h"

#include <QJsonObject>

#include <memory>

namespace appforge::core {

class AFCORE_EXPORT IJsonSerializable
{
public:
    virtual ~IJsonSerializable() = default;

    // ✅ Serialize
    virtual QJsonObject toJson() const = 0;

    // ✅ Deserialize into current instance
    virtual void fromJson(const QJsonObject& j) = 0;

    // ✅ Static builder (templated)
    template<typename T>
    static std::unique_ptr<T> buildFromJson(const QJsonObject& j)
    {
        static_assert(std::is_base_of<IJsonSerializable, T>::value,
                      "T must inherit from IJsonSerializable");

        auto instance = std::make_unique<T>();
        instance->fromJson(j);

        return instance;
    }
};

}