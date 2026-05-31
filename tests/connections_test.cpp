// Copyright (c) 2024, Cisco Systems, Inc.
// All rights reserved.

#include "flow/core/Connections.hpp"
#include "flow/core/UUID.hpp"

#include <gtest/gtest.h>

using namespace flow;

TEST(ConnectionsContainerTest, EmptyOnConstruction)
{
    Connections c;
    EXPECT_EQ(c.Size(), 0u);
    EXPECT_TRUE(c.FindConnections(UUID{}).empty());
}

TEST(ConnectionsContainerTest, AddAndSize)
{
    Connections c;
    UUID a, b;
    auto& conn = c.Add(a, "out", b, "in");
    EXPECT_NE(conn, nullptr);
    EXPECT_EQ(c.Size(), 1u);
}

TEST(ConnectionsContainerTest, FindByNodeId)
{
    Connections c;
    UUID a, b, x;
    c.Add(a, "out", b, "in");
    c.Add(a, "out2", b, "in2");
    c.Add(x, "out", b, "in");

    auto from_a = c.FindConnections(a);
    EXPECT_EQ(from_a.size(), 2u);

    auto from_x = c.FindConnections(x);
    EXPECT_EQ(from_x.size(), 1u);

    auto from_missing = c.FindConnections(UUID{});
    EXPECT_TRUE(from_missing.empty());
}

TEST(ConnectionsContainerTest, FindByNodeAndPortKey)
{
    Connections c;
    UUID a, b;
    c.Add(a, "out", b, "in");
    c.Add(a, "other_out", b, "other_in");

    auto only_out = c.FindConnections(a, "out");
    ASSERT_EQ(only_out.size(), 1u);
    EXPECT_EQ(only_out[0]->StartPortKey(), IndexableName{"out"});

    auto missing_key = c.FindConnections(a, "nonexistent");
    EXPECT_TRUE(missing_key.empty());
}

TEST(ConnectionsContainerTest, RemoveByConnectionId)
{
    Connections c;
    UUID a, b;
    auto& conn = c.Add(a, "out", b, "in");
    auto id    = conn->ID();

    EXPECT_EQ(c.Size(), 1u);
    c.Remove(id);
    EXPECT_EQ(c.Size(), 0u);

    // Removing nonexistent id should be a no-op
    c.Remove(UUID{});
    EXPECT_EQ(c.Size(), 0u);
}

TEST(ConnectionsContainerTest, RemoveByStartEndPair)
{
    Connections c;
    UUID a, b, other;
    c.Add(a, "out", b, "in");
    c.Add(a, "out2", other, "in");

    EXPECT_EQ(c.Size(), 2u);
    c.Remove(a, b);
    EXPECT_EQ(c.Size(), 1u);

    // No matching pair
    c.Remove(a, UUID{});
    EXPECT_EQ(c.Size(), 1u);

    // Start id not in container
    c.Remove(UUID{}, b);
    EXPECT_EQ(c.Size(), 1u);
}

TEST(ConnectionsContainerTest, RemoveByNodeIdDropsAll)
{
    Connections c;
    UUID a, b, x;
    c.Add(a, "out", b, "in");
    c.Add(a, "out2", b, "in2");
    c.Add(x, "out", b, "in");

    EXPECT_EQ(c.Size(), 3u);
    c.RemoveByNodeID(a);
    EXPECT_EQ(c.Size(), 1u);

    // Removing again on the now-absent id is a no-op
    c.RemoveByNodeID(a);
    EXPECT_EQ(c.Size(), 1u);
}

TEST(ConnectionsContainerTest, ClearEmptiesContainer)
{
    Connections c;
    UUID a, b;
    c.Add(a, "out", b, "in");
    c.Add(a, "out2", b, "in2");

    c.Clear();
    EXPECT_EQ(c.Size(), 0u);
}

TEST(ConnectionsContainerTest, IteratesAllEntries)
{
    Connections c;
    UUID a, b;
    c.Add(a, "out", b, "in");
    c.Add(a, "out2", b, "in2");

    std::size_t count = 0;
    for (auto it = c.begin(); it != c.end(); ++it)
    {
        ASSERT_NE(it->second, nullptr);
        ++count;
    }
    EXPECT_EQ(count, 2u);

    const Connections& cc = c;
    std::size_t const_count = 0;
    for (auto it = cc.begin(); it != cc.end(); ++it)
    {
        ++const_count;
    }
    EXPECT_EQ(const_count, 2u);
}
