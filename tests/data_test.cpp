// Copyright (c) 2024, Cisco Systems, Inc.
// All rights reserved.

#include "flow/core/Env.hpp"
#include "flow/core/NodeData.hpp"
#include "flow/core/NodeFactory.hpp"

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

using namespace flow;

TEST(Data, Construction)
{
    auto data = MakeNodeData<int>(101);
    EXPECT_EQ(data->Get(), 101);
}

TEST(Data, Copy)
{
    auto data = MakeNodeData<int>(101);
    auto x    = data->Get();
    EXPECT_EQ(x, 101);
}

TEST(Data, Move)
{
    auto data = MakeNodeData<int>(101);
    auto x    = std::move(data);
    EXPECT_EQ(x->Get(), 101);
}

TEST(Data, Get)
{
    auto data = MakeNodeData<int>(101);
    auto x    = data->Get();
    EXPECT_EQ(x, 101);
}
