// Copyright (c) 2024, Cisco Systems, Inc.
// All rights reserved.

#pragma once

#include "Core.hpp"

#include <array>
#include <cstdint>
#include <string_view>

FLOW_NAMESPACE_BEGIN

/**
 * @brief Unique hashable integer representation of string.
 *
 * @details IndexableName creates an integer representation of string using CRC-64-ECMA for
 *          collision-resistant hashing. The hash is computed at compile-time when possible.
 *          The name reference is kept to help with visualization and debugging, but the hash
 *          is used for all comparisons and storage.
 *
 * @note Uses CRC-64-ECMA polynomial: 0xC96C5795D7870F42
 */
class IndexableName
{
  private:
    /// CRC-64-ECMA lookup table for efficient hash computation
    static constexpr std::array<std::size_t, 256> crc_table = [] {
        std::array<std::size_t, 256> table;
        for (std::size_t c = 0; c < 256; ++c)
        {
            std::size_t crc = c;
            for (std::size_t i = 0; i < 8; ++i)
            {
                std::size_t b = (crc & 1);
                crc >>= 1;
                crc ^= (0 - b) & 0xc96c5795d7870f42ull;
            }
            table[c] = crc;
        }

        return table;
    }();

    /**
     * @brief Reverses bits in a 64-bit integer for CRC finalization.
     * @param value The value to reverse
     * @returns The bit-reversed value
     */
    static constexpr std::size_t reverse_bits(std::size_t value) noexcept
    {
        std::size_t result = 0;
        for (std::size_t i = 0; i < 64; ++i, value >>= 1)
        {
            result = (result << 1) | (value & 1);
        }

        return result;
    }

    /**
     * @brief Computes CRC-64-ECMA hash of string.
     * @param str The string to hash
     * @returns The CRC-64 hash of the given string
     * @throws std::invalid_argument if str is empty
     */
    static constexpr std::size_t hash(std::string_view str)
    {
        if (str.empty())
        {
            throw std::invalid_argument("IndexableName cannot be empty");
        }

        std::size_t crc = 0;
        for (auto c : str)
        {
            crc = crc_table[(crc & 0xFF) ^ c] ^ (crc >> 8);
        }

        return reverse_bits(crc);
    }

  public:
    /// Construct from string view, computing hash at compile time when possible
    constexpr IndexableName(std::string_view name) noexcept : _value{hash(name)}, _name{name} {}

    /// Construct from C-string, computing hash at compile time when possible
    constexpr IndexableName(const char* name) noexcept : IndexableName(std::string_view(name)) {}

    constexpr IndexableName(const IndexableName&) = default;
    constexpr IndexableName(IndexableName&&)      = default;

    constexpr IndexableName& operator=(const IndexableName&) = default;
    constexpr IndexableName& operator=(IndexableName&&)      = default;

    constexpr auto operator<=>(const IndexableName& other) const { return _value <=> other._value; };

    constexpr operator std::size_t() const { return _value; }
    constexpr operator std::string_view() const { return _name; }

    /// Get the hash value
    [[nodiscard]] constexpr std::size_t value() const noexcept { return _value; }

    /// Get the original string
    [[nodiscard]] constexpr std::string_view name() const noexcept { return _name; }

  public:
    static const IndexableName None;

  private:
    /// CRC-64-ECMA hash of the name
    std::size_t _value;

    /// Reference to the original string
    std::string_view _name;
};

/**
 * @brief Constant representing the unique idea for "None".
 *
 * @details Indexable name SHOULD have an idea of "empty" without using an actually null (i.e. "") string. Thus, this is
 *          the default idea of an "empty" IndexableName.
 */
inline constexpr const IndexableName IndexableName::None{"None"};

FLOW_NAMESPACE_END

template<>
struct std::hash<flow::IndexableName>
{
    std::size_t operator()(const flow::IndexableName& name) const { return std::size_t(name); }
};
