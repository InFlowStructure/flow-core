// Copyright (c) 2024, Cisco Systems, Inc.
// All rights reserved.

#pragma once

#include "Core.hpp"

#include <algorithm>
#include <string_view>
#include <type_traits>

FLOW_NAMESPACE_START

/**
 * @brief Compile time type that creates a string representation of a C++ type.
 * @tparam T The type to get a string representation of.
 */
template<typename T>
struct TypeName;

template<>
struct TypeName<void>
{
    static constexpr std::string_view value = "void";
    static constexpr bool Check(std::string_view v) { return v == value; }
};

namespace detail
{
template<typename T>
constexpr std::string_view wrapped_type_name()
{
#ifdef __clang__
    return __PRETTY_FUNCTION__;
#elif defined(__GNUC__)
    return __PRETTY_FUNCTION__;
#elif defined(_MSC_VER)
    return __FUNCSIG__;
#else
#error "Unsupported compiler"
#endif
}

constexpr std::size_t wrapped_type_name_prefix_length()
{
    return wrapped_type_name<void>().find(TypeName<void>::value);
}

constexpr std::size_t wrapped_type_name_suffix_length()
{
    return wrapped_type_name<void>().length() - wrapped_type_name_prefix_length() - TypeName<void>::value.length();
}
} // namespace detail

template<typename T>
struct TypeName
{
    /**
     * @brief The string representation of the given type.
     */
    static constexpr std::string_view value = [] {
        constexpr auto wrapped_name    = detail::wrapped_type_name<typename std::remove_cvref_t<T>>();
        constexpr auto prefix_length   = detail::wrapped_type_name_prefix_length();
        constexpr auto suffix_length   = detail::wrapped_type_name_suffix_length();
        constexpr auto TypeName_length = wrapped_name.length() - prefix_length - suffix_length;
        constexpr auto name            = wrapped_name.substr(prefix_length, TypeName_length);

        constexpr auto adjusted_prefix_length = (name.starts_with("class") ? 6 : (name.starts_with("struct") ? 7 : 0));
        constexpr auto adjusted_TypeName_length = name.length() - adjusted_prefix_length;

        return name.substr(adjusted_prefix_length, adjusted_TypeName_length);
    }();
};

template<typename T, typename U>
constexpr bool operator==(const TypeName<T>&, const TypeName<U>&)
{
    return false;
}

template<typename T>
constexpr bool operator==(const TypeName<T>&, const TypeName<T>&)
{
    return true;
}

template<typename T>
constexpr bool operator==(const TypeName<T>& type_name, const std::string_view& type_string)
{
    return type_name.value == type_string;
}

/**
 * @brief Helper method for retrieving the value of a TypeName to save line space.
 * @tparam T The type to get a string representation of.
 */
template<typename T>
static constexpr std::string_view TypeName_v = TypeName<T>::value;

FLOW_NAMESPACE_END
