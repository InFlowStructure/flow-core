// Copyright (c) 2024, Cisco Systems, Inc.
// All rights reserved.

#include "flow/core/UUID.hpp"

#include <gtest/gtest.h>

#include <stdexcept>
#include <string>

using namespace flow;

// -------------------------------------------------------------------------
// Positive cases
// -------------------------------------------------------------------------

TEST(UUIDTest, DefaultConstruction_DoesNotThrow) { ASSERT_NO_THROW(UUID{}); }

TEST(UUIDTest, WellFormedString_RoundTrips)
{
    const std::string canonical = "4bb1696d-30d1-41d2-aa19-0edf0560a951";
    UUID uuid(canonical);
    EXPECT_EQ(static_cast<std::string>(uuid), canonical);
}

TEST(UUIDTest, WellFormedString_AllLowercase_RoundTrips)
{
    // The toString() path uses lowercase hex; verify another well-formed UUID
    const std::string canonical = "550e8400-e29b-41d4-a716-446655440000";
    UUID uuid(canonical);
    EXPECT_EQ(static_cast<std::string>(uuid), canonical);
}

// -------------------------------------------------------------------------
// Negative cases — all must throw std::invalid_argument
// -------------------------------------------------------------------------

TEST(UUIDTest, EmptyString_ThrowsInvalidArgument) { EXPECT_THROW(UUID(""), std::invalid_argument); }

TEST(UUIDTest, NotAUUID_ThrowsInvalidArgument) { EXPECT_THROW(UUID("not-a-uuid"), std::invalid_argument); }

TEST(UUIDTest, GarbageInteriorChars_ThrowsInvalidArgument)
{
    // Correct length and hyphen positions, but non-hex interior characters.
    // This is the Linux regression: before the fix uuid_parse returned nonzero
    // but the discarded return value let garbage stack bytes through silently.
    EXPECT_THROW(UUID("xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"), std::invalid_argument);
}

TEST(UUIDTest, TooShort_ThrowsInvalidArgument) { EXPECT_THROW(UUID("4bb1696d-30d1-41d2"), std::invalid_argument); }

TEST(UUIDTest, TooLong_ThrowsInvalidArgument)
{
    EXPECT_THROW(UUID("4bb1696d-30d1-41d2-aa19-0edf0560a951-extra"), std::invalid_argument);
}

TEST(UUIDTest, MalformedHyphens_ThrowsInvalidArgument)
{
    // Right hex chars but hyphens in wrong places
    EXPECT_THROW(UUID("4bb1696d30d1-41d2-aa19-0edf0560a951"), std::invalid_argument);
}
