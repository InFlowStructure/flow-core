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
 * @details IndexableName creates an integer representation of string which can be quickly compared and easily stored.
 *          It maintains an unowned reference to the name that was used, which should generally be a const char* or a
 *          string that you know will never go away. The name reference is kept to help with visualizing the names when
 *          printed or debugged.
 */
class IndexableName
{
    /// The generated CRC-64 table.
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
     * @brief Reverses the bits in a 64 bit integer.
     * @param value The value to reverse.
     * @returns The reversed value.
     */
    static constexpr std::size_t reverse_bits(std::size_t value)
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
     * @returns The hash of the given string.
     */
    static constexpr std::size_t hash(std::string_view str)
    {
        std::size_t crc = 0;
        for (auto c : str)
        {
            crc = crc_table[(crc & 0xFF) ^ c] ^ (crc >> 8);
        }

        return reverse_bits(crc);
    }

  public:
    constexpr IndexableName(std::string_view name) : _value{hash(name)}, _name{name} {}
    constexpr IndexableName(const char* name) : IndexableName(std::string_view(name)) {}
    constexpr IndexableName(const IndexableName&) = default;
    constexpr IndexableName(IndexableName&&)      = default;

    constexpr IndexableName& operator=(const IndexableName&) = default;
    constexpr IndexableName& operator=(IndexableName&&)      = default;

    constexpr auto operator<=>(const IndexableName& other) const { return _value <=> other._value; };

    constexpr operator std::size_t() const { return _value; }
    constexpr operator std::string_view() const { return _name; }

  public:
    static const IndexableName None;

  private:
    std::size_t _value;
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
