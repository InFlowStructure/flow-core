// Copyright (c) 2025, Cisco Systems, Inc.
// All rights reserved.

#include <benchmark/benchmark.h>

#include <flow/core/UUID.hpp>

static void UUID_ConstructGenerate(benchmark::State& state)
{
    for ([[maybe_unused]] const auto& _ : state)
    {
        flow::UUID id{};
        benchmark::DoNotOptimize(id);
        benchmark::ClobberMemory();
    }
}

static void UUID_ConstructFromString(benchmark::State& state)
{
    for ([[maybe_unused]] const auto& _ : state)
    {
        flow::UUID id{"b24f917e-3626-4246-bf13-c2543145abfd"};
        benchmark::DoNotOptimize(id);
        benchmark::ClobberMemory();
    }
}

static void UUID_ConstructToString(benchmark::State& state)
{
    flow::UUID id{"b24f917e-3626-4246-bf13-c2543145abfd"};
    for ([[maybe_unused]] const auto& _ : state)
    {
        std::string uuid_str = std::string(id);
        benchmark::DoNotOptimize(id);
        benchmark::ClobberMemory();
    }
}

static void UUID_Hash(benchmark::State& state)
{
    flow::UUID id{"b24f917e-3626-4246-bf13-c2543145abfd"};
    for ([[maybe_unused]] const auto& _ : state)
    {
        auto h = std::hash<flow::UUID>{}(id);
        benchmark::DoNotOptimize(h);
        benchmark::ClobberMemory();
    }
}

BENCHMARK(UUID_ConstructGenerate);
BENCHMARK(UUID_ConstructFromString);
BENCHMARK(UUID_ConstructToString);
BENCHMARK(UUID_Hash);
