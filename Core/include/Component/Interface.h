#pragma once

#include <QString>
#include <concepts>
#include <string_view>

namespace AppForge
{

// The id of an interface components provide or consume, given by APPFORGE_DECLARE_INTERFACE.
template <typename T> struct InterfaceTraits;

// A type declared with APPFORGE_DECLARE_INTERFACE.
template <typename T>
concept Interface = requires {
    { InterfaceTraits<T>::id } -> std::convertible_to<std::string_view>;
};

// The id of I: what matches an interface a component provides with the one another component consumes, across plugins.
template <Interface I> QString interfaceId()
{
    constexpr std::string_view id = InterfaceTraits<I>::id;
    return QString::fromUtf8(id.data(), static_cast<qsizetype>(id.size()));
}

} // namespace AppForge

// Declares Type as an interface components provide or consume, under iid; versioned, as a plugin consuming it might be
// built against another version. Used once, outside any namespace, in the header of the interface:
// APPFORGE_DECLARE_INTERFACE(IWidget, "Shuko83.AppForge.IWidget/1.0");
#define APPFORGE_DECLARE_INTERFACE(Type, iid)                                                                          \
    template <> struct AppForge::InterfaceTraits<Type>                                                                 \
    {                                                                                                                  \
        static constexpr std::string_view id = iid;                                                                    \
    }
