// Copyright (c) 2025, Cisco Systems, Inc.
// All rights reserved.

#include <benchmark/benchmark.h>

#include <flow/core/FunctionNode.hpp>
#include <flow/core/NodeFactory.hpp>

#include <cmath>

static void FunctionNode_RawFunctionCompute(benchmark::State& state)
{
    for ([[maybe_unused]] const auto& _ : state)
    {
        double value = std::sin(0);
        benchmark::DoNotOptimize(value);
        benchmark::ClobberMemory();
    }
}

static void FunctionNode_Compute(benchmark::State& state)
{
    auto factory = flow::NodeFactory::Create();
    auto env     = flow::Env::Create(factory);

    auto sin_node = flow::FunctionNode<static_cast<double (*)(double)>(&std::sin)>(flow::UUID{}, "std::sin", env);
    sin_node.SetInputData("a", flow::MakeNodeData<double>(0));

    for ([[maybe_unused]] const auto& _ : state)
    {
        sin_node.InvokeCompute();
    }
}

BENCHMARK(FunctionNode_RawFunctionCompute);
BENCHMARK(FunctionNode_Compute);
