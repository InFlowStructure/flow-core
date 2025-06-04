// Copyright (c) 2024, Cisco Systems, Inc.
// All rights reserved.

#pragma once

#include "Concepts.hpp"
#include "TypeName.hpp"

#include <nlohmann/json_fwd.hpp>

#include <chrono>
#include <memory>
#include <type_traits>

FLOW_NAMESPACE_START

using json = nlohmann::json;

/**
 * @brief Converts the given value type to a string.
 * @tparam T The type of the value being converted.
 *
 * @param value The value to convert to string.
 *
 * @returns The string representation of the given value.
 */
template<typename T>
std::string ToString(const T&)
{
    return "";
}

template<concepts::String T>
std::string ToString(const T& value)
{
    return value;
}

template<concepts::Arithmetic T>
std::string ToString(const T& value)
{
    return std::to_string(value);
}

/**
 * @brief Function which returns a mapping of enum values to strings
 * @tparam T The enumeration type.
 *
 * @note This method MUST be overridden manually to account for custom enums.
 *
 * @returns A map of enum values to their respective string names.
 */
template<concepts::Enumeration T>
std::map<T, std::string> EnumToStringMap();

template<concepts::Enumeration T>
std::string ToString(const T& value)
{
    return EnumToStringMap<T>()[value];
}

template<typename T>
std::string ToString(const std::unique_ptr<T>& value)
{
    return value ? ::FLOW_NAMESPACE::ToString<T>(*value) : "None";
}

template<concepts::CopyablePointer T>
std::string ToString(const T& value)
{
    return value ? ::FLOW_NAMESPACE::ToString(*value) : "None";
}

template<typename T>
std::string ToString(const std::reference_wrapper<T>& value)
{
    return ::FLOW_NAMESPACE::ToString<T>(value.get());
}

template<typename T>
std::string ToString(const std::vector<T>& value)
{
    std::string os;
    os += "[";
    std::for_each(value.begin(), std::prev(value.end()), [&](auto&& v) { os += ToString<T>(v) + ", "; });
    os += ToString<T>(value.back()) + "]";
    return os;
}

template<concepts::Duration T>
std::string ToString(const T& value)
{
    return ::FLOW_NAMESPACE::ToString<std::int64_t>(value.count());
}

template<typename T>
struct EnumAsByte : std::false_type
{
};

template<concepts::Enumeration T>
struct EnumAsByte<T> : std::true_type
{
};

template<concepts::Enumeration T>
struct TypeName<T> : TypeName<EnumAsByte<T>>
{
};

/**
 * @brief Interface for node data.
 */
class INodeData
{
  public:
    virtual ~INodeData() = default;

    /**
     * @brief String of the data type currently held.
     * @returns A string view of the typename of the held data.
     */
    virtual constexpr std::string_view Type() const = 0;

    /**
     * @brief Get the string representation of the current data's value.
     * @returns The string representation of the current value.
     */
    virtual std::string ToString() const = 0;

  protected:
    /**
     * @brief Get the current data as a void pointer.
     * @returns A void pointer to the current data.
     */
    virtual void* AsPointer() const = 0;

    /**
     * @brief Sets the value of the current data from the given data.
     */
    virtual void FromPointer(void* data) = 0;

    friend class Port;
};

/**
 * @brief The alias for shared node data.
 */
using SharedNodeData = std::shared_ptr<class INodeData>;

namespace detail
{
template<typename T>
class NodeData : public INodeData
{
  public:
    NodeData()          = default;
    virtual ~NodeData() = default;

    constexpr NodeData(const NodeData&) = default;
    constexpr NodeData(NodeData&&)      = default;
    constexpr NodeData(const T& value) : _value{value} {}
    constexpr NodeData(T&& value) : _value{std::move(value)} {}

    template<typename U, std::enable_if_t<std::is_convertible_v<U, T>, bool> = true>
    constexpr NodeData(const U& other) : _value{static_cast<const T&>(other)}
    {
    }

    template<typename U, std::enable_if_t<std::is_convertible_v<U, T>, bool> = true>
    constexpr NodeData(const NodeData<U>& other) : _value{static_cast<const T&>(other.Get())}
    {
    }

    template<typename U, std::enable_if_t<std::is_convertible_v<U, T>, bool> = true>
    constexpr NodeData(NodeData<U>&& other) : _value{static_cast<T&&>(std::move(other.Get()))}
    {
    }

    constexpr NodeData& operator=(const NodeData&) = default;
    constexpr NodeData& operator=(NodeData&&)      = default;

    template<typename U, std::enable_if_t<std::is_convertible_v<U, T>, bool> = true>
    constexpr NodeData& operator=(const NodeData<U>& other)
    {
        _value = static_cast<const T&>(other._value);
        return *this;
    }

