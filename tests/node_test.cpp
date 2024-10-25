#include <gtest/gtest.h>

#include "flow/core/Env.hpp"
#include "flow/core/Node.hpp"
#include "flow/core/NodeData.hpp"
#include "flow/core/NodeFactory.hpp"

using namespace FLOW_NAMESPACE;

namespace
{
auto factory = std::make_shared<NodeFactory>();
auto env     = Env::Create(factory);
} // namespace

namespace NodeTest
{

struct TestNode : public Node
{
    TestNode() : Node(UUID{}, TypeName_v<TestNode>, "Test", env) {}
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

TEST(NodeTest, Construction)
{
    ASSERT_NO_THROW(NodeTest::TestNode());

    NodeTest::TestNode node;
    EXPECT_EQ(node.GetClass(), "NodeTest::TestNode");
    EXPECT_EQ(node.GetName(), "Test");
    EXPECT_EQ(node.GetEnv(), env);
}

TEST(NodeTest, AddInputPorts)
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
TEST(NodeTest, AddOutputPorts)
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

TEST(NodeTest, Compute)
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
