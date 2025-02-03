// Copyright (c) 2024, Cisco Systems, Inc.
// All rights reserved.

#include "flow/core/UUID.hpp"

#ifdef FLOW_WINDOWS
#define NOMINMAX
#include <objbase.h>
#elif defined(FLOW_APPLE)
#include <CoreFoundation/CFUUID.h>
#else
#include <uuid/uuid.h>
#endif

#include <stdexcept>
#include <stdlib.h>

namespace
{
#ifdef FLOW_WINDOWS
std::array<std::byte, 16> GenerateUUID()
{
    GUID guid;
    CoCreateGuid(&guid);
    return {
        static_cast<std::byte>((guid.Data1 >> 24) & 0xFF),
        static_cast<std::byte>((guid.Data1 >> 16) & 0xFF),
        static_cast<std::byte>((guid.Data1 >> 8) & 0xFF),
        static_cast<std::byte>((guid.Data1) & 0xFF),

        static_cast<std::byte>((guid.Data2 >> 8) & 0xFF),
        static_cast<std::byte>((guid.Data2) & 0xFF),

        static_cast<std::byte>((guid.Data3 >> 8) & 0xFF),
        static_cast<std::byte>((guid.Data3) & 0xFF),

        static_cast<std::byte>(guid.Data4[0]),
        static_cast<std::byte>(guid.Data4[1]),
        static_cast<std::byte>(guid.Data4[2]),
        static_cast<std::byte>(guid.Data4[3]),
        static_cast<std::byte>(guid.Data4[4]),
        static_cast<std::byte>(guid.Data4[5]),
        static_cast<std::byte>(guid.Data4[6]),
        static_cast<std::byte>(guid.Data4[7]),
    };
}

std::array<std::byte, 16> UUIDFromString(const std::string& uuid_str)
{
    GUID guid;
    std::wstring uuid_w = std::wstring(uuid_str.begin(), uuid_str.end());
    uuid_w              = L"{" + uuid_w + L"}";

    HRESULT hr = CLSIDFromString(uuid_w.c_str(), (LPCLSID)&guid);
    if (hr != S_OK)
    {
        throw std::invalid_argument("Bad UUID string: " + uuid_str);
    }

    return {
        static_cast<std::byte>((guid.Data1 >> 24) & 0xFF),
        static_cast<std::byte>((guid.Data1 >> 16) & 0xFF),
        static_cast<std::byte>((guid.Data1 >> 8) & 0xFF),
        static_cast<std::byte>((guid.Data1) & 0xFF),

        static_cast<std::byte>((guid.Data2 >> 8) & 0xFF),
        static_cast<std::byte>((guid.Data2) & 0xFF),

        static_cast<std::byte>((guid.Data3 >> 8) & 0xFF),
        static_cast<std::byte>((guid.Data3) & 0xFF),

        static_cast<std::byte>(guid.Data4[0]),
        static_cast<std::byte>(guid.Data4[1]),
        static_cast<std::byte>(guid.Data4[2]),
        static_cast<std::byte>(guid.Data4[3]),
        static_cast<std::byte>(guid.Data4[4]),
        static_cast<std::byte>(guid.Data4[5]),
        static_cast<std::byte>(guid.Data4[6]),
        static_cast<std::byte>(guid.Data4[7]),
    };
}
#elif defined(FLOW_APPLE)
std::array<std::byte, 16> GenerateUUID()
{
    auto uuid  = CFUUIDCreate(nullptr);
    auto bytes = CFUUIDGetUUIDBytes(uuid);
    CFRelease(uuid);
    return std::bit_cast<std::array<std::byte, 16>>(bytes);
}

std::array<std::byte, 16> UUIDFromString(const std::string& uuid_str)
{
    auto uuid_cfstr = CFStringCreateWithCStringNoCopy(nullptr, uuid_str.c_str(), kCFStringEncodingASCII, nullptr);
    auto uuid       = CFUUIDCreateFromString(nullptr, uuid_cfstr);
    auto bytes      = CFUUIDGetUUIDBytes(uuid);
    CFRelease(uuid);
    return std::bit_cast<std::array<std::byte, 16>>(bytes);
}
#else
std::array<std::byte, 16> GenerateUUID()
{
    uuid_t id;
    uuid_generate_random(id);
    return std::bit_cast<std::array<std::byte, 16>>(id);
}

std::array<std::byte, 16> UUIDFromString(const std::string& uuid_str)
{
    uuid_t id;
    uuid_parse(uuid_str.c_str(), id);
    return std::bit_cast<std::array<std::byte, 16>>(id);
}
#endif
} // namespace

FLOW_NAMESPACE_START

UUID::UUID() : _id{::GenerateUUID()} {}

UUID::UUID(const std::string& uuid_s) : _id{::UUIDFromString(uuid_s)} {}

UUID::operator std::string() const
{
    char str[37] = {0};
    snprintf(str, 37, "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
             static_cast<unsigned char>(_id[0]), static_cast<unsigned char>(_id[1]), static_cast<unsigned char>(_id[2]),
             static_cast<unsigned char>(_id[3]), static_cast<unsigned char>(_id[4]), static_cast<unsigned char>(_id[5]),
             static_cast<unsigned char>(_id[6]), static_cast<unsigned char>(_id[7]), static_cast<unsigned char>(_id[8]),
             static_cast<unsigned char>(_id[9]), static_cast<unsigned char>(_id[10]),
             static_cast<unsigned char>(_id[11]), static_cast<unsigned char>(_id[12]),
             static_cast<unsigned char>(_id[13]), static_cast<unsigned char>(_id[14]),
             static_cast<unsigned char>(_id[15]));
    return str;
}

FLOW_NAMESPACE_END
