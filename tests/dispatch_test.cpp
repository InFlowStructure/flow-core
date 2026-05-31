// Copyright (c) 2024, Cisco Systems, Inc.
// All rights reserved.

#include "flow/core/Env.hpp"
#include "flow/core/Graph.hpp"
#include "flow/core/IndexableName.hpp"
#include "flow/core/Node.hpp"
#include "flow/core/NodeData.hpp"
#include "flow/core/NodeFactory.hpp"

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <atomic>
#include <stdexcept>

using namespace flow;

namespace
{
struct CountingSource : public Node
{
    CountingSource(std::shared_ptr<Env> env, std::atomic<int>& counter)
        : Node(UUID{}, TypeName_v<CountingSource>, "src", std::move(env)), _counter{counter}
    {
        AddOutput<int>("out", "");
    }

    void Compute() override
    {
        _counter.fetch_add(1);
        SetOutputData("out", MakeNodeData<int>(_counter.load()));
    }

    std::atomic<int>& _counter;
};

struct Identity : public Node
{
    Identity(std::shared_ptr<Env> env)
        : Node(UUID{}, TypeName_v<Identity>, "id", std::move(env))
    {
        AddInput<int>("in", "");
        AddOutput<int>("out", "");
    }

    Identity(const UUID& id, std::string_view name, std::shared_ptr<Env> env)
        : Node(id, TypeName_v<Identity>, name, std::move(env))
    {
        AddInput<int>("in", "");
        AddOutput<int>("out", "");
    }

    void Compute() override
    {
        if (auto d = GetInputData<int>("in"))
        {
            SetOutputData("out", MakeNodeData<int>(d->Get()));
        }
    }
};

struct Sink : public Node
{
    Sink(std::shared_ptr<Env> env, std::atomic<int>& last)
        : Node(UUID{}, TypeName_v<Sink>, "sink", std::move(env)), _last{last}
    {
        AddInput<int>("in", "");
    }

    void Compute() override
    {
        if (auto d = GetInputData<int>("in"))
        {
            _last.store(d->Get());
        }
    }

    std::atomic<int>& _last;
};

struct ThrowingNode : public Node
{
    enum class Kind { StdEx, StdString, CString, Int, Other };
    Kind kind;

    ThrowingNode(std::shared_ptr<Env> env, Kind k)
        : Node(UUID{}, TypeName_v<ThrowingNode>, "throw", std::move(env)), kind{k}
    {
    }

    void Compute() override
    {
        switch (kind)
        {
            case Kind::StdEx:    throw std::runtime_error("boom");
            case Kind::StdString: throw std::string("boom");
            case Kind::CString:  throw "boom";
            case Kind::Int:      throw 42;
            case Kind::Other:    throw 3.14;
        }
    }
};
} // namespace

class DispatchTest : public ::testing::Test
{
  protected:
    std::shared_ptr<NodeFactory> factory = std::make_shared<NodeFactory>();
    std::shared_ptr<Env> env             = Env::Create(factory);
};

TEST_F(DispatchTest, RunFiresSourceNodes)
{
    auto graph = std::make_shared<Graph>("test", env);
    std::atomic<int> counter{0};
    std::atomic<int> last{0};

    auto src  = std::make_shared<CountingSource>(env, counter);
    auto sink = std::make_shared<Sink>(env, last);

    graph->AddNode(src);
    graph->AddNode(sink);
    graph->ConnectNodes(src->ID(), "out", sink->ID(), "in");

    graph->Run();
    env->Wait();

    EXPECT_EQ(counter.load(), 1);
    EXPECT_EQ(last.load(), 1);
}

