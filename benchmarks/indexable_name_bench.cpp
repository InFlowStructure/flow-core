// Copyright (c) 2025, Cisco Systems, Inc.
// All rights reserved.

#include <benchmark/benchmark.h>

#include <flow/core/IndexableName.hpp>

static void IndexableName_Construct(benchmark::State& state)
{
    for ([[maybe_unused]] const auto& _ : state)
    {
        benchmark::DoNotOptimize(flow::IndexableName{"benchmark"});
    }
}

static void IndexableName_Hash(benchmark::State& state)
{
    constexpr flow::IndexableName name{"benchmark"};
    for ([[maybe_unused]] const auto& _ : state)
    {
        benchmark::DoNotOptimize(std::hash<flow::IndexableName>{}(name));
    }
}

BENCHMARK(IndexableName_Construct);
BENCHMARK(IndexableName_Hash);
