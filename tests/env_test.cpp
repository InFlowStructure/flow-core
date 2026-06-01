// Copyright (c) 2024, Cisco Systems, Inc.
// All rights reserved.

#include "flow/core/Env.hpp"
#include "flow/core/NodeFactory.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <stdlib.h>

using namespace flow;

namespace
{
int PortableSetenv(const char* name, const char* value)
{
#ifdef _WIN32
    return _putenv_s(name, value);
#else
    return setenv(name, value, 1);
#endif
}

int PortableUnsetenv(const char* name)
{
#ifdef _WIN32
    return _putenv_s(name, "");
#else
    return unsetenv(name);
#endif
}
} // namespace

TEST(EnvTest, CreatesWithDefaultSettings)
{
    auto factory = std::make_shared<NodeFactory>();
    auto env     = Env::Create(factory);
    ASSERT_NE(env, nullptr);
    EXPECT_EQ(env->GetFactory(), factory);
}

TEST(EnvTest, CreatesWithCustomMaxThreads)
{
    auto factory = std::make_shared<NodeFactory>();
    Settings s;
    s.MaxThreads = 2;
    auto env     = Env::Create(factory, s);
    ASSERT_NE(env, nullptr);
}

TEST(EnvTest, GetVarReturnsValueWhenSet)
{
    auto factory = std::make_shared<NodeFactory>();
    auto env     = Env::Create(factory);
    ASSERT_EQ(PortableSetenv("FLOW_CORE_TEST_VAR", "hello"), 0);
    EXPECT_EQ(env->GetVar("FLOW_CORE_TEST_VAR"), "hello");
    PortableUnsetenv("FLOW_CORE_TEST_VAR");
}

TEST(EnvTest, GetVarReturnsEmptyForMissing)
{
    auto factory = std::make_shared<NodeFactory>();
    auto env     = Env::Create(factory);
    PortableUnsetenv("FLOW_CORE_NONEXISTENT_VAR_XYZ");
    EXPECT_EQ(env->GetVar("FLOW_CORE_NONEXISTENT_VAR_XYZ"), "");
}

TEST(EnvTest, AddTaskExecutes)
{
    auto factory = std::make_shared<NodeFactory>();
    auto env     = Env::Create(factory);

    std::atomic<int> counter{0};
    for (int i = 0; i < 64; ++i)
    {
        env->AddTask([&] { counter.fetch_add(1); });
    }
    env->Wait();
    EXPECT_EQ(counter.load(), 64);
}

TEST(EnvTest, AddSequenceTaskExecutes)
{
    auto factory = std::make_shared<NodeFactory>();
    auto env     = Env::Create(factory);

    std::atomic<int> counter{0};
    env->AddSequenceTask<int>(0, 32, [&](int) { counter.fetch_add(1); });
    env->Wait();
    EXPECT_EQ(counter.load(), 32);
}

TEST(EnvTest, AddLoopTaskExecutes)
{
    auto factory = std::make_shared<NodeFactory>();
    auto env     = Env::Create(factory);

    std::atomic<int> counter{0};
    env->AddLoopTask<int>(0, 100, [&](int) { counter.fetch_add(1); }, 4);
    env->Wait();
    EXPECT_EQ(counter.load(), 100);
}

TEST(EnvTest, AddBlocksTaskExecutes)
{
    auto factory = std::make_shared<NodeFactory>();
    auto env     = Env::Create(factory);

    std::atomic<int> sum{0};
    env->AddBlocksTask<int>(0, 100, [&](int start, int end) { sum.fetch_add(end - start); }, 4);
    env->Wait();
    EXPECT_EQ(sum.load(), 100);
}
