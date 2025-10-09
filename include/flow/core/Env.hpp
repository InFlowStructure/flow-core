// Copyright (c) 2024, Cisco Systems, Inc.
// All rights reserved.

#pragma once

#include "Core.hpp"

#include <BS_thread_pool.hpp>

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

FLOW_NAMESPACE_BEGIN

using thread_pool = BS::thread_pool<>;

class Node;
class NodeFactory;

/**
 * @brief Settings for the flow environment.
 *
 * @details Holds configuration options for the Env, such as the maximum number of threads in the thread pool.
 */
struct Settings
{
    /// The maximum number of threads in the thread pool.
    std::size_t MaxThreads = 10;
};

/**
 * @brief Flow environment
 *
 * @details Env represents the environment in which all flows run. It holds the thread pool, provides easy access to
 *          system env vars, and more.
 */
class Env
{
    /**
     * @brief Constructs an Env with the given node factory and settings.
     *
     * @param factory The node factory to use for constructing available nodes.
     * @param settings The settings for the environment.
     */
    explicit Env(std::shared_ptr<NodeFactory> factory, const Settings& settings);

  public:
    /// Type alias for a function that visits a shared node.
    using VisitorFunction = std::function<void(const std::shared_ptr<Node>&)>;

    Env(const Env&) = delete;

    /**
     * @brief Creator method which constructs only shared pointers.
     *
     * @param factory The node factory to use for constructing available nodes.
     * @param settings The settings for the environment. Defaults to default-constructed Settings.
     * @returns A shared pointer to the created Env.
     */
    static std::shared_ptr<Env> Create(std::shared_ptr<NodeFactory> factory, const Settings& settings = {});

    /**
     * @brief Gets the current factory for building nodes.
     * @returns The shared pointer to the NodeFactory.
     */
    [[nodiscard]] std::shared_ptr<NodeFactory> GetFactory() const { return _factory; }

    /**
     * @brief Waits for all threads in the pool to dequeue and execute.
     */
    void Wait();

    /**
     * @brief Add a task to the thread pool queue.
     *
     * @tparam F The task type
     * @tparam Args Variadic list of argument types for the task.
     * @param task A function to be executed on a thread from the pool.
     * @param args Variadic list of arguments for the task.
     */
    template<typename F, typename... Args>
    void AddTask(F&& task, Args&&... args)
    {
        _pool->detach_task([=] { task(std::forward<Args>(args)...); });
    }

    /**
     * @brief Add a sequence of tasks to the thread pool queue.
     *
     * @tparam I The index type.
     * @tparam F The task type.
     * @tparam Args Variadic list of argument types for the task.
     * @param first_index The first index in the sequence.
     * @param last_index The last index in the sequence.
     * @param task A function to be executed on a thread from the pool. MUST have a first argument of the index.
     * @param args Variadic list of arguments for the task.
     */
    template<typename I, typename F, typename... Args>
    void AddSequenceTask(I first_index, I last_index, F&& task, Args&&... args)
    {
        _pool->detach_sequence(first_index, last_index, [=](auto&& idx) { task(idx, std::forward<Args>(args)...); });
    }

    /**
     * @brief Parallelise a loop task into blocks, one index at a time.
     *
     * @tparam I The index type.
     * @tparam F The task type.
     * @tparam Args Variadic list of argument types for the task.
     * @param first_index The first index in the loop.
     * @param last_index The last index in the loop.
     * @param task A function to be executed on a thread from the pool. MUST have a first argument of the index.
     * @param num_blocks The maximum number of blocks to split the task into. Default is 0, which means blocks will be
     *                   equal to the number of threads in the pool.
     * @param args Variadic list of arguments for the task.
     */
    template<typename I, typename F, typename... Args>
    void AddLoopTask(I first_index, I last_index, F&& task, std::size_t num_blocks, Args&&... args)
    {
        _pool->detach_loop(
            first_index, last_index, [=](auto&& idx) { task(idx, std::forward<Args>(args)...); }, num_blocks);
    }

    /**
     * @brief Parallelise a loop task into blocks, one range at a time.
     *
     * @tparam I The index type.
     * @tparam F The task type.
     * @tparam Args Variadic list of argument types for the task.
     * @param first_index The first index in the range.
     * @param last_index The last index in the range.
     * @param task A function to be executed on a thread from the pool. MUST have two arguments, which are the start and
     *             end indices.
     * @param num_blocks The maximum number of blocks to split the task into. Default is 0, which means blocks will be
     *                   equal to the number of threads in the pool.
     * @param args Variadic list of arguments for the task.
     */
    template<typename I, typename F, typename... Args>
    void AddBlocksTask(I first_index, I last_index, F&& task, std::size_t num_blocks, Args&&... args)
    {
        _pool->detach_blocks(
            first_index, last_index, [=](auto&& start, auto&& end) { task(start, end, std::forward<Args>(args)...); },
            num_blocks);
    }

    /**
     * @brief Returns a system environment variable value.
     *
     * @param name The name of a system environment variable.
     * @returns The value of the environment variable.
     */
    [[nodiscard]] std::string GetVar(const std::string& name) const;

  private:
    /// The node factory to use for constructing available nodes.
    std::shared_ptr<NodeFactory> _factory;

    /// The thread pool to use for executing graphs.
    std::unique_ptr<thread_pool> _pool;
};

FLOW_NAMESPACE_END
