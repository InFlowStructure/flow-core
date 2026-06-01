// Copyright (c) 2024, Cisco Systems, Inc.
// All rights reserved.
//
// Real-measurement microbenchmark for the flow-core graph dispatch model.
//
// What we measure:
//   1. Baseline: invoking Compute() directly (no scheduling, no locks).
//   2. InvokeCompute() overhead vs raw Compute (the noexcept-try wrapper +
//      OnCompute broadcast).
//   3. Thread-pool detach_task overhead (raw env->AddTask round-trip).
//   4. Per-hop propagation cost: one-stage graph where data flows source -> sink
//      through one connection. Wall-clock per emitted value, end-to-end.
//   5. Multi-hop end-to-end pipeline latency for chains of length 1, 4, 16.
//   6. Fan-out: one source emitting to N sinks (8, 64, 256).
//   7. Connection-lookup cost (`Connections::FindConnections`) under varying
//      table sizes.
//   8. IndexableName CRC-64 hash cost per construction.
//
// Each measurement runs a warmup phase, then a timed phase. Reports min /
// mean / p99 (or median for slow ones) and ops/sec. No external benchmark
// framework — std::chrono::steady_clock only.

#include "flow/core/Connections.hpp"
#include "flow/core/Env.hpp"
#include "flow/core/Graph.hpp"
#include "flow/core/IndexableName.hpp"
#include "flow/core/Node.hpp"
#include "flow/core/NodeData.hpp"
#include "flow/core/NodeFactory.hpp"
#include "flow/core/UUID.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

using namespace flow;
using clk = std::chrono::steady_clock;

