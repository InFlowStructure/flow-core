// Copyright (c) 2024, Cisco Systems, Inc.
// All rights reserved.

#include "flow/core/Connection.hpp"
#include "flow/core/IndexableName.hpp"
#include "flow/core/UUID.hpp"

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <thread>

using namespace flow;

namespace
{
SharedConnection MakeConn()
{
    return std::make_shared<Connection>(UUID{}, IndexableName{"out"}, UUID{}, IndexableName{"in"});
}
} // namespace

TEST(ConnectionTest, ConstructsWithUniqueId)
{
    auto a = MakeConn();
    auto b = MakeConn();
    EXPECT_NE(a->ID(), b->ID());
}

TEST(ConnectionTest, StoresEndpoints)
{
    UUID start, end;
    Connection c(start, IndexableName{"out_port"}, end, IndexableName{"in_port"});

    EXPECT_EQ(c.StartNodeID(), start);
    EXPECT_EQ(c.EndNodeID(), end);
    EXPECT_EQ(c.StartPortKey(), IndexableName{"out_port"});
    EXPECT_EQ(c.EndPortKey(), IndexableName{"in_port"});
}

TEST(ConnectionTest, LockUnlockWorksAsMutex)
{
    auto c = MakeConn();
    c->lock();
    EXPECT_FALSE(c->try_lock());
    c->unlock();
    EXPECT_TRUE(c->try_lock());
    c->unlock();
}

TEST(ConnectionTest, LockGuardCompatibility)
{
    auto c = MakeConn();
    ASSERT_NO_THROW({
        std::lock_guard<Connection> _(*c);
    });
}

TEST(ConnectionTest, SaveProducesExpectedJson)
{
    UUID s, e;
    Connection c(s, IndexableName{"a"}, e, IndexableName{"b"});
    auto j = c.Save();

    ASSERT_TRUE(j.contains("in_id"));
    ASSERT_TRUE(j.contains("in_var_name"));
    ASSERT_TRUE(j.contains("out_id"));
    ASSERT_TRUE(j.contains("out_var_name"));
    EXPECT_EQ(j["in_id"], std::string(s));
    EXPECT_EQ(j["out_id"], std::string(e));
    EXPECT_EQ(j["in_var_name"], "a");
    EXPECT_EQ(j["out_var_name"], "b");
}

TEST(ConnectionTest, RestoreRoundTrips)
{
    UUID s, e;
    Connection c(s, IndexableName{"a"}, e, IndexableName{"b"});
    auto j = c.Save();

    Connection c2(UUID{}, IndexableName{"x"}, UUID{}, IndexableName{"y"});
    c2.Restore(j);

    EXPECT_EQ(c2.StartNodeID(), s);
    EXPECT_EQ(c2.EndNodeID(), e);
    EXPECT_EQ(static_cast<std::string_view>(c2.StartPortKey()), "a");
    EXPECT_EQ(static_cast<std::string_view>(c2.EndPortKey()), "b");
}

TEST(ConnectionTest, ConcurrentLockContention)
{
    auto c        = MakeConn();
    std::atomic<int> in_critical{0};
    std::atomic<int> max_in_critical{0};

    auto worker = [&] {
        for (int i = 0; i < 100; ++i)
        {
            std::lock_guard<Connection> _(*c);
            int cur = ++in_critical;
            int prev = max_in_critical.load();
            while (cur > prev && !max_in_critical.compare_exchange_weak(prev, cur)) {}
            std::this_thread::yield();
            --in_critical;
        }
    };

    std::thread t1(worker), t2(worker), t3(worker);
    t1.join(); t2.join(); t3.join();

    EXPECT_EQ(max_in_critical.load(), 1);
}