    template<typename U, std::enable_if_t<std::is_convertible_v<U, T>, bool> = true>
    constexpr NodeData& operator=(NodeData<U>&& other)
    {
        _value = static_cast<T&&>(std::move(other._value));
        return *this;
    }

    constexpr NodeData& operator=(const T& value)
    {
        _value = value;
        return *this;
    }

    constexpr NodeData& operator=(T&& value)
    {
        _value = std::move(value);
        return *this;
    }

    template<typename U, std::enable_if_t<std::is_convertible_v<U, T>, bool> = true>
    constexpr NodeData& operator=(const U& value)
    {
        _value = static_cast<const T&>(value);
        return *this;
    }

    template<typename U, std::enable_if_t<std::is_convertible_v<U, T>, bool> = true>
    constexpr NodeData& operator=(U&& value)
    {
        _value = static_cast<T&&>(std::move(value));
        return *this;
    }

    [[nodiscard]] constexpr T& operator*() noexcept { return _value; }
    [[nodiscard]] constexpr const T& operator*() const noexcept { return _value; }

    [[nodiscard]] constexpr T& Get() noexcept { return _value; }
    [[nodiscard]] constexpr const T& Get() const noexcept { return _value; }

    constexpr void Set(const T& value) { _value = value; }
    constexpr void Set(T&& value) { _value = std::move(value); }

    virtual constexpr std::string_view Type() const noexcept final { return TypeName_v<T>; }

    std::string ToString() const final
    try
    {
        return ::FLOW_NAMESPACE::ToString(this->_value);
    }
    catch (const std::exception& e)
    {
        return "Error: " + std::string(e.what());
    }

  protected:
    void* AsPointer() const override { return std::bit_cast<void*>(&this->_value); }
    void FromPointer(void* value) override
    {
        if constexpr (std::is_copy_assignable_v<T>)
        {
            this->_value = *std::bit_cast<T*>(value);
        }

        if constexpr (std::is_move_assignable_v<T>)
        {
            this->_value = std::move(*std::bit_cast<T*>(value));
        }
    }

  protected:
    T _value;
};

template<typename T>
class NodeData<T&> : public INodeData
{
  public:
    using ValueType = T;

    NodeData()          = delete;
    virtual ~NodeData() = default;

    constexpr NodeData(const NodeData&) = default;
    constexpr NodeData(NodeData&&)      = default;
    constexpr NodeData(T& value) : _value{value} {}
    constexpr NodeData(T&& value) = delete;

    template<typename U, std::enable_if_t<std::is_convertible_v<U, T>, bool> = true>
    constexpr NodeData(const NodeData<U>& other) : _value{static_cast<const T&>(other._value)}
    {
    }

    constexpr NodeData& operator=(const NodeData&) = default;
    constexpr NodeData& operator=(NodeData&&)      = delete;

    template<typename U>
        requires std::convertible_to<U, T>
    constexpr NodeData& operator=(const NodeData<U>& other)
    {
        _value = static_cast<const T&>(other._value);
        return *this;
    }

    constexpr NodeData& operator=(const T& value)
    {
        _value = value;
        return *this;
    }

    template<typename U>
        requires std::convertible_to<U, T>
    constexpr NodeData& operator=(const U& value)
    {
        _value = static_cast<const T&>(value);
        return *this;
    }

    [[nodiscard]] constexpr T& operator*() { return _value; }
    [[nodiscard]] constexpr const T& operator*() const { return _value; }

    [[nodiscard]] constexpr T& Get() { return _value; }
    [[nodiscard]] constexpr const T& Get() const { return _value; }

    virtual constexpr std::string_view Type() const final { return TypeName_v<T>; }

    std::string ToString() const final
    try
    {
        return ::FLOW_NAMESPACE::ToString(this->_value);
    }
    catch (const std::exception& e)
    {
        return "Error: " + std::string(e.what());
    }

  protected:
    void* AsPointer() const override { return std::bit_cast<void*>(&this->_value); }
    void FromPointer(void* value) override
    {
        if constexpr (std::is_copy_assignable_v<T>)
        {
            this->_value = *std::bit_cast<T*>(value);
        }

        if constexpr (std::is_move_assignable_v<T>)
        {
            this->_value = std::move(*std::bit_cast<T*>(value));
        }
    }

  protected:
    T& _value;
};
} // namespace detail

/**
 * @brief Templated type for containing node data.
 * @tparam T The data type.
 */
template<typename T>
class NodeData : public detail::NodeData<T>
{
    using Base = detail::NodeData<T>;

