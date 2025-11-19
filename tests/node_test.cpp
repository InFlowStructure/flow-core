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
auto factory = std::make_shared<NodeFactory>();
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

TEST(NodeTest, Construction)
{
    ASSERT_NO_THROW(NodeTest::TestNode());

    NodeTest::TestNode node;
    EXPECT_EQ(node.GetClass(), "NodeTest::TestNode");
    EXPECT_EQ(node.GetName(), "Test");
    EXPECT_EQ(node.GetEnv(), test::env);
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

void void_test_method(int) {}
int return_test_method(int i) { return i; }
int return_ref_test_method(int& i) { return i; }

struct TestData
{
};
void custom_type(const TestData&) {}

TEST(NodeTest, WrapFunctions)
{
    FunctionNode<decltype(void_test_method), void_test_method> void_node({}, "void_test_method", test::env);
    FunctionNode<decltype(return_test_method), return_test_method> return_node({}, "return_test_method", test::env);
    FunctionNode<decltype(return_ref_test_method), return_ref_test_method> return_ref_node({}, "return_ref_test_method",
                                                                                           test::env);
    FunctionNode<decltype(custom_type), custom_type> custom_type_node({}, "custom_type", test::env);

    ASSERT_EQ(void_node.GetInputPorts().size(), 1);
    ASSERT_EQ(return_node.GetInputPorts().size(), 1);
    ASSERT_TRUE(return_ref_node.GetInputPorts().empty());

    ASSERT_TRUE(void_node.GetOutputPorts().empty());
    ASSERT_EQ(return_node.GetOutputPorts().size(), 1);
    ASSERT_EQ(return_ref_node.GetOutputPorts().size(), 2);
}

struct CustomSerializingNode : public Node
{
    CustomSerializingNode() : Node(UUID{}, TypeName_v<CustomSerializingNode>, "CustomNode", test::env) {}

    void Compute() override {}

    template<typename T>
    void AddInput(std::string_view key, const std::string& caption, SharedNodeData data = nullptr)
    {
        return Node::AddInput<T>(key, caption, std::move(data));
    }

    template<typename T>
    void AddOutput(std::string_view key, const std::string& caption, SharedNodeData data = nullptr)
    {
        return Node::AddOutput<T>(key, caption, std::move(data));
    }

    json SaveInputs() const override
    {
        json inputs_json = json::object();
        for (const auto& [key, port] : GetInputPorts())
        {
            inputs_json[std::string(key)] = {
                {"caption", port->GetCaption()},
                {"type", port->GetDataType()},
                {"required", port->IsRequired()},
                {"has_default", port->GetData() != nullptr},
            };
        }
        return inputs_json;
    }
};

TEST(NodeTest, SerializeToJSON)
{
    NodeTest::TestNode node;
    node.SetName("MyTestNode");

    // Add several input ports with different configurations
    node.AddInput<int>("input_no_default", "Integer Input");
    node.AddInput<float>("input_with_default", "Float Input", MakeNodeData<float>(3.14f));
    node.AddInput<std::string>("input_string", "String Input", MakeNodeData<std::string>("hello"));

    // Add several output ports
    node.AddOutput<int>("output_result", "Result Output");
    node.AddOutput<bool>("output_status", "Status Output", MakeNodeData<bool>(true));
    node.AddOutput<std::string>("output_message", "Message Output");

    // Serialize to JSON
    json node_json = node.Save();

    // Print the JSON
    std::cout << "\n=== Base Node Serialization (No Input Serialization) ===" << std::endl;
    std::cout << "Inputs: " << node_json["inputs"] << std::endl;
    std::cout << "Note: inputs is null because base Node::SaveInputs() returns empty" << std::endl;
    std::cout << std::endl;

    // Verify basic structure
    ASSERT_TRUE(node_json.contains("id"));
    ASSERT_TRUE(node_json.contains("class"));
    ASSERT_TRUE(node_json.contains("name"));
    ASSERT_TRUE(node_json.contains("inputs"));

    ASSERT_EQ(node_json["class"], "NodeTest::TestNode");
    ASSERT_EQ(node_json["name"], "MyTestNode");
}

TEST(NodeTest, SerializeToJSONWithCustomSerialization)
{
    CustomSerializingNode node;
    node.SetName("CustomSerializingNode");

    // Add several input ports with different configurations
    node.AddInput<int>("input_no_default", "Integer Input");
    node.AddInput<float>("input_with_default", "Float Input", MakeNodeData<float>(3.14f));
    node.AddInput<std::string>("input_string", "String Input", MakeNodeData<std::string>("hello"));

    // Add several output ports
    node.AddOutput<int>("output_result", "Result Output");
    node.AddOutput<bool>("output_status", "Status Output", MakeNodeData<bool>(true));
    node.AddOutput<std::string>("output_message", "Message Output");

    // Serialize to JSON
    json node_json = node.Save();

    // Print the JSON
    std::cout << "\n=== Custom Node Serialization (With Input Serialization) ===" << std::endl;
    std::cout << node_json.dump(2) << std::endl;
    std::cout << "================================================\n" << std::endl;

    // Verify structure with custom serialization
    ASSERT_TRUE(node_json.contains("id"));
    ASSERT_TRUE(node_json.contains("class"));
    ASSERT_TRUE(node_json.contains("name"));
    ASSERT_TRUE(node_json.contains("inputs"));
    ASSERT_NE(node_json["inputs"], nullptr);

    ASSERT_EQ(node_json["class"], "CustomSerializingNode");
    ASSERT_EQ(node_json["name"], "CustomSerializingNode");

    // Verify input serialization
    ASSERT_TRUE(node_json["inputs"].contains("input_no_default"));
    ASSERT_TRUE(node_json["inputs"].contains("input_with_default"));
    ASSERT_TRUE(node_json["inputs"].contains("input_string"));

    // Check that defaults were tracked
    ASSERT_FALSE(node_json["inputs"]["input_no_default"]["has_default"]);
    ASSERT_TRUE(node_json["inputs"]["input_with_default"]["has_default"]);
    ASSERT_TRUE(node_json["inputs"]["input_string"]["has_default"]);
}