TEST_F(DispatchTest, RunPropagatesThroughChain)
{
    auto graph = std::make_shared<Graph>("test", env);
    std::atomic<int> counter{0};
    std::atomic<int> last{0};

    auto src   = std::make_shared<CountingSource>(env, counter);
    auto mid1  = std::make_shared<Identity>(env);
    auto mid2  = std::make_shared<Identity>(env);
    auto sink  = std::make_shared<Sink>(env, last);

    graph->AddNode(src);
    graph->AddNode(mid1);
    graph->AddNode(mid2);
    graph->AddNode(sink);
    graph->ConnectNodes(src->ID(), "out", mid1->ID(), "in");
    graph->ConnectNodes(mid1->ID(), "out", mid2->ID(), "in");
    graph->ConnectNodes(mid2->ID(), "out", sink->ID(), "in");

    graph->Run();
    env->Wait();

    EXPECT_EQ(last.load(), 1);
}

TEST_F(DispatchTest, RunFanOutOneSourceTwoSinks)
{
    auto graph = std::make_shared<Graph>("test", env);
    std::atomic<int> counter{0};
    std::atomic<int> a{0}, b{0};

    auto src = std::make_shared<CountingSource>(env, counter);
    auto s1  = std::make_shared<Sink>(env, a);
    auto s2  = std::make_shared<Sink>(env, b);

    graph->AddNode(src);
    graph->AddNode(s1);
    graph->AddNode(s2);
    graph->ConnectNodes(src->ID(), "out", s1->ID(), "in");
    graph->ConnectNodes(src->ID(), "out", s2->ID(), "in");

    graph->Run();
    env->Wait();

    EXPECT_EQ(a.load(), 1);
    EXPECT_EQ(b.load(), 1);
}

TEST_F(DispatchTest, VisitTraversesAllNodes)
{
    auto graph = std::make_shared<Graph>("test", env);
    std::atomic<int> counter{0};
    std::atomic<int> last{0};

    auto src  = std::make_shared<CountingSource>(env, counter);
    auto mid  = std::make_shared<Identity>(env);
    auto sink = std::make_shared<Sink>(env, last);

    graph->AddNode(src);
    graph->AddNode(mid);
    graph->AddNode(sink);
    graph->ConnectNodes(src->ID(), "out", mid->ID(), "in");
    graph->ConnectNodes(mid->ID(), "out", sink->ID(), "in");

    std::vector<UUID> visited;
    graph->Visit([&](const SharedNode& n) { visited.push_back(n->ID()); });
    EXPECT_EQ(visited.size(), 3u);
}

TEST_F(DispatchTest, VisitOnEmptyGraphIsNoOp)
{
    auto graph = std::make_shared<Graph>("test", env);
    int calls  = 0;
    graph->Visit([&](const SharedNode&) { ++calls; });
    EXPECT_EQ(calls, 0);
}

TEST_F(DispatchTest, VisitIncludesOrphans)
{
    auto graph = std::make_shared<Graph>("test", env);
    std::atomic<int> counter{0};
    std::atomic<int> last{0};

    auto src    = std::make_shared<CountingSource>(env, counter);
    auto sink   = std::make_shared<Sink>(env, last);
    auto orphan = std::make_shared<Identity>(env);

    graph->AddNode(src);
    graph->AddNode(sink);
    graph->AddNode(orphan);
    graph->ConnectNodes(src->ID(), "out", sink->ID(), "in");

    std::vector<UUID> visited;
    graph->Visit([&](const SharedNode& n) { visited.push_back(n->ID()); });
    EXPECT_EQ(visited.size(), 3u);
}

TEST_F(DispatchTest, OnErrorCalledForStdExceptionInCompute)
{
    auto graph = std::make_shared<Graph>("test", env);
    auto node  = std::make_shared<ThrowingNode>(env, ThrowingNode::Kind::StdEx);
    graph->AddNode(node);

    int err_count = 0;
    node->OnError.Bind("t", [&](const std::exception&) { ++err_count; });

    node->InvokeCompute();
    EXPECT_EQ(err_count, 1);
}

TEST_F(DispatchTest, OnErrorCalledForStringThrow)
{
    auto node = std::make_shared<ThrowingNode>(env, ThrowingNode::Kind::StdString);
    int err_count = 0;
    node->OnError.Bind("t", [&](const std::exception&) { ++err_count; });
    node->InvokeCompute();
    EXPECT_EQ(err_count, 1);
}

