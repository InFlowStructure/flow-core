#include <gtest/gtest.h>

#include "flow/core/Env.hpp"
#include "flow/core/Graph.hpp"
#include "flow/core/Node.hpp"
#include "flow/core/NodeData.hpp"
#include "flow/core/NodeFactory.hpp"

using namespace flow;

namespace
{
auto factory = std::make_shared<NodeFactory>();
auto env     = Env::Create(factory);

struct TestNode : public Node
{
    TestNode() : Node(UUID{}, TypeName_v<TestNode>, "Test", env)
    {
        AddInput<int>("in", "");
        AddInput<int>("other_in", "");
        AddOutput<int>("out", "");
        AddOutput<int>("other_out", "");
    }

    void Compute() override
    {
        if (auto data = GetInputData<int>("in"))
        {
            SetOutputData("out", std::move(data));
        }
        if (auto data = GetInputData<int>("other_in"))
        {
            SetOutputData("other_out", std::move(data));
        }
    }
};
} // namespace

TEST(GraphTest, Construction) { ASSERT_NO_THROW(auto graph = std::make_shared<Graph>("test", env)); }

TEST(GraphTest, AddNodes)
{
    auto graph = std::make_shared<Graph>("test", env);
    auto node1 = std::make_shared<::TestNode>();
    auto node2 = std::make_shared<::TestNode>();

    EXPECT_EQ(graph->Size(), 0);

    graph->AddNode(node1);

    EXPECT_EQ(graph->Size(), 1);

    graph->AddNode(node2);

    EXPECT_EQ(graph->Size(), 2);
}

TEST(GraphTest, RemoveNodes)
{
    auto graph = std::make_shared<Graph>("test", env);
    auto node1 = std::make_shared<::TestNode>();
    auto node2 = std::make_shared<::TestNode>();

    graph->AddNode(node1);
    graph->AddNode(node2);

    EXPECT_EQ(graph->Size(), 2);

    graph->RemoveNode(node1);

    EXPECT_EQ(graph->Size(), 1);

    graph->RemoveNodeByID(node2->ID());

    EXPECT_EQ(graph->Size(), 0);
}

TEST(GraphTest, ConnectNodes)
{
    auto graph = std::make_shared<Graph>("test", env);
    auto node1 = std::make_shared<::TestNode>();
    auto node2 = std::make_shared<::TestNode>();

    graph->AddNode(node1);
    graph->AddNode(node2);

    EXPECT_EQ(graph->ConnectionCount(), 0);

    ASSERT_NO_THROW(graph->ConnectNodes(node1->ID(), "out", node2->ID(), "in"));

    EXPECT_EQ(graph->ConnectionCount(), 1);
}

TEST(GraphTest, DisconnectNodes)
{
    auto graph = std::make_shared<Graph>("test", env);
    auto node1 = std::make_shared<::TestNode>();
    auto node2 = std::make_shared<::TestNode>();

    graph->AddNode(node1);
    graph->AddNode(node2);
    graph->ConnectNodes(node1->ID(), "out", node2->ID(), "in");

    EXPECT_EQ(graph->ConnectionCount(), 1);

    ASSERT_NO_THROW(graph->DisconnectNodes(node1->ID(), "out", node2->ID(), "in"));

    EXPECT_EQ(graph->ConnectionCount(), 0);
}

TEST(GraphTest, PropagateConnectionData)
{
    auto graph = std::make_shared<Graph>("test", env);
    auto node1 = std::make_shared<::TestNode>();
    auto node2 = std::make_shared<::TestNode>();

    graph->AddNode(node1);
    graph->AddNode(node2);
    graph->ConnectNodes(node1->ID(), "out", node2->ID(), "in");

    EXPECT_EQ(node1->GetOutputData<int>("out"), nullptr);
    EXPECT_EQ(node1->GetOutputData<int>("other_out"), nullptr);

    ASSERT_NO_THROW(node1->SetInputData("in", MakeNodeData<int>(101)));

    EXPECT_NE(node1->GetOutputData<int>("out"), nullptr);
    EXPECT_EQ(node1->GetOutputData<int>("other_out"), nullptr);

    env->Wait();

    EXPECT_NE(node2->GetInputData<int>("in"), nullptr);
    EXPECT_EQ(node2->GetInputData<int>("other_in"), nullptr);

    graph->ConnectNodes(node1->ID(), "other_out", node2->ID(), "other_in");

    ASSERT_NO_THROW(node1->SetInputData("other_in", MakeNodeData<int>(202)));

    env->Wait();

    EXPECT_NE(node2->GetInputData<int>("in"), nullptr);
    EXPECT_NE(node2->GetInputData<int>("other_in"), nullptr);

    EXPECT_EQ(node2->GetInputData<int>("in")->Get(), 101);
    EXPECT_EQ(node2->GetInputData<int>("other_in")->Get(), 202);
}

TEST(GraphTest, DistinguishNodes)
{
    auto graph = std::make_shared<Graph>("test", env);
    auto node1 = std::make_shared<::TestNode>();
    auto node2 = std::make_shared<::TestNode>();
    auto node3 = std::make_shared<::TestNode>();

    graph->AddNode(node1);
    graph->AddNode(node2);
    graph->AddNode(node3);

    // All nodes are orphans, no connections made yet.
    {
        const auto& source_nodes = graph->GetSourceNodes();
        const auto& leaf_nodes   = graph->GetLeafNodes();
        const auto& orphan_nodes = graph->GetOrphanNodes();

        EXPECT_TRUE(source_nodes.empty());
        EXPECT_TRUE(leaf_nodes.empty());
        EXPECT_EQ(orphan_nodes.size(), 3);
    }

    graph->ConnectNodes(node1->ID(), "out", node2->ID(), "in");

    // A connection has been made, 2 nodes should no longer be orphaned
    {
        const auto& source_nodes = graph->GetSourceNodes();
        const auto& leaf_nodes   = graph->GetLeafNodes();
        const auto& orphan_nodes = graph->GetOrphanNodes();

        EXPECT_EQ(source_nodes.size(), 1);
        EXPECT_EQ(leaf_nodes.size(), 1);
        EXPECT_EQ(orphan_nodes.size(), 1);
    }

    auto node4 = std::make_shared<::TestNode>();
    graph->AddNode(node4);

    // New nodes are always orphans
    {
        const auto& source_nodes = graph->GetSourceNodes();
        const auto& leaf_nodes   = graph->GetLeafNodes();
        const auto& orphan_nodes = graph->GetOrphanNodes();

        EXPECT_EQ(source_nodes.size(), 1);
        EXPECT_EQ(leaf_nodes.size(), 1);
        EXPECT_EQ(orphan_nodes.size(), 2);
    }

    graph->ConnectNodes(node1->ID(), "out", node4->ID(), "in");

    // New connection made, leaves should increase, and orphans should decrease.
    {
        const auto& source_nodes = graph->GetSourceNodes();
        const auto& leaf_nodes   = graph->GetLeafNodes();
        const auto& orphan_nodes = graph->GetOrphanNodes();

        EXPECT_EQ(source_nodes.size(), 1);
        EXPECT_EQ(leaf_nodes.size(), 2);
        EXPECT_EQ(orphan_nodes.size(), 1);
    }
}
