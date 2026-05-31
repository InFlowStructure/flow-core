// Copyright (c) 2024, Cisco Systems, Inc.
// All rights reserved.

#include "flow/core/Env.hpp"
#include "flow/core/NodeData.hpp"
#include "flow/core/NodeFactory.hpp"
#include "flow/core/TypeConversion.hpp"
#include "flow/core/TypeName.hpp"

#include <gtest/gtest.h>

#include <any>
#include <string>

using namespace flow;

TEST(TypeConversionTest, NullDataReturnsNull)
{
    TypeRegistry r;
    auto out = r.Convert(nullptr, TypeName_v<int>);
    EXPECT_EQ(out, nullptr);
}

TEST(TypeConversionTest, SameTypeReturnsOriginal)
{
    TypeRegistry r;
    auto d   = MakeNodeData<int>(7);
    auto out = r.Convert(d, TypeName_v<int>);
    EXPECT_EQ(out, d);
}

TEST(TypeConversionTest, AnyTargetReturnsOriginal)
{
    TypeRegistry r;
    auto d   = MakeNodeData<int>(7);
    auto out = r.Convert(d, TypeName_v<std::any>);
    EXPECT_EQ(out, d);
}

TEST(TypeConversionTest, NoConversionRegisteredReturnsOriginal)
{
    TypeRegistry r;
    auto d   = MakeNodeData<int>(7);
    auto out = r.Convert(d, TypeName_v<double>);
    EXPECT_EQ(out, d);
}

TEST(TypeConversionTest, RegisteredConversionConvertsData)
{
    TypeRegistry r;
    r.RegisterUnidirectionalConversion<int, double>();
    auto d   = MakeNodeData<int>(42);
    auto out = r.Convert(d, TypeName_v<double>);
    ASSERT_NE(out, nullptr);
    auto typed = CastNodeData<double>(out);
    ASSERT_NE(typed, nullptr);
    EXPECT_DOUBLE_EQ(typed->Get(), 42.0);
}

TEST(TypeConversionTest, BidirectionalConversion)
{
    TypeRegistry r;
    r.RegisterBidirectionalConversion<int, double>();

    EXPECT_TRUE(r.IsConvertible(TypeName_v<int>, TypeName_v<double>));
    EXPECT_TRUE(r.IsConvertible(TypeName_v<double>, TypeName_v<int>));

    auto d   = MakeNodeData<double>(3.5);
    auto out = r.Convert(d, TypeName_v<int>);
    ASSERT_NE(out, nullptr);
    auto typed = CastNodeData<int>(out);
    ASSERT_NE(typed, nullptr);
    EXPECT_EQ(typed->Get(), 3);
}

TEST(TypeConversionTest, IsConvertibleSameType)
{
    TypeRegistry r;
    EXPECT_TRUE(r.IsConvertible(TypeName_v<int>, TypeName_v<int>));
}

TEST(TypeConversionTest, IsConvertibleToAny)
{
    TypeRegistry r;
    EXPECT_TRUE(r.IsConvertible(TypeName_v<int>, TypeName_v<std::any>));
}

TEST(TypeConversionTest, IsConvertibleFalseWhenSourceUnknown)
{
    TypeRegistry r;
    EXPECT_FALSE(r.IsConvertible("UnknownTypeX", TypeName_v<int>));
}

TEST(TypeConversionTest, IsConvertibleFalseWhenTargetUnknown)
{
    TypeRegistry r;
    r.RegisterUnidirectionalConversion<int, double>();
    EXPECT_FALSE(r.IsConvertible(TypeName_v<int>, TypeName_v<std::string>));
}

TEST(NodeFactoryConversionTest, EnvPreRegistersIntegerMesh)
{
    auto factory = std::make_shared<NodeFactory>();
    auto env     = Env::Create(factory);

    EXPECT_TRUE((factory->IsConvertible<int, double>()));
    EXPECT_TRUE((factory->IsConvertible<int, std::int64_t>()));
    EXPECT_TRUE((factory->IsConvertible<float, int>()));
    EXPECT_TRUE(factory->IsConvertible<int>(TypeName_v<double>));
}

TEST(NodeFactoryConversionTest, ConvertHelperReturnsTypedData)
{
    auto factory = std::make_shared<NodeFactory>();
    auto env     = Env::Create(factory);

    auto d   = MakeNodeData<int>(10);
    auto out = factory->Convert<double>(d);
    ASSERT_NE(out, nullptr);
    EXPECT_DOUBLE_EQ(out->Get(), 10.0);
}
