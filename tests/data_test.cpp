// Copyright (c) 2024, Cisco Systems, Inc.
// All rights reserved.

#include "flow/core/Env.hpp"
#include "flow/core/NodeData.hpp"
#include "flow/core/NodeFactory.hpp"

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <memory>

using namespace flow;

TEST(Data, Construction)
{
    auto data = flow::NodeData<int>(101);
    EXPECT_EQ(data.Get(), 101);

    auto ptr_data = flow::NodeData<std::unique_ptr<int>>(std::make_unique<int>(202));
    EXPECT_NE(ptr_data.Get(), nullptr);
    EXPECT_EQ(*ptr_data.Get(), 202);
}

TEST(Data, Copy)
{
    auto data     = flow::NodeData<int>(101);
    auto cpy_data = data;
    EXPECT_EQ(cpy_data.Get(), 101);
}

TEST(Data, Move)
{
    auto data = flow::NodeData<int>(101);
    auto x    = std::move(data);
    EXPECT_EQ(x.Get(), 101);
}

TEST(Data, Get)
{
    auto data = flow::NodeData<int>(101);
    EXPECT_EQ(data.Get(), 101);
}
