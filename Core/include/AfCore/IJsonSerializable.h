#if not defined(I_JSON_SERIALIZABLE_H)
#define I_JSON_SERIALIZABLE_H

#include "CoreConstants.h"

#include <nlohmann/json.hpp>
#include <memory>

namespace appforge::core {

using json = nlohmann::json;

class CORE_EXPORT IJsonSerializable
{
public:
    virtual ~IJsonSerializable() = default;

    // ✅ Serialize
    [[nodiscard]] virtual json toJson() const = 0;

    // ✅ Deserialize into current instance
    virtual void fromJson(const json& json) = 0;

    // ✅ Static builder (templated)
    template<typename T> static std::unique_ptr<T> buildFromJson(const json& json)
    {
        static_assert(std::is_base_of<IJsonSerializable, T>::value,
                      "T must inherit from IJsonSerializable");

        auto instance = std::make_unique<T>();
        instance->fromJson(json);

        return instance;
    }
};

}
#endif