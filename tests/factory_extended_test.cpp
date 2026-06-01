// Copyright (c) 2024, Cisco Systems, Inc.
// All rights reserved.

#include "flow/core/Env.hpp"
#include "flow/core/FunctionNode.hpp"
#include "flow/core/Node.hpp"
#include "flow/core/NodeFactory.hpp"

#include <gtest/gtest.h>

using namespace flow;

namespace fx
{
struct AlphaNode : public Node
{
    AlphaNode(const UUID& id, std::string_view name, std::shared_ptr<Env> env)
        : Node(id, TypeName_v<AlphaNode>, name, std::move(env))
    {
    }
    void Compute() override {}
};

struct BetaNode : public Node
{
    BetaNode(const UUID& id, std::string_view name, std::shared_ptr<Env> env)
        : Node(id, TypeName_v<BetaNode>, name, std::move(env))
    {
    }
    void Compute() override {}
};

int add(int a, int b) { return a + b; }
} // namespace fx

TEST(FactoryExtendedTest, CreateUnknownClassReturnsNull)
{
    auto factory = std::make_shared<NodeFactory>();
    auto env     = Env::Create(factory);
    EXPECT_EQ(factory->CreateNode("NotRegistered", UUID{}, "x", env), nullptr);
}

TEST(FactoryExtendedTest, RegisterFiresEvent)
{
    auto factory = std::make_shared<NodeFactory>();
    int hits     = 0;
    factory->OnNodeClassRegistered.Bind("h", [&](std::string_view) { ++hits; });
    factory->RegisterNodeClass<fx::AlphaNode>("Test");
    EXPECT_EQ(hits, 1);
}

TEST(FactoryExtendedTest, UnregisterFiresEvent)
{
    auto factory = std::make_shared<NodeFactory>();
    factory->RegisterNodeClass<fx::AlphaNode>("Test");
    int hits = 0;
    factory->OnNodeClassUnregistered.Bind("h", [&](std::string_view) { ++hits; });
    factory->UnregisterNodeClass<fx::AlphaNode>("Test");
    EXPECT_EQ(hits, 1);
}

TEST(FactoryExtendedTest, GetFriendlyNameReturnsRegisteredName)
{
    auto factory = std::make_shared<NodeFactory>();
    factory->RegisterNodeClass<fx::AlphaNode>("Test", "Friendly Alpha");
    EXPECT_EQ(factory->GetFriendlyName(std::string{TypeName_v<fx::AlphaNode>}), "Friendly Alpha");
}

TEST(FactoryExtendedTest, GetFriendlyNameReturnsArgumentForUnknown)
{
    auto factory = std::make_shared<NodeFactory>();
    EXPECT_EQ(factory->GetFriendlyName("Nope"), "Nope");
}

TEST(FactoryExtendedTest, GetCategoriesContainsRegistered)
{
    auto factory = std::make_shared<NodeFactory>();
    factory->RegisterNodeClass<fx::AlphaNode>("CatA");
    factory->RegisterNodeClass<fx::BetaNode>("CatB");

    const auto& cats = factory->GetCategories();
    EXPECT_GE(cats.size(), 2u);
}

TEST(FactoryExtendedTest, CategoryRegistrationViaHelper)
{
    auto factory = std::make_shared<NodeFactory>();
    auto env     = Env::Create(factory);
    Category cat("Helpers");
    cat.RegisterNodeClass<fx::AlphaNode>(factory);

    auto node = factory->CreateNode(std::string{TypeName_v<fx::AlphaNode>}, UUID{}, "x", env);
    ASSERT_NE(node, nullptr);

    factory->UnregisterCategory(cat);
    EXPECT_EQ(factory->CreateNode(std::string{TypeName_v<fx::AlphaNode>}, UUID{}, "x", env), nullptr);
}

TEST(FactoryExtendedTest, ParentCategoryPrefixesName)
{
    Category parent("Root");
    Category child(parent, "Leaf");
    // Constructor builds name "Root::Leaf"; can't read it directly but
    // exercise via factory registration roundtrip.
    auto factory = std::make_shared<NodeFactory>();
    child.RegisterNodeClass<fx::AlphaNode>(factory);

    bool found = false;
    for (const auto& [cat_name, _] : factory->GetCategories())
    {
        if (cat_name == "Root::Leaf")
        {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found);

    factory->UnregisterCategory(child);
}

TEST(FactoryExtendedTest, UnregisterCategoryRemovesAllClasses)
{
    auto factory = std::make_shared<NodeFactory>();
    auto env     = Env::Create(factory);
    Category cat("Cleanup");
    cat.RegisterNodeClass<fx::AlphaNode>(factory);
    cat.RegisterNodeClass<fx::BetaNode>(factory);

    ASSERT_NE(factory->CreateNode(std::string{TypeName_v<fx::AlphaNode>}, UUID{}, "x", env), nullptr);
    ASSERT_NE(factory->CreateNode(std::string{TypeName_v<fx::BetaNode>}, UUID{}, "x", env), nullptr);

    factory->UnregisterCategory(cat);

    EXPECT_EQ(factory->CreateNode(std::string{TypeName_v<fx::AlphaNode>}, UUID{}, "x", env), nullptr);
    EXPECT_EQ(factory->CreateNode(std::string{TypeName_v<fx::BetaNode>}, UUID{}, "x", env), nullptr);
}

TEST(FactoryExtendedTest, RegisterFunctionCreatesNode)
{
    auto factory = std::make_shared<NodeFactory>();
    auto env     = Env::Create(factory);
    factory->RegisterFunction<decltype(fx::add), fx::add>("Math", "add");

    auto type_name = std::string{TypeName_v<FunctionNode<decltype(fx::add), fx::add>>};
    auto node      = factory->CreateNode(type_name, UUID{}, "add", env);
    ASSERT_NE(node, nullptr);
    EXPECT_EQ(node->GetInputPorts().size(), 2u);
}

TEST(FactoryExtendedTest, IsConvertibleTypedSameType)
{
    auto factory = std::make_shared<NodeFactory>();
    auto env     = Env::Create(factory);
    EXPECT_TRUE((factory->IsConvertible<int, int>()));
}
