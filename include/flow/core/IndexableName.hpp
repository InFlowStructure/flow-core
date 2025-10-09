// Copyright (c) 2024, Cisco Systems, Inc.
// All rights reserved.

#pragma once

#include "Core.hpp"

#include <array>
#include <bit>
#include <compare>
#include <concepts>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string_view>

FLOW_NAMESPACE_BEGIN

/**
 * @brief Unique hashable integer representation of string.
 *
 * @details IndexableName creates an integer representation of string using CityHash64 for
 *          collision-resistant hashing. The hash is computed at compile-time when possible.
 *          The name reference is kept to help with visualization and debugging, but the hash
 *          is used for all comparisons and storage.
 *
 * @note Uses CityHash64
 */
class IndexableName
{
  private:
    /**
     * @brief Computes a CityHash64 hash for a span of bytes.
     *
     * @note Original algorithm from https://github.com/google/cityhash
     *
     * Copyright (c) 2011 Google, Inc.
     *
     * Permission is hereby granted, free of charge, to any person obtaining a copy
     * of this software and associated documentation files (the "Software"), to deal
     * in the Software without restriction, including without limitation the rights
     * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
     * copies of the Software, and to permit persons to whom the Software is
     * furnished to do so, subject to the following conditions:
     *
     * The above copyright notice and this permission notice shall be included in
     * all copies or substantial portions of the Software.
     *
     * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
     * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
     * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
     * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
     * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
     * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
     * THE SOFTWARE.
     */
    class CityHash64
    {
        using uint128_t = std::array<std::uint64_t, 2>;

        static constexpr std::uint64_t k0 = 0xc3a5c85c97cb3127ULL;
        static constexpr std::uint64_t k1 = 0xb492b66fbe98f273ULL;
        static constexpr std::uint64_t k2 = 0x9ae16a3b2f90404fULL;

      private:
        constexpr std::uint32_t SwapBytes(const std::uint32_t value)
        {
            return ((value >> 24) & 0x000000FF) | ((value >> 8) & 0x0000FF00) | ((value << 8) & 0x00FF0000) |
                   ((value << 24) & 0xFF000000);
        }

        constexpr std::uint64_t SwapBytes(const std::uint64_t value)
        {
            return ((value >> 56) & 0x00000000000000FF) | ((value >> 40) & 0x000000000000FF00) |
                   ((value >> 24) & 0x0000000000FF0000) | ((value >> 8) & 0x00000000FF000000) |
                   ((value << 8) & 0x000000FF00000000) | ((value << 24) & 0x0000FF0000000000) |
                   ((value << 40) & 0x00FF000000000000) | ((value << 56) & 0xFF00000000000000);
        }

        template<std::unsigned_integral T>
        constexpr T UnalignedLoad(std::string_view bytes)
        {
            T result = 0;
            if (std::is_constant_evaluated())
            {
                for (size_t i = 0; i < sizeof(T); ++i)
                {
                    result += bytes[i];
                    result <<= 8;
                }
            }
            else
            {
                std::memcpy(&result, bytes.data(), sizeof(T));
            }

            return result;
        }

        template<std::unsigned_integral T>
        constexpr T Fetch(std::string_view bytes)
        {
            return SwapBytes(UnalignedLoad<T>(bytes));
        }

        template<std::unsigned_integral T>
        constexpr std::uint64_t Rotate(T val, int shift)
        {
            return shift == 0 ? val : ((val >> shift) | (val << ((sizeof(T) * 8) - shift)));
        }

        constexpr std::uint64_t ShiftMix(std::uint64_t val) { return val ^ (val >> 47); }

        constexpr std::uint64_t Hash128to64(const uint128_t& x)
        {
            constexpr std::uint64_t kMul = 0x9ddfea08eb382d69ULL;
            std::uint64_t a              = (x[0] ^ x[1]) * kMul;
            a ^= (a >> 47);
            std::uint64_t b = (x[1] ^ a) * kMul;
            b ^= (b >> 47);
            b *= kMul;
            return b;
        }

        constexpr std::uint64_t HashLen16(std::uint64_t u, std::uint64_t v) { return Hash128to64(uint128_t{u, v}); }