TEST_F(DispatchTest, OnErrorCalledForCStringThrow)
{
    auto node = std::make_shared<ThrowingNode>(env, ThrowingNode::Kind::CString);
    int err_count = 0;
    node->OnError.Bind("t", [&](const std::exception&) { ++err_count; });
    node->InvokeCompute();
    EXPECT_EQ(err_count, 1);
}

TEST_F(DispatchTest, OnErrorCalledForIntThrow)
{
    auto node = std::make_shared<ThrowingNode>(env, ThrowingNode::Kind::Int);
    int err_count = 0;
    node->OnError.Bind("t", [&](const std::exception&) { ++err_count; });
    node->InvokeCompute();
    EXPECT_EQ(err_count, 1);
}

TEST_F(DispatchTest, OnErrorCalledForUnknownThrow)
{
    auto node = std::make_shared<ThrowingNode>(env, ThrowingNode::Kind::Other);
    int err_count = 0;
    node->OnError.Bind("t", [&](const std::exception&) { ++err_count; });
    node->InvokeCompute();
    EXPECT_EQ(err_count, 1);
}

TEST_F(DispatchTest, OnComputeEventBroadcasts)
{
    auto node = std::make_shared<Identity>(env);
    int hits  = 0;
    node->OnCompute.Bind("c", [&] { ++hits; });
    node->InvokeCompute();
    EXPECT_EQ(hits, 1);
}

TEST_F(DispatchTest, GraphEmitsNodeAddedRemovedEvents)
{
    auto graph = std::make_shared<Graph>("test", env);
    int added = 0, removed = 0;
    graph->OnNodeAdded.Bind("a", [&](const SharedNode&) { ++added; });
    graph->OnNodeRemoved.Bind("r", [&](const SharedNode&) { ++removed; });

    auto n = std::make_shared<Identity>(env);
    graph->AddNode(n);
    graph->RemoveNode(n);

    EXPECT_EQ(added, 1);
    EXPECT_EQ(removed, 1);
}

TEST_F(DispatchTest, GraphEmitsConnectedDisconnectedEvents)
{
    auto graph = std::make_shared<Graph>("test", env);
    int conn = 0, disc = 0;
    graph->OnNodesConnected.Bind("c", [&](const SharedConnection&) { ++conn; });
    graph->OnNodesDisconnected.Bind("d", [&](const SharedConnection&) { ++disc; });

    auto a = std::make_shared<Identity>(env);
    auto b = std::make_shared<Identity>(env);
    graph->AddNode(a);
    graph->AddNode(b);
    graph->ConnectNodes(a->ID(), "out", b->ID(), "in");
    graph->DisconnectNodes(a->ID(), "out", b->ID(), "in");

    EXPECT_EQ(conn, 1);
    EXPECT_EQ(disc, 1);
}

TEST_F(DispatchTest, AddNodeNullIsNoOp)
{
    auto graph = std::make_shared<Graph>("test", env);
    graph->AddNode(nullptr);
    EXPECT_EQ(graph->Size(), 0u);
}

TEST_F(DispatchTest, RemoveNodeNullIsNoOp)
{
    auto graph = std::make_shared<Graph>("test", env);
    auto n     = std::make_shared<Identity>(env);
    graph->AddNode(n);
    graph->RemoveNode(nullptr);
    EXPECT_EQ(graph->Size(), 1u);
}

TEST_F(DispatchTest, GetNodeMissingReturnsNull)
{
    auto graph = std::make_shared<Graph>("test", env);
    EXPECT_EQ(graph->GetNode(UUID{}), nullptr);
}

TEST_F(DispatchTest, ConnectNodesReturnsNullWhenNodesMissing)
{
    auto graph = std::make_shared<Graph>("test", env);
    EXPECT_EQ(graph->ConnectNodes(UUID{}, "x", UUID{}, "y"), nullptr);
}

