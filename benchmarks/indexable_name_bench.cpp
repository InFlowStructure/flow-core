// Copyright (c) 2025, Cisco Systems, Inc.
// All rights reserved.

#include <benchmark/benchmark.h>

#include <flow/core/IndexableName.hpp>

static void IndexableName_Construct(benchmark::State& state)
{
    for ([[maybe_unused]] const auto& _ : state)
    {
        flow::IndexableName{"benchmark"};
    }
}

static void IndexableName_Hash(benchmark::State& state)
{
    constexpr flow::IndexableName name{"benchmark"};
    for ([[maybe_unused]] const auto& _ : state)
    {
        auto h = std::hash<flow::IndexableName>{}(name);
        benchmark::DoNotOptimize(h);
        benchmark::ClobberMemory();
    }
}

BENCHMARK(IndexableName_Construct);
BENCHMARK(IndexableName_Hash);
