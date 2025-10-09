// Copyright (c) 2024, Cisco Systems, Inc.
// All rights reserved.

#pragma once

#include "Core.hpp"

#include <array>
#include <bit>
#include <cstdint>
#include <string>

FLOW_NAMESPACE_BEGIN

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

    /**
     * @brief Convert UUID to string representation.
     * @returns String representation of the UUID in standard format.
     */
    operator std::string() const;

    /**
     * @brief Three-way comparison operator.
     * @param other UUID to compare against.
     * @returns Strong ordering based on internal byte representation.
     */
    constexpr auto operator<=>(const UUID& other) const = default;

    /**
     * @brief Swap the value of the UUID with another.
     * @param other The other UUID to swap values with.
     */
    constexpr void swap(UUID& other) { _id.swap(other._id); }

    /**
     * @brief Compute hash value for UUID.
     * @returns Size_t hash of the UUID data.
     */
    [[nodiscard]] std::size_t hash() const noexcept
    {
        const auto* half = reinterpret_cast<const uint64_t*>(_id.data());
        return half[0] ^ half[1];
    }

  private:
    /// Storage for 16-byte UUID value
    std::array<std::byte, 16> _id;
};

FLOW_NAMESPACE_END

namespace std
{
template<>
struct hash<flow::UUID>
{
    std::size_t operator()(const flow::UUID& id) const { return id.hash(); }
};

template<>
inline void swap<flow::UUID>(flow::UUID& lhs, flow::UUID& rhs) noexcept
{
    lhs.swap(rhs);
}
} // namespace std