namespace
{

struct CountingSource : public Node
{
    CountingSource(std::shared_ptr<Env> env) : Node(UUID{}, TypeName_v<CountingSource>, "src", std::move(env))
    {
        AddOutput<int>("out", "");
    }
    int counter = 0;
    void Compute() override
    {
        ++counter;
        SetOutputData("out", MakeNodeData<int>(counter));
    }
    // Bypass for raw-compute path comparisons; doesn't broadcast.
    void RawCompute() { ++counter; }
    // Helper so external benchmark code can stub the propagation callback
    // without a real Graph.
    void StubPropagate()
    {
        _propagate_output_update = [](const UUID&, const IndexableName&, SharedNodeData) {};
    }
};

struct Identity : public Node
{
    Identity(std::shared_ptr<Env> env) : Node(UUID{}, TypeName_v<Identity>, "id", std::move(env))
    {
        AddInput<int>("in", "");
        AddOutput<int>("out", "");
    }
    void Compute() override
    {
        if (auto d = GetInputData<int>("in"))
        {
            SetOutputData("out", MakeNodeData<int>(d->Get()));
        }
    }
};

// Sink that signals completion via a condition variable for end-to-end timing.
struct SignalSink : public Node
{
    SignalSink(std::shared_ptr<Env> env, std::mutex& m, std::condition_variable& cv, std::atomic<int>& seen)
        : Node(UUID{}, TypeName_v<SignalSink>, "sink", std::move(env)), _m{m}, _cv{cv}, _seen{seen}
    {
        AddInput<int>("in", "");
    }
    void Compute() override
    {
        if (auto d = GetInputData<int>("in"))
        {
            {
                std::lock_guard<std::mutex> _(_m);
                _seen.store(d->Get());
            }
            _cv.notify_one();
        }
    }
    std::mutex& _m;
    std::condition_variable& _cv;
    std::atomic<int>& _seen;
};

struct Stats
{
    double mean_ns;
    double min_ns;
    double median_ns;
    double p99_ns;
};

Stats Summarize(std::vector<double>& samples_ns)
{
    std::sort(samples_ns.begin(), samples_ns.end());
    Stats s{};
    s.min_ns   = samples_ns.empty() ? 0.0 : samples_ns.front();
    double sum = 0.0;
    for (double v : samples_ns)
        sum += v;
    s.mean_ns   = samples_ns.empty() ? 0.0 : sum / samples_ns.size();
    s.median_ns = samples_ns.empty() ? 0.0 : samples_ns[samples_ns.size() / 2];
    s.p99_ns =
        samples_ns.empty()
            ? 0.0
            : samples_ns[std::min(samples_ns.size() - 1, static_cast<std::size_t>(samples_ns.size() * 99 / 100))];
    return s;
}

void PrintRow(const char* label, const Stats& s, std::size_t n)
{
    double ops = s.mean_ns > 0 ? 1e9 / s.mean_ns : 0.0;
    std::printf("  %-44s  n=%-8zu  min=%10.1f ns  mean=%10.1f ns  p99=%10.1f ns  (%8.0f ops/s)\n", label, n, s.min_ns,
                s.mean_ns, s.p99_ns, ops);
}

// ----- 1. raw Compute() -----
void BenchRawCompute(std::shared_ptr<Env> env)
{
    auto src                     = std::make_shared<CountingSource>(env);
    constexpr std::size_t warmup = 10'000, iters = 200'000;
    for (std::size_t i = 0; i < warmup; ++i)
        src->RawCompute();

    std::vector<double> samples;
    samples.reserve(iters);
    for (std::size_t i = 0; i < iters; ++i)
    {
        auto t0 = clk::now();
        src->RawCompute();
        auto t1 = clk::now();
        samples.push_back(std::chrono::duration<double, std::nano>(t1 - t0).count());
    }
    PrintRow("RawCompute() (++counter only)", Summarize(samples), iters);
}

// ----- 2. InvokeCompute() including SetOutputData/EmitUpdate path -----
void BenchInvokeComputeNoGraph(std::shared_ptr<Env> env)
{
    auto src = std::make_shared<CountingSource>(env);
    src->StubPropagate();
    constexpr std::size_t warmup = 10'000, iters = 100'000;
    for (std::size_t i = 0; i < warmup; ++i)
        src->InvokeCompute();

    std::vector<double> samples;
    samples.reserve(iters);
    for (std::size_t i = 0; i < iters; ++i)
    {
        auto t0 = clk::now();
        src->InvokeCompute();
        auto t1 = clk::now();
        samples.push_back(std::chrono::duration<double, std::nano>(t1 - t0).count());
    }
    PrintRow("InvokeCompute() + SetOutputData (no graph)", Summarize(samples), iters);
}

// ----- 3. Thread-pool detach_task overhead -----
void BenchPoolDispatch(std::shared_ptr<Env> env)
{
    constexpr std::size_t iters = 50'000;

    // Warmup
    for (int i = 0; i < 1000; ++i)
        env->AddTask([] {});
    env->Wait();

    // Per-task wall-clock when amortized over the whole batch.
    std::atomic<int> sink{0};
    auto t0 = clk::now();
    for (std::size_t i = 0; i < iters; ++i)
        env->AddTask([&] { sink.fetch_add(1); });
    env->Wait();
    auto t1         = clk::now();
    double total_ns = std::chrono::duration<double, std::nano>(t1 - t0).count();
    double per_task = total_ns / iters;

    Stats s{};
    s.mean_ns   = per_task;
    s.min_ns    = per_task;
    s.median_ns = per_task;
    s.p99_ns    = per_task;
    PrintRow("Env::AddTask round-trip (amortized)", s, iters);
}

// ----- 4. End-to-end one-hop propagation latency -----
void BenchOneHop(std::shared_ptr<Env> env)
{
    auto graph = std::make_shared<Graph>("g", env);
    auto src   = std::make_shared<CountingSource>(env);

    std::mutex m;
    std::condition_variable cv;
    std::atomic<int> seen{0};
    auto sink = std::make_shared<SignalSink>(env, m, cv, seen);

    graph->AddNode(src);
    graph->AddNode(sink);
    graph->ConnectNodes(src->ID(), "out", sink->ID(), "in");

    constexpr std::size_t warmup = 200, iters = 5'000;

    for (std::size_t i = 0; i < warmup; ++i)
    {
        seen.store(0);
        src->InvokeCompute();
        std::unique_lock<std::mutex> lk(m);
        cv.wait(lk, [&] { return seen.load() != 0; });
    }

    std::vector<double> samples;
    samples.reserve(iters);
    for (std::size_t i = 0; i < iters; ++i)
    {
        seen.store(0);
        auto t0 = clk::now();
        src->InvokeCompute();
        std::unique_lock<std::mutex> lk(m);
        cv.wait(lk, [&] { return seen.load() != 0; });
        auto t1 = clk::now();
        samples.push_back(std::chrono::duration<double, std::nano>(t1 - t0).count());
    }
    PrintRow("End-to-end 1-hop (src -> sink)", Summarize(samples), iters);
}

// ----- 5. Multi-hop pipeline latency -----
void BenchPipeline(std::shared_ptr<Env> env, std::size_t hops)
{
    auto graph = std::make_shared<Graph>("g", env);
    auto src   = std::make_shared<CountingSource>(env);
    graph->AddNode(src);

    std::vector<std::shared_ptr<Identity>> mids;
    for (std::size_t i = 0; i < hops - 1; ++i)
    {
        auto n = std::make_shared<Identity>(env);
        graph->AddNode(n);
        mids.push_back(n);
    }

    std::mutex m;
    std::condition_variable cv;
    std::atomic<int> seen{0};
    auto sink = std::make_shared<SignalSink>(env, m, cv, seen);
    graph->AddNode(sink);

    // Wire: src -> mids[0] -> mids[1] -> ... -> sink
    SharedNode prev = src;
    for (auto& mid : mids)
    {
        graph->ConnectNodes(prev->ID(), "out", mid->ID(), "in");
        prev = mid;
    }
    graph->ConnectNodes(prev->ID(), "out", sink->ID(), "in");

    constexpr std::size_t warmup = 50, iters = 1'000;
    for (std::size_t i = 0; i < warmup; ++i)
    {
        seen.store(0);
        src->InvokeCompute();
        std::unique_lock<std::mutex> lk(m);
        cv.wait(lk, [&] { return seen.load() != 0; });
    }

    std::vector<double> samples;
    samples.reserve(iters);
    for (std::size_t i = 0; i < iters; ++i)
    {
        seen.store(0);
        auto t0 = clk::now();
        src->InvokeCompute();
        std::unique_lock<std::mutex> lk(m);
        cv.wait(lk, [&] { return seen.load() != 0; });
        auto t1 = clk::now();
        samples.push_back(std::chrono::duration<double, std::nano>(t1 - t0).count());
    }

    char label[80];
    std::snprintf(label, sizeof(label), "End-to-end %zu-hop pipeline", hops);
    PrintRow(label, Summarize(samples), iters);
}

// ----- 6. Fan-out latency (one src, N sinks) -----
void BenchFanout(std::shared_ptr<Env> env, std::size_t fanout)
{
    auto graph = std::make_shared<Graph>("g", env);
    auto src   = std::make_shared<CountingSource>(env);
    graph->AddNode(src);

    std::mutex m;
    std::condition_variable cv;
    std::atomic<int> remaining{0};

    struct CountSink : public Node
    {
        CountSink(std::shared_ptr<Env> env, std::mutex& m, std::condition_variable& cv, std::atomic<int>& r)
            : Node(UUID{}, TypeName_v<CountSink>, "cs", std::move(env)), _m{m}, _cv{cv}, _r{r}
        {
            AddInput<int>("in", "");
        }
        void Compute() override
        {
            if (auto d = GetInputData<int>("in"))
            {
                int prev = _r.fetch_sub(1);
                if (prev == 1)
                {
                    std::lock_guard<std::mutex> _(_m);
                    _cv.notify_one();
                }
            }
        }
        std::mutex& _m;
        std::condition_variable& _cv;
        std::atomic<int>& _r;
    };

    std::vector<std::shared_ptr<CountSink>> sinks;
    for (std::size_t i = 0; i < fanout; ++i)
    {
        auto s = std::make_shared<CountSink>(env, m, cv, remaining);
        graph->AddNode(s);
        graph->ConnectNodes(src->ID(), "out", s->ID(), "in");
        sinks.push_back(s);
    }

    constexpr std::size_t warmup = 20, iters = 200;
    for (std::size_t i = 0; i < warmup; ++i)
    {
        remaining.store(static_cast<int>(fanout));
        src->InvokeCompute();
        std::unique_lock<std::mutex> lk(m);
        cv.wait(lk, [&] { return remaining.load() == 0; });
    }

    std::vector<double> samples;
    samples.reserve(iters);
    for (std::size_t i = 0; i < iters; ++i)
    {
        remaining.store(static_cast<int>(fanout));
        auto t0 = clk::now();
        src->InvokeCompute();
        std::unique_lock<std::mutex> lk(m);
        cv.wait(lk, [&] { return remaining.load() == 0; });
        auto t1 = clk::now();
        samples.push_back(std::chrono::duration<double, std::nano>(t1 - t0).count());
    }

    char label[96];
    std::snprintf(label, sizeof(label), "End-to-end fan-out 1 -> %zu", fanout);
    PrintRow(label, Summarize(samples), iters);
}

// ----- 7. Connection lookup cost -----
void BenchFindConnections(std::size_t table_size)
{
    Connections c;
    UUID target_id;
    for (std::size_t i = 0; i < table_size; ++i)
    {
        UUID id;
        c.Add(id, "out", target_id, "in");
    }
    // Add a handful targeted at a specific id we'll query.
    UUID hot_id;
    for (int i = 0; i < 4; ++i)
        c.Add(hot_id, "out", target_id, "in");

    constexpr std::size_t iters = 100'000;
    for (std::size_t i = 0; i < 1000; ++i)
        (void)c.FindConnections(hot_id);

    std::vector<double> samples;
    samples.reserve(iters);
    volatile std::size_t sink = 0;
    for (std::size_t i = 0; i < iters; ++i)
    {
        auto t0 = clk::now();
        auto v  = c.FindConnections(hot_id);
        auto t1 = clk::now();
        sink += v.size();
        samples.push_back(std::chrono::duration<double, std::nano>(t1 - t0).count());
    }
    (void)sink;

    char label[96];
    std::snprintf(label, sizeof(label), "FindConnections (table=%zu, 4 hot)", table_size);
    PrintRow(label, Summarize(samples), iters);
}

// ----- 8. IndexableName hash cost -----
void BenchIndexableNameHash()
{
    const char* strings[]       = {"port_a", "input_value", "out", "very_long_port_name_for_test"};
    constexpr std::size_t iters = 1'000'000;

    volatile std::size_t sink = 0;
    auto t0                   = clk::now();
    for (std::size_t i = 0; i < iters; ++i)
    {
        IndexableName n(strings[i & 3]);
        sink += static_cast<std::size_t>(n);
    }
    auto t1    = clk::now();
    double per = std::chrono::duration<double, std::nano>(t1 - t0).count() / iters;
    Stats s{};
    s.mean_ns   = per;
    s.min_ns    = per;
    s.median_ns = per;
    s.p99_ns    = per;
    PrintRow("IndexableName(str) construct (avg)", s, iters);
    (void)sink;
}

} // namespace

int main()
{
    auto factory = std::make_shared<NodeFactory>();
    auto env     = Env::Create(factory);

    std::printf("flow-core dispatch benchmark\n");
    std::printf("===========================================================================\n");

    std::printf("\n[ Compute / dispatch overhead ]\n");
    BenchRawCompute(env);
    BenchInvokeComputeNoGraph(env);
    BenchPoolDispatch(env);

    std::printf("\n[ End-to-end propagation through graph ]\n");
    BenchOneHop(env);
    BenchPipeline(env, 4);
    BenchPipeline(env, 16);

    std::printf("\n[ Fan-out latency (1 source -> N sinks) ]\n");
    BenchFanout(env, 8);
    BenchFanout(env, 64);
    BenchFanout(env, 256);

    std::printf("\n[ Connection lookup ]\n");
    BenchFindConnections(8);
    BenchFindConnections(128);
    BenchFindConnections(2048);

    std::printf("\n[ IndexableName hash ]\n");
    BenchIndexableNameHash();

    std::printf("\n");
    return 0;
}
