// Copyright (c) 2024, Cisco Systems, Inc.
// All rights reserved.

#include "flow/core/Env.hpp"
#include "flow/core/Graph.hpp"
#include "flow/core/IndexableName.hpp"
#include "flow/core/Node.hpp"
#include "flow/core/NodeData.hpp"
#include "flow/core/NodeFactory.hpp"
#include "flow/core/TypeConversion.hpp"

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <any>
#include <atomic>
#include <stdexcept>

using namespace flow;

namespace de
{
struct Identity : public Node
{
    Identity(const UUID& id, std::string_view name, std::shared_ptr<Env> env)
        : Node(id, TypeName_v<Identity>, name, std::move(env))
    {
        AddInput<int>("in", "");
        AddOutput<int>("out", "");
    }
    Identity(std::shared_ptr<Env> env) : Identity(UUID{}, "id", std::move(env)) {}
    void Compute() override
    {
        if (auto d = GetInputData<int>("in"))
        {
            SetOutputData("out", MakeNodeData<int>(d->Get()));
        }
    }
};
} // namespace de

class DispatchEdgeTest : public ::testing::Test
{
  protected:
    std::shared_ptr<NodeFactory> factory = std::make_shared<NodeFactory>();
    std::shared_ptr<Env> env             = Env::Create(factory);
};

TEST_F(DispatchEdgeTest, PropagateOnFailingConversionFiresOnError)
{
    // When the registered conversion is set to a function that throws, the propagation
    // task should swallow the exception and emit OnError on the Graph.
    TypeRegistry tr;
    // Register a conversion int -> double that throws.
    factory->RegisterUnidirectionalConversion<int, std::string>(
        [](const SharedNodeData&) -> SharedNodeData { throw std::runtime_error("conv-boom"); });

    auto graph = std::make_shared<Graph>("g", env);
    auto a     = std::make_shared<de::Identity>(env);

    // Build a sink that takes a string input so propagation triggers conversion.
    struct StringSink : public Node
    {
        StringSink(std::shared_ptr<Env> env) : Node(UUID{}, TypeName_v<StringSink>, "ss", std::move(env))
        {
            AddInput<std::string>("in", "");
        }
        void Compute() override {}
    };
    auto sink = std::make_shared<StringSink>(env);

    graph->AddNode(a);
    graph->AddNode(sink);
    graph->ConnectNodes(a->ID(), "out", sink->ID(), "in");

    std::atomic<int> err_count{0};
    graph->OnError.Bind("e", [&](const std::exception&) { err_count.fetch_add(1); });

    a->SetOutputData("out", MakeNodeData<int>(1));
    env->Wait();

    EXPECT_GE(err_count.load(), 1);
}

TEST_F(DispatchEdgeTest, PropagateAfterEndNodeRemovedIsSafe)
{
    auto graph = std::make_shared<Graph>("g", env);
    auto a     = std::make_shared<de::Identity>(env);
    auto b     = std::make_shared<de::Identity>(env);
    graph->AddNode(a);
    graph->AddNode(b);
    graph->ConnectNodes(a->ID(), "out", b->ID(), "in");

    // Remove the end node from the graph, but keep the SharedConnection alive
    // by virtue of `_connections` ownership being detached on RemoveNodeByID.
    graph->RemoveNode(b);

    // Propagation should not crash even if the connection's end node is gone.
    ASSERT_NO_THROW(a->SetOutputData("out", MakeNodeData<int>(7)));
    env->Wait();
    SUCCEED();
}

TEST_F(DispatchEdgeTest, FromJsonLegacyFormatHonored)
{
    factory->RegisterNodeClass<de::Identity>("Test", "Identity");

    json legacy;
    legacy["nodes"] = json::array();
    legacy["nodes"].push_back(json{
        {"id", "11111111-2222-3333-4444-555555555555"},
        {"model", {{"class", std::string{TypeName_v<de::Identity>}}, {"name", "legacy-name"}}},
        {"position", {{"x", 0}, {"y", 0}}},
    });
    legacy["connections"] = json::array();

    auto g = std::make_shared<Graph>("g", env);
    ASSERT_NO_THROW(from_json(legacy, *g));
    EXPECT_EQ(g->Size(), 1u);

    factory->UnregisterCategory(Category{"Test"});
}

TEST_F(DispatchEdgeTest, FromJsonSkipsUnknownClass)
{
    json j;
    j["nodes"] = json::array();
    j["nodes"].push_back(json{
        {"id", "11111111-2222-3333-4444-555555555555"},
        {"class", "NotRegistered"},
        {"name", "ghost"},
    });
    j["connections"] = json::array();

    auto g = std::make_shared<Graph>("g", env);
    ASSERT_NO_THROW(from_json(j, *g));
    EXPECT_EQ(g->Size(), 0u);
}

TEST_F(DispatchEdgeTest, FromJsonConnectionLegacyKeyNames)
{
    factory->RegisterNodeClass<de::Identity>("Test", "Identity");

    json j;
    UUID a_id, b_id;
    j["nodes"] = json::array();
    j["nodes"].push_back(
        json{{"id", std::string(a_id)}, {"class", std::string{TypeName_v<de::Identity>}}, {"name", "a"}});
    j["nodes"].push_back(
        json{{"id", std::string(b_id)}, {"class", std::string{TypeName_v<de::Identity>}}, {"name", "b"}});

    j["connections"] = json::array();
    j["connections"].push_back(json{
        {"in_id", std::string(a_id)},
        {"in_var_name", "out"},
        {"out_id", std::string(b_id)},
        {"out_var_name", "in"},
    });

    auto g = std::make_shared<Graph>("g", env);
    ASSERT_NO_THROW(from_json(j, *g));
    EXPECT_EQ(g->Size(), 2u);
    EXPECT_EQ(g->ConnectionCount(), 1u);

    factory->UnregisterCategory(Category{"Test"});
}

TEST_F(DispatchEdgeTest, TypeRegistryConvertReturnsOriginalWhenTargetUnknown)
{
    TypeRegistry r;
    r.RegisterUnidirectionalConversion<int, double>();
    auto d = MakeNodeData<int>(7);
    // No conversion to std::string exists for int, even though int is in the map.
    auto out = r.Convert(d, TypeName_v<std::string>);
    EXPECT_EQ(out, d);
}

TEST_F(DispatchEdgeTest, TypeRegistryThrowsWhenConversionFunctionNull)
{
    TypeRegistry r;
    // Register a null conversion function explicitly.
    r.RegisterUnidirectionalConversion<int, double>(TypeRegistry::ConversionFunc{});
    auto d = MakeNodeData<int>(7);
    EXPECT_THROW(r.Convert(d, TypeName_v<double>), std::runtime_error);
}
