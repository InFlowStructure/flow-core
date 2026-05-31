// Copyright (c) 2024, Cisco Systems, Inc.
// All rights reserved.

#include "flow/core/IndexableName.hpp"
#include "flow/core/NodeData.hpp"
#include "flow/core/Port.hpp"

#include <gtest/gtest.h>

using namespace flow;

TEST(PortTest, ConstructorStoresFields)
{
    Port p(IndexableName{"k"}, "caption", "int", MakeNodeData<int>(42), false, 3);
    EXPECT_EQ(p.GetKey(), IndexableName{"k"});
    EXPECT_EQ(p.GetVarName(), "k");
    EXPECT_EQ(p.GetCaption(), "caption");
    EXPECT_EQ(p.GetDataType(), "int");
    EXPECT_FALSE(p.IsRequired());
    EXPECT_EQ(p.Index(), 3u);
    EXPECT_FALSE(p.IsConnected());
}

TEST(PortTest, ConnectDisconnectStateTransitions)
{
    Port p(IndexableName{"k"}, "", "int", nullptr, false, 0);

    EXPECT_TRUE(p.Connect());
    EXPECT_TRUE(p.IsConnected());

    // Reconnecting an already-connected port is a no-op
    EXPECT_FALSE(p.Connect());

    EXPECT_TRUE(p.Disconnect());
    EXPECT_FALSE(p.IsConnected());

    // Disconnecting an already-disconnected port is a no-op
    EXPECT_FALSE(p.Disconnect());
}

TEST(PortTest, SetDataReplacesStorage)
{
    Port p(IndexableName{"k"}, "", "int", nullptr, false, 0);
    EXPECT_EQ(p.GetData(), nullptr);

    p.SetData(MakeNodeData<int>(7));
    ASSERT_NE(p.GetData(), nullptr);
    auto typed = CastNodeData<int>(p.GetData());
    ASSERT_NE(typed, nullptr);
    EXPECT_EQ(typed->Get(), 7);
}

TEST(PortTest, SetDataPreservesIdentityOnSameType)
{
    auto initial = MakeNodeData<int>(1);
    Port p(IndexableName{"k"}, "", "int", initial, false, 0);
    auto prev_raw = p.GetData().get();

    // Same type, no output flag: identity should be preserved via FromPointer
    p.SetData(MakeNodeData<int>(99));
    EXPECT_EQ(p.GetData().get(), prev_raw);
    auto typed = CastNodeData<int>(p.GetData());
    ASSERT_NE(typed, nullptr);
    EXPECT_EQ(typed->Get(), 99);
}

TEST(PortTest, SetDataReplacesWhenOutputFlagSet)
{
    Port p(IndexableName{"k"}, "", "int", MakeNodeData<int>(1), false, 0);
    auto prev_raw = p.GetData().get();
    p.SetData(MakeNodeData<int>(99), /*output=*/true);
    // With output=true, the SharedNodeData is replaced (different identity)
    EXPECT_NE(p.GetData().get(), prev_raw);
}

TEST(PortTest, SetDataRejectsNullForRequired)
{
    auto initial = MakeNodeData<int>(5);
    Port p(IndexableName{"k"}, "", "int&", initial, /*required=*/true, 0);
    p.SetData(nullptr);
    // Required port keeps existing data
    EXPECT_NE(p.GetData(), nullptr);
}

TEST(PortTest, OnSetDataFiresWhenBound)
{
    Port p(IndexableName{"k"}, "", "int", nullptr, false, 0);

    int call_count = 0;
    p.OnSetData    = [&](const IndexableName&, const SharedNodeData&, bool) { ++call_count; };

    p.SetData(MakeNodeData<int>(1));
    EXPECT_EQ(call_count, 1);

    p.SetData(MakeNodeData<int>(2));
    EXPECT_EQ(call_count, 2);
}

TEST(PortTest, SetCaptionUpdates)
{
    Port p(IndexableName{"k"}, "old", "int", nullptr, false, 0);
    p.SetCaption("new");
    EXPECT_EQ(p.GetCaption(), "new");
}

TEST(PortTest, GetDataTypeReturnsDataTypeWhenSet)
{
    Port p(IndexableName{"k"}, "", "int", MakeNodeData<int>(1), false, 0);
    EXPECT_EQ(p.GetDataType(), "int");
}

TEST(PortTest, LessComparesByIndex)
{
    auto a = std::make_shared<Port>(IndexableName{"a"}, "", "int", nullptr, false, 0);
    auto b = std::make_shared<Port>(IndexableName{"b"}, "", "int", nullptr, false, 5);
    std::less<SharedPort> cmp_shared;
    EXPECT_TRUE(cmp_shared(a, b));
    EXPECT_FALSE(cmp_shared(b, a));

    std::less<Port> cmp;
    EXPECT_TRUE(cmp(*a, *b));
    EXPECT_FALSE(cmp(*b, *a));
}
