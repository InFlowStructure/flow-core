// Copyright (c) 2025, Cisco Systems, Inc.
// All rights reserved.

#include <benchmark/benchmark.h>

#include <flow/core/TypeName.hpp>

static void TypeName_Construct(benchmark::State& state)
{
    for ([[maybe_unused]] const auto& _ : state)
    {
        flow::TypeName<int> name;
        benchmark::DoNotOptimize(name);
        benchmark::ClobberMemory();
    }
}

BENCHMARK(TypeName_Construct);
