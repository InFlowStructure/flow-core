// Copyright (c) 2025, Cisco Systems, Inc.
// All rights reserved.

#include <benchmark/benchmark.h>

#include <flow/core/Env.hpp>
#include <flow/core/Graph.hpp>
#include <flow/core/NodeFactory.hpp>

static void Graph_Construct(benchmark::State& state)
{
    auto factory = flow::NodeFactory::Create();
    auto env     = flow::Env::Create(factory);
    for ([[maybe_unused]] const auto& _ : state)
    {
        flow::Graph("benchmark", env);
    }
}

BENCHMARK(Graph_Construct);