        constexpr std::uint64_t HashLen16(std::uint64_t u, std::uint64_t v, std::uint64_t mul)
        {
            // Murmur-inspired hashing.
            std::uint64_t a = (u ^ v) * mul;
            a ^= (a >> 47);
            std::uint64_t b = (v ^ a) * mul;
            b ^= (b >> 47);
            b *= mul;
            return b;
        }

        constexpr std::uint64_t HashLen0to16(std::string_view bytes)
        {
            const std::size_t len = bytes.size();

            if (len >= 8)
            {
                std::uint64_t mul = k2 + len * 2;
                std::uint64_t a   = Fetch<std::uint64_t>(bytes) + k2;
                std::uint64_t b   = Fetch<std::uint64_t>(bytes.substr(len - 8));
                std::uint64_t c   = Rotate(b, 37) * mul + a;
                std::uint64_t d   = (Rotate(a, 25) + b) * mul;
                return HashLen16(c, d, mul);
            }
            if (len >= 4)
            {
                std::uint64_t mul = k2 + len * 2;
                std::uint64_t a   = Fetch<std::uint32_t>(bytes);
                return HashLen16(len + (a << 3), Fetch<std::uint32_t>(bytes.substr(len - 4)), mul);
            }
            if (len > 0)
            {
                std::uint8_t a  = static_cast<std::uint8_t>(bytes[0]);
                std::uint8_t b  = static_cast<std::uint8_t>(bytes[len >> 1]);
                std::uint8_t c  = static_cast<std::uint8_t>(bytes[len - 1]);
                std::uint32_t y = static_cast<std::uint32_t>(a) + (static_cast<std::uint32_t>(b) << 8);
                std::uint32_t z = static_cast<std::uint32_t>(len) + (static_cast<std::uint32_t>(c) << 2);
                return ShiftMix(y * k2 ^ z * k0) * k2;
            }
            return k2;
        }

        constexpr std::uint64_t HashLen17to32(std::string_view bytes)
        {
            const std::size_t len = bytes.size();

            std::uint64_t mul = k2 + len * 2;
            std::uint64_t a   = Fetch<std::uint64_t>(bytes) * k1;
            std::uint64_t b   = Fetch<std::uint64_t>(bytes.substr(8));
            std::uint64_t c   = Fetch<std::uint64_t>(bytes.substr(len - 8)) * mul;
            std::uint64_t d   = Fetch<std::uint64_t>(bytes.substr(len - 16)) * k2;
            return HashLen16(Rotate(a + b, 43) + Rotate(c, 30) + d, a + Rotate(b + k2, 18) + c, mul);
        }

        constexpr uint128_t WeakHashLen32WithSeeds(std::uint64_t w, std::uint64_t x, std::uint64_t y, std::uint64_t z,
                                                   std::uint64_t a, std::uint64_t b)
        {
            a += w;
            b               = Rotate(b + a + z, 21);
            std::uint64_t c = a;
            a += x;
            a += y;
            b += Rotate(a, 44);
            return {a + z, b + c};
        }

        constexpr uint128_t WeakHashLen32WithSeeds(std::string_view bytes, std::uint64_t a, std::uint64_t b)
        {
            return WeakHashLen32WithSeeds(Fetch<std::uint64_t>(bytes), Fetch<std::uint64_t>(bytes.substr(8)),
                                          Fetch<std::uint64_t>(bytes.substr(16)),
                                          Fetch<std::uint64_t>(bytes.substr(24)), a, b);
        }

        constexpr std::uint64_t HashLen33to64(std::string_view bytes)
        {
            const std::size_t len = bytes.size();
            std::uint64_t mul     = k2 + len * 2;
            std::uint64_t a       = Fetch<std::uint64_t>(bytes) * k2;
            std::uint64_t b       = Fetch<std::uint64_t>(bytes.substr(8));
            std::uint64_t c       = Fetch<std::uint64_t>(bytes.substr(len - 24));
            std::uint64_t d       = Fetch<std::uint64_t>(bytes.substr(len - 32));
            std::uint64_t e       = Fetch<std::uint64_t>(bytes.substr(16)) * k2;
            std::uint64_t f       = Fetch<std::uint64_t>(bytes.substr(24)) * 9;
            std::uint64_t g       = Fetch<std::uint64_t>(bytes.substr(len - 8));
            std::uint64_t h       = Fetch<std::uint64_t>(bytes.substr(len - 16)) * mul;
            std::uint64_t u       = Rotate(a + g, 43) + (Rotate(b, 30) + c) * 9;
            std::uint64_t v       = ((a + g) ^ d) + f + 1;
            std::uint64_t w       = SwapBytes((u + v) * mul) + h;
            std::uint64_t x       = Rotate(e + f, 42) + c;
            std::uint64_t y       = (SwapBytes((v + w) * mul) + g) * mul;
            std::uint64_t z       = e + f + c;

            a = SwapBytes((x + z) * mul + y) + b;
            b = ShiftMix((z + a) * mul + d + h) * mul;

            return b + x;
        }

