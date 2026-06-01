// Copyright (c) 2025, Cisco Systems, Inc.
// All rights reserved.

#include <benchmark/benchmark.h>

#include <flow/core/Env.hpp>
#include <flow/core/NodeFactory.hpp>
#include <flow/core/Port.hpp>

#include <memory>

namespace
{
auto test_factory  = flow::NodeFactory::Create();
auto test_env      = flow::Env::Create(test_factory);
auto existing_data = flow::MakeNodeData<int>(101);
auto new_data      = flow::MakeNodeData<int>(202);
} // namespace

static void Port_ConstructShared(benchmark::State& state)
{
    for ([[maybe_unused]] const auto& _ : state)
    {
        auto port = std::make_shared<flow::Port>("port_x", "test port", "int", existing_data, false, 0);
        benchmark::DoNotOptimize(port);
        benchmark::ClobberMemory();
    }
}

static void Port_SetData_NoOutput(benchmark::State& state)
{
    auto port = std::make_shared<flow::Port>("port_x", "test port", "int", existing_data, false, 0);
    for ([[maybe_unused]] const auto& _ : state)
    {
        port->SetData(new_data, false);
    }
}

static void Port_SetData_Output(benchmark::State& state)
{
    auto port = std::make_shared<flow::Port>("port_x", "test port", "int", existing_data, false, 0);
    for ([[maybe_unused]] const auto& _ : state)
    {
        port->SetData(new_data, true);
    }
}

BENCHMARK(Port_ConstructShared);
BENCHMARK(Port_SetData_NoOutput);
BENCHMARK(Port_SetData_Output);