TEST_F(DispatchTest, ConnectNodesIdempotentWhenSamePair)
{
    auto graph = std::make_shared<Graph>("test", env);
    auto a     = std::make_shared<Identity>(env);
    auto b     = std::make_shared<Identity>(env);
    graph->AddNode(a);
    graph->AddNode(b);

    auto c1 = graph->ConnectNodes(a->ID(), "out", b->ID(), "in");
    auto c2 = graph->ConnectNodes(a->ID(), "out", b->ID(), "in");
    ASSERT_NE(c1, nullptr);
    EXPECT_EQ(c1, c2);
    EXPECT_EQ(graph->ConnectionCount(), 1u);
}

TEST_F(DispatchTest, DisconnectMissingIsNoOp)
{
    auto graph = std::make_shared<Graph>("test", env);
    auto a     = std::make_shared<Identity>(env);
    auto b     = std::make_shared<Identity>(env);
    graph->AddNode(a);
    graph->AddNode(b);

    graph->DisconnectNodes(a->ID(), "out", b->ID(), "in");
    EXPECT_EQ(graph->ConnectionCount(), 0u);
}

TEST_F(DispatchTest, ClearRemovesAll)
{
    auto graph = std::make_shared<Graph>("test", env);
    auto a     = std::make_shared<Identity>(env);
    auto b     = std::make_shared<Identity>(env);
    graph->AddNode(a);
    graph->AddNode(b);
    graph->ConnectNodes(a->ID(), "out", b->ID(), "in");

    graph->Clear();
    EXPECT_EQ(graph->Size(), 0u);
    EXPECT_EQ(graph->ConnectionCount(), 0u);
}

TEST_F(DispatchTest, ConnectAfterDataPresentPropagatesImmediately)
{
    auto graph = std::make_shared<Graph>("test", env);
    auto a     = std::make_shared<Identity>(env);
    auto b     = std::make_shared<Identity>(env);
    graph->AddNode(a);
    graph->AddNode(b);

    a->SetOutputData("out", MakeNodeData<int>(99));
    graph->ConnectNodes(a->ID(), "out", b->ID(), "in");
    env->Wait();
    auto d = b->GetInputData<int>("in");
    ASSERT_NE(d, nullptr);
    EXPECT_EQ(d->Get(), 99);
}

TEST_F(DispatchTest, SetNameAndGetNameAndID)
{
    auto graph = std::make_shared<Graph>("orig", env);
    EXPECT_EQ(graph->GetName(), "orig");
    graph->SetName("renamed");
    EXPECT_EQ(graph->GetName(), "renamed");
    EXPECT_EQ(graph->GetEnv(), env);
    EXPECT_FALSE(std::string(graph->ID()).empty());
}

TEST_F(DispatchTest, ValidateNodeMatchesMembership)
{
    auto graph = std::make_shared<Graph>("g", env);
    auto a     = std::make_shared<Identity>(env);
    auto b     = std::make_shared<Identity>(env);
    graph->AddNode(a);
    EXPECT_TRUE(graph->ValidateNode(a));
    EXPECT_FALSE(graph->ValidateNode(b));
    EXPECT_FALSE(graph->ValidateNode(nullptr));
}

TEST_F(DispatchTest, JsonRoundTripPreservesStructure)
{
    factory->RegisterNodeClass<Identity>("Test", "Identity");

    auto graph = std::make_shared<Graph>("g", env);
    auto a     = std::make_shared<Identity>(env);
    auto b     = std::make_shared<Identity>(env);
    graph->AddNode(a);
    graph->AddNode(b);
    graph->ConnectNodes(a->ID(), "out", b->ID(), "in");

    json j;
    to_json(j, *graph);
    ASSERT_TRUE(j.contains("nodes"));
    ASSERT_TRUE(j.contains("connections"));
    EXPECT_EQ(j["nodes"].size(), 2u);
    EXPECT_EQ(j["connections"].size(), 1u);

    auto graph2 = std::make_shared<Graph>("g2", env);
    ASSERT_NO_THROW(from_json(j, *graph2));
    EXPECT_EQ(graph2->Size(), 2u);
    EXPECT_EQ(graph2->ConnectionCount(), 1u);

    factory->UnregisterNodeClass<Identity>("Test");
}
