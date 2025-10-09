// Copyright (c) 2025, Cisco Systems, Inc.
// All rights reserved.

#include <benchmark/benchmark.h>

#include <flow/core/NodeData.hpp>

static void NodeData_Construct(benchmark::State& state)
{
    for ([[maybe_unused]] const auto& _ : state)
    {
        flow::NodeData<int> data;
        benchmark::DoNotOptimize(data);
        benchmark::ClobberMemory();
    }
}

static void NodeData_ConstructShared(benchmark::State& state)
{
    for ([[maybe_unused]] const auto& _ : state)
    {
        auto data = flow::MakeNodeData<int>(0);
        benchmark::DoNotOptimize(data);
        benchmark::ClobberMemory();
    }
}

BENCHMARK(NodeData_Construct);
BENCHMARK(NodeData_ConstructShared);
