// Copyright (c) 2025, Cisco Systems, Inc.
// All rights reserved.

#include <benchmark/benchmark.h>

#include <flow/core/IndexableName.hpp>

static void IndexableName_Construct(benchmark::State& state)
{
    for ([[maybe_unused]] const auto& _ : state)
    {
        flow::IndexableName name{"benchmark"};
        benchmark::DoNotOptimize(name);
        benchmark::ClobberMemory();
    }
}

static void IndexableName_ConstexprConstruct(benchmark::State& state)
{
    for ([[maybe_unused]] const auto& _ : state)
    {
        constexpr flow::IndexableName name{"benchmark"};
        benchmark::DoNotOptimize(name);
        benchmark::ClobberMemory();
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
BENCHMARK(IndexableName_ConstexprConstruct);
BENCHMARK(IndexableName_Hash);
