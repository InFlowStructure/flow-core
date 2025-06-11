// Copyright (c) 2024, Cisco Systems, Inc.
// All rights reserved.

#pragma once

#include "Core.hpp"

#include <chrono>
#include <memory>
#include <string_view>
#include <type_traits>

FLOW_NAMESPACE_BEGIN

// Forward declaration
class Node;

FLOW_NAMESPACE_END

/**
 * @brief Type traits for compile-time type information and validation
 */
FLOW_SUBNAMESPACE_START(type_traits)

/**
 * @brief Check if type is specialization of template
 * @example is_specialization_of<vector<int>, vector> == true
 */
template<typename T, template<typename...> class C>
struct is_specialization_of final : std::false_type
{
};

template<template<typename...> class C, typename... Args>
struct is_specialization_of<C<Args...>, C> final : std::true_type
{
};

template<typename T, template<typename...> class C>
constexpr bool is_specialization_of_v = is_specialization_of<T, C>::value;

FLOW_SUBNAMESPACE_END

/**
 * @brief Concepts for constraining template parameters
 */
FLOW_SUBNAMESPACE_START(concepts)

/**
 * @brief Requires type to be arithmetic (integral or floating point)
 */
template<typename T>
concept Arithmetic = std::is_arithmetic_v<T>;

/**
 * @brief Requires type to be a std::chrono duration
 */
template<typename T>
concept Duration = type_traits::is_specialization_of_v<T, std::chrono::duration>;

/**
 * @brief Requires type to be an enumeration
 */
template<class E>
concept Enumeration = std::is_enum_v<E>;

/**
 * @brief Requires type to be a reference type
 */
template<class R>
concept Reference = std::is_reference_v<R>;

/**
 * @brief Requires type to be convertible to string_view
 */
template<typename T>
concept String = std::is_convertible_v<T, std::string_view>;

/**
 * @brief Requires type to be a copyable pointer (raw, shared, or weak)
 */
template<typename T>
concept CopyablePointer = std::is_pointer_v<T> || type_traits::is_specialization_of_v<T, std::shared_ptr> ||
                          type_traits::is_specialization_of_v<T, std::weak_ptr>;

/**
 * @brief Requires type to be derived from Node
 */
template<class N>
concept NodeType = std::is_base_of_v<Node, N>;

/**
 * @brief Requires type to be a function
 */
template<class F>
concept Function = std::is_function_v<F>;

/**
 * @brief Requires type to be moveable but not copyable
 */
template<typename T>
concept OnlyMoveable = std::is_move_constructible_v<T> && std::is_move_assignable_v<T> &&
                       !(std::is_copy_constructible_v<T> || std::is_copy_assignable_v<T>);

FLOW_SUBNAMESPACE_END
