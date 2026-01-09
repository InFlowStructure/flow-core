// Copyright (c) 2024, Cisco Systems, Inc.
// All rights reserved.

#include "flow/core/Env.hpp"
#include "flow/core/FunctionNode.hpp"
#include "flow/core/Node.hpp"
#include "flow/core/NodeData.hpp"
#include "flow/core/NodeFactory.hpp"

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

using namespace flow;

namespace test
{
auto factory = NodeFactory::Create();
auto env     = Env::Create(factory);
} // namespace test

namespace NodeTest
{

struct TestNode : public Node
{
    TestNode() : Node(UUID{}, TypeName_v<TestNode>, "Test", test::env) {}
    void Compute() override
    {
        if (auto data = GetInputData<int>("in"))
        {
            SetOutputData("out", std::move(data));
        }
    }

    template<typename T>
    void AddInput(std::string_view key, const std::string& caption, SharedNodeData data = nullptr)
    {
        return Node::AddInput<T>(key, caption, std::move(data));
    }

    template<typename T>
    void AddRequiredInput(std::string_view key, const std::string& caption, SharedNodeData data = nullptr)
    {
        return Node::AddRequiredInput<T>(key, caption, std::move(data));
    }

    template<typename T>
    void AddOutput(std::string_view key, const std::string& caption, SharedNodeData data = nullptr)
    {
        return Node::AddOutput<T>(key, caption, std::move(data));
    }
};
} // namespace NodeTest

TEST(Node, Construction)
{
    ASSERT_NO_THROW(NodeTest::TestNode());

    NodeTest::TestNode node;
    EXPECT_EQ(node.GetClass(), "NodeTest::TestNode");
    EXPECT_EQ(node.GetName(), "Test");
    EXPECT_EQ(node.GetEnv(), test::env);
}

TEST(Node, AddInputPorts)
{
    NodeTest::TestNode node;

    ASSERT_TRUE(node.GetInputPorts().empty());

    node.AddInput<int>("1", "Caption 1");
    ASSERT_TRUE(node.GetInputPorts().size() == 1);

    {
        const auto& port = node.GetInputPort("1");
        ASSERT_NE(port, nullptr);
        ASSERT_EQ(port->GetVarName(), "1");
        ASSERT_EQ(port->GetData(), nullptr);

        const auto& data = node.GetInputData("1");
        ASSERT_EQ(port->GetData(), data);
        ASSERT_EQ(data, nullptr);
    }

    node.AddInput<int>("2", "Caption 2", MakeNodeData<int>(101));
    ASSERT_TRUE(node.GetInputPorts().size() == 2);

    {
        const auto& port = node.GetInputPort("2");
        ASSERT_NE(port, nullptr);
        ASSERT_EQ(port->GetVarName(), "2");
        ASSERT_NE(port->GetData(), nullptr);

        const auto& data = node.GetInputData<int>("2");
        ASSERT_EQ(port->GetData(), data);
        ASSERT_NE(data, nullptr);
        ASSERT_EQ(data->Get(), 101);
    }
}

TEST(Node, AddOutputPorts)
{
    NodeTest::TestNode node;

    ASSERT_TRUE(node.GetOutputPorts().empty());

    node.AddOutput<int>("1", "Caption 1");
    ASSERT_TRUE(node.GetOutputPorts().size() == 1);

    {
        const auto& port = node.GetOutputPort("1");
        ASSERT_NE(port, nullptr);
        ASSERT_EQ(port->GetVarName(), "1");
        ASSERT_EQ(port->GetData(), nullptr);

        const auto& data = node.GetOutputData("1");
        ASSERT_EQ(port->GetData(), data);
        ASSERT_EQ(data, nullptr);
    }

    node.AddOutput<int>("2", "Caption 2", MakeNodeData<int>(101));
    ASSERT_TRUE(node.GetOutputPorts().size() == 2);

    {
        const auto& port = node.GetOutputPort("2");
        ASSERT_NE(port, nullptr);
        ASSERT_EQ(port->GetVarName(), "2");
        ASSERT_NE(port->GetData(), nullptr);

        const auto& data = node.GetOutputData<int>("2");
        ASSERT_EQ(port->GetData(), data);
        ASSERT_NE(data, nullptr);
        ASSERT_EQ(data->Get(), 101);
    }
}

TEST(Node, Compute)
{
    NodeTest::TestNode node;
    node.AddInput<int>("in", "");
    node.AddOutput<int>("out", "");

    EXPECT_EQ(node.GetInputData<int>("in"), nullptr);

    ASSERT_NO_THROW(node.SetInputData("in", MakeNodeData(101)));

    EXPECT_NE(node.GetInputData<int>("in"), nullptr);
    EXPECT_EQ(node.GetInputData<int>("in")->Get(), 101);

    EXPECT_NE(node.GetOutputData<int>("out"), nullptr);
    EXPECT_EQ(node.GetOutputData<int>("out")->Get(), 101);
}

void void_test_method(int) {}
int return_test_method(int i) { return i; }
int return_ref_test_method(int& i) { return i; }

struct TestData
{
};
void custom_type_test_method(const TestData&) {}

struct TestFunctor
{
    void operator()(int) {}
};

TEST(Node, WrapFunctions)
{
    FunctionNode<void_test_method> void_test_method_node({}, "void_test_method", test::env);
    FunctionNode<return_test_method> return_test_method_node({}, "return_test_method", test::env);
    FunctionNode<return_ref_test_method> return_ref_test_method_node({}, "return_ref_test_method", test::env);
    FunctionNode<custom_type_test_method> custom_type_test_method_node({}, "custom_type_test_method", test::env);
    FunctionNode<TestFunctor{}> functor_node({}, "functor", test::env);

    ASSERT_EQ(void_test_method_node.GetInputPorts().size(), 1);
    ASSERT_EQ(return_test_method_node.GetInputPorts().size(), 1);
    ASSERT_TRUE(return_ref_test_method_node.GetInputPorts().empty());
    ASSERT_EQ(custom_type_test_method_node.GetInputPorts().size(), 1);
    ASSERT_EQ(functor_node.GetInputPorts().size(), 1);

    ASSERT_TRUE(void_test_method_node.GetOutputPorts().empty());
    ASSERT_EQ(return_test_method_node.GetOutputPorts().size(), 1);
    ASSERT_EQ(return_ref_test_method_node.GetOutputPorts().size(), 2);
    ASSERT_TRUE(custom_type_test_method_node.GetOutputPorts().empty());
    ASSERT_TRUE(functor_node.GetOutputPorts().empty());
}

TEST(Node, Save)
{
    NodeTest::TestNode node;

    auto x = node.Save();
    EXPECT_EQ(x, json({{"id", node.ID()}, {"class", node.GetClass()}, {"name", node.GetName()}, {"inputs", {}}}) );
}
