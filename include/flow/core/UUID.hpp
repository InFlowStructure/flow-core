// Copyright (c) 2024, Cisco Systems, Inc.
// All rights reserved.

#pragma once

#include "Core.hpp"

#include <array>
#include <bit>
#include <cstdint>
#include <string>

FLOW_NAMESPACE_START

/**
 * @brief Universally Unique IDentifier class
 */
class UUID
{
  public:
    /**
     * @brief Creates a random unique UUID.
     */
    UUID();

    /**
     * @brief Creates a UUID from a string.
     * @param uuid_str The correctly formatted UUID string to create UUID from.
     */
    UUID(const std::string& uuid_str);

    UUID(const UUID&) = default;
    UUID(UUID&&)      = default;

    UUID& operator=(const UUID&) = default;
    UUID& operator=(UUID&&)      = default;

    operator std::string() const;

    constexpr auto operator<=>(const UUID& other) const = default;

    /**
     * @brief Swap the value of the UUID with another.
     * @param other The other UUID to swap values with.
     */
    constexpr void swap(UUID& other) { _id.swap(other._id); }

  private:
    std::size_t hash() const
    {
        const uint64_t* half = reinterpret_cast<const uint64_t*>(_id.data());
        return half[0] ^ half[1];
    }

    friend struct std::hash<UUID>;

  private:
    std::array<std::byte, 16> _id;
};

FLOW_NAMESPACE_END

namespace std
{
template<>
struct hash<FLOW_NAMESPACE::UUID>
{
    std::size_t operator()(const FLOW_NAMESPACE::UUID& id) const { return id.hash(); }
};

template<>
inline void swap<FLOW_NAMESPACE::UUID>(FLOW_NAMESPACE::UUID& lhs, FLOW_NAMESPACE::UUID& rhs) noexcept
{
    lhs.swap(rhs);
}
} // namespace std