  public:
    virtual ~NodeData() = default;

    using Base::Base;
    using Base::operator=;
};

template<concepts::OnlyMoveable T>
class NodeData<T> : public detail::NodeData<T>
{
    using Base = detail::NodeData<T>;

  public:
    virtual ~NodeData() = default;

    using Base::Base;
    using Base::operator=;
};

/**
 * @brief Specialisation for unique_ptr.
 */
template<typename T>
class NodeData<std::unique_ptr<T>> : public detail::NodeData<std::unique_ptr<T>>
{
    using Base = detail::NodeData<std::unique_ptr<T>>;

  public:
    constexpr NodeData(const NodeData&) = delete;
    constexpr NodeData(NodeData&&)      = default;
    constexpr NodeData(std::unique_ptr<T>&& value) : Base(std::move(value)) {}

    template<typename U, std::enable_if_t<std::is_convertible_v<U, T>, bool> = true>
    constexpr NodeData(NodeData<std::unique_ptr<U>>&& other)
        : detail::NodeData<std::unique_ptr<T>>::NodeData{std::make_unique<T>(static_cast<T&&>(std::move(other.Get())))}
    {
    }

    virtual ~NodeData() = default;

    constexpr NodeData& operator=(const NodeData&) = delete;
    constexpr NodeData& operator=(NodeData&&)      = default;

    constexpr auto operator*() const noexcept { return this->_value.operator*(); }
    constexpr auto operator->() const noexcept { return this->_value.operator->(); }
};

/**
 * @brief Specialisation for copyable pointer types (raw, shared, weak)
 */
template<concepts::CopyablePointer Ptr>
class NodeData<Ptr> : public detail::NodeData<Ptr>
{
    using Base = detail::NodeData<Ptr>;

  public:
    virtual ~NodeData() = default;

    using Base::Base;
    using Base::operator=;

    constexpr auto operator*() const noexcept { return this->_value.operator*(); }
    constexpr auto operator->() const noexcept { return this->_value.operator->(); }
};

/**
 * @brief Disallow the use of const char* data type, and instead use the std::string version.
 */
NodeData(const char*) -> NodeData<std::string>;

/**
 * @brief Specialisation for std::chrono::duration types.
 */
template<concepts::Duration D>
class NodeData<D> : public detail::NodeData<D>
{
    using Base = detail::NodeData<D>;

  public:
    template<concepts::Duration U>
    constexpr NodeData(const U& other) : NodeData<D>{std::chrono::duration_cast<D>(other)}
    {
    }

    template<concepts::Duration U>
    constexpr NodeData(const NodeData<U>& other) : NodeData<D>{std::chrono::duration_cast<D>(other.Get())}
    {
    }

    virtual ~NodeData() = default;

    using Base::Base;
    using Base::operator=;
};

/**
 * @brief Templated shared pointer type of NodeData.
 * @tparam T The type of data stored.
 */
template<typename T>
using TSharedNodeData = std::shared_ptr<class NodeData<T>>;

/**
 * @brief Templated weak pointer type of NodeData.
 * @tparam T The type of data stored.
 */
template<typename T>
using TWeakNodeData = std::weak_ptr<class NodeData<T>>;

/**
 * @brief Helper function for TSharedNodeData.
 * @tparam T The data type being created.
 * @param value The value of the data to store.
 * @returns A shared pointer of the NodeData.
 */
template<typename T>
[[nodiscard]] constexpr auto MakeNodeData(const T& value)
{
    return std::make_shared<NodeData<typename std::remove_reference_t<T>>>(value);
}

template<typename T>
[[nodiscard]] constexpr auto MakeNodeData(T&& value)
{
    return std::make_shared<NodeData<typename std::remove_reference_t<T>>>(std::move(value));
}

template<typename T, typename U>
[[nodiscard]] constexpr auto MakeNodeData(const NodeData<U>& value)
{
    return std::make_shared<NodeData<typename std::remove_reference_t<T>>>(value);
}

template<typename T, typename U>
[[nodiscard]] constexpr auto MakeNodeData(NodeData<U>&& value)
{
    return std::make_shared<NodeData<typename std::remove_reference_t<T>>>(std::move(value));
}

template<typename T>
[[nodiscard]] constexpr auto MakeRefNodeData(typename std::remove_reference_t<T>& value)
{
    return std::make_shared<NodeData<typename std::remove_reference_t<T>&>>(value);
}

template<typename T>
[[nodiscard]] constexpr TSharedNodeData<T> CastNodeData(const SharedNodeData& value)
{
    return std::dynamic_pointer_cast<NodeData<T>>(value);
}

FLOW_NAMESPACE_END
