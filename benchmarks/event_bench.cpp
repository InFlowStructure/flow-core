// Copyright (c) 2025, Cisco Systems, Inc.
// All rights reserved.

#include <benchmark/benchmark.h>

#include <flow/core/Env.hpp>
#include <flow/core/Event.hpp>
#include <flow/core/IndexableName.hpp>
#include <flow/core/NodeFactory.hpp>

#include <array>
#include <string>

static void EventDispatcher_Broadcast(benchmark::State& state)
{
    flow::EventDispatcher<> dispatcher;
    std::array<std::string, 1000> names;

    for (int i = 0; i < names.size(); ++i)
    {
        names[i] = "Event_" + std::to_string(i);
        dispatcher.Bind(flow::IndexableName{names[i]}, [&] { ++i; });
    }

    for ([[maybe_unused]] const auto& _ : state)
    {
        dispatcher.Broadcast();
    }
}

BENCHMARK(EventDispatcher_Broadcast);