      public:
        constexpr std::uint64_t operator()(std::string_view bytes)
        {
            std::size_t len = bytes.size();

            if (len <= 16)
            {
                return HashLen0to16(bytes);
            }
            else if (len <= 32)
            {
                return HashLen17to32(bytes);
            }
            else if (len <= 64)
            {
                return HashLen33to64(bytes);
            }

            // For strings over 64 bytes we hash the end first, and then as we
            // loop we keep 56 bytes of state: v, w, x, y, and z.
            std::uint64_t x = Fetch<std::uint64_t>(bytes.substr(len - 40));
            std::uint64_t y =
                Fetch<std::uint64_t>(bytes.substr(len - 16)) + Fetch<std::uint64_t>(bytes.substr(len - 56));
            std::uint64_t z = HashLen16(Fetch<std::uint64_t>(bytes.substr(len - 48)) + len,
                                        Fetch<std::uint64_t>(bytes.substr(len - 24)));
            uint128_t v     = WeakHashLen32WithSeeds(bytes.substr(len - 64), len, z);
            uint128_t w     = WeakHashLen32WithSeeds(bytes.substr(len - 32), y + k1, x);
            x               = x * k1 + Fetch<std::uint64_t>(bytes);

            // Decrease len to the nearest multiple of 64, and operate on 64-byte chunks.
            len = (len - 1) & ~static_cast<size_t>(63);
            do
            {
                x = Rotate(x + y + v[0] + Fetch<std::uint64_t>(bytes.substr(8)), 37) * k1;
                y = Rotate(y + v[1] + Fetch<std::uint64_t>(bytes.substr(48)), 42) * k1;
                x ^= w[1];
                y += v[0] + Fetch<std::uint64_t>(bytes.substr(40));
                z = Rotate(z + w[0], 33) * k1;
                v = WeakHashLen32WithSeeds(bytes, v[1] * k1, x + w[0]);
                w = WeakHashLen32WithSeeds(bytes.substr(32), z + w[1], y + Fetch<std::uint64_t>(bytes.substr(16)));
                std::swap(z, x);
                bytes = bytes.substr(64);
                len -= 64;
            } while (len != 0);

            return HashLen16(HashLen16(v[0], w[0]) + ShiftMix(y) * k1 + z, HashLen16(v[1], w[1]) + x);
        }
    };

  public:
    /// Construct from string view, computing hash at compile time when possible
    constexpr IndexableName(std::string_view name) noexcept : _value{CityHash64{}(name)}, _name{name} {}

    /// Construct from C-string, computing hash at compile time when possible
    constexpr IndexableName(const char* name) noexcept : IndexableName(std::string_view(name)) {}

    constexpr IndexableName(const IndexableName&) = default;
    constexpr IndexableName(IndexableName&&)      = default;

    constexpr IndexableName& operator=(const IndexableName&) = default;
    constexpr IndexableName& operator=(IndexableName&&)      = default;

    /**
     * @brief Three-way comparison operator.
     * @param other IndexableName to compare against.
     * @returns Strong ordering based on hash value.
     */
    constexpr auto operator<=>(const IndexableName& other) const { return _value <=> other._value; };

    /**
     * @brief Convert to size_t hash value.
     * @returns The hash value of this IndexableName.
     */
    constexpr operator std::size_t() const { return _value; }

    /**
     * @brief Convert to string view.
     * @returns String view of the original name.
     */
    constexpr operator std::string_view() const { return _name; }

    /// Get the hash value
    [[nodiscard]] constexpr std::size_t value() const noexcept { return _value; }

    /// Get the original string
    [[nodiscard]] constexpr std::string_view name() const noexcept { return _name; }

  public:
    static const IndexableName None;

  private:
    /// Hash of the name
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
    constexpr std::size_t operator()(const flow::IndexableName& name) const { return name.value(); }
};
