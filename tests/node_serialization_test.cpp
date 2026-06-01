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

namespace ns
{
struct PlainNode : public Node
{
    PlainNode(const UUID& id, std::string_view name, std::shared_ptr<Env> env)
        : Node(id, TypeName_v<PlainNode>, name, std::move(env))
    {
        AddInput<int>("in", "");
        AddOutput<int>("out", "");
    }
    void Compute() override {}
};

int plus_one(int v) { return v + 1; }
} // namespace ns

class NodeSerializationTest : public ::testing::Test
{
  protected:
    std::shared_ptr<NodeFactory> factory = std::make_shared<NodeFactory>();
    std::shared_ptr<Env> env             = Env::Create(factory);
};

TEST_F(NodeSerializationTest, SaveProducesIdClassName)
{
    ns::PlainNode node(UUID{}, "friendly", env);
    auto j = node.Save();
    ASSERT_TRUE(j.contains("id"));
    ASSERT_TRUE(j.contains("class"));
    ASSERT_TRUE(j.contains("name"));
    EXPECT_EQ(j["name"], "friendly");
}

TEST_F(NodeSerializationTest, RestoreReadsFields)
{
    ns::PlainNode original(UUID{}, "first", env);
    auto j = original.Save();

    ns::PlainNode restored(UUID{}, "default", env);
    ASSERT_NO_THROW(restored.Restore(j));
    EXPECT_EQ(restored.GetName(), "first");
    EXPECT_EQ(restored.GetClass(), std::string{TypeName_v<ns::PlainNode>});
}

TEST_F(NodeSerializationTest, RestoreThrowsOnMissingFields)
{
    ns::PlainNode node(UUID{}, "x", env);
    json bad = {{"id", "00000000-0000-0000-0000-000000000000"}};
    EXPECT_THROW(node.Restore(bad), std::runtime_error);
}

TEST_F(NodeSerializationTest, FunctionNodeSaveRestoreRoundTrip)
{
    using FN = FunctionNode<decltype(ns::plus_one), ns::plus_one>;
    FN node(UUID{}, "plus", env);

    node.SetInputData("a", MakeNodeData<int>(7), false);
    auto j = node.Save();
    ASSERT_TRUE(j.contains("inputs"));

    FN restored(UUID{}, "plus2", env);
    ASSERT_NO_THROW(restored.Restore(j));
    auto in = restored.GetInputData<int>("a");
    ASSERT_NE(in, nullptr);
    EXPECT_EQ(in->Get(), 7);
}

TEST_F(NodeSerializationTest, SetInputDataWithComputeFalseDoesNotInvokeCompute)
{
    int compute_count = 0;

    struct Counter : public Node
    {
        Counter(std::shared_ptr<Env> env, int& c) : Node(UUID{}, TypeName_v<Counter>, "c", std::move(env)), _c{c}
        {
            AddInput<int>("in", "");
        }
        void Compute() override { ++_c; }
        int& _c;
    };

    auto n = std::make_shared<Counter>(env, compute_count);
    n->SetInputData("in", MakeNodeData<int>(1), /*compute=*/false);
    EXPECT_EQ(compute_count, 0);

    n->SetInputData("in", MakeNodeData<int>(2), /*compute=*/true);
    EXPECT_EQ(compute_count, 1);
}

TEST_F(NodeSerializationTest, EmitUpdateWithoutGraphIsSafeAfterBinding)
{
    struct Emitter : public Node
    {
        Emitter(std::shared_ptr<Env> env) : Node(UUID{}, TypeName_v<Emitter>, "e", std::move(env))
        {
            AddOutput<int>("out", "");
            _propagate_output_update = [](const UUID&, const IndexableName&, SharedNodeData) {};
        }
        void Compute() override {}
        void TriggerEmit() { EmitUpdate("out", MakeNodeData<int>(42)); }
    };

    auto n = std::make_shared<Emitter>(env);

    int captured = 0;
    n->OnEmitOutput.Bind("c", [&](const UUID&, const IndexableName&, const SharedNodeData& d) {
        if (auto t = CastNodeData<int>(d)) captured = t->Get();
    });
    n->TriggerEmit();
    EXPECT_EQ(captured, 42);
}

TEST_F(NodeSerializationTest, SetOutputDataWithoutEmit)
{
    struct E : public Node
    {
        E(std::shared_ptr<Env> env) : Node(UUID{}, TypeName_v<E>, "e", std::move(env)) { AddOutput<int>("o", ""); }
        void Compute() override {}
    };
    auto n = std::make_shared<E>(env);

    int emitted = 0;
    n->OnEmitOutput.Bind("c", [&](const UUID&, const IndexableName&, const SharedNodeData&) { ++emitted; });
    n->SetOutputData("o", MakeNodeData<int>(1), /*emit=*/false);
    EXPECT_EQ(emitted, 0);
}
