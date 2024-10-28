// Copyright (c) 2024, Cisco Systems, Inc.
// All rights reserved.

#pragma once

#include "Core.hpp"

#include <chrono>
#include <memory>
#include <string_view>
#include <type_traits>

FLOW_NAMESPACE_START

class Node;

FLOW_NAMESPACE_END

FLOW_SUBNAMESPACE_START(type_traits)

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

FLOW_SUBNAMESPACE_START(concepts)

template<typename T>
concept Arithmetic = std::is_arithmetic_v<T>;

template<typename T>
concept Duration = type_traits::is_specialization_of_v<T, std::chrono::duration>;

template<class E>
concept Enumeration = std::is_enum_v<E>;

template<typename T>
concept String = std::is_convertible_v<T, std::string_view>;

template<typename T>
concept CopyablePointer = std::is_pointer_v<T> || type_traits::is_specialization_of_v<T, std::shared_ptr> ||
                          type_traits::is_specialization_of_v<T, std::weak_ptr>;

template<class N>
concept NodeType = std::is_base_of_v<Node, N>;

template<typename T>
concept OnlyMoveable = std::is_move_constructible_v<T> && std::is_move_assignable_v<T> &&
                       !(std::is_copy_constructible_v<T> || std::is_copy_assignable_v<T>);

FLOW_SUBNAMESPACE_END
