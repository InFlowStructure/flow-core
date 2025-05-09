// Copyright (c) 2024, Cisco Systems, Inc.
// All rights reserved.

#pragma once

#include "Connection.hpp"
#include "Core.hpp"
#include "Graph.hpp"
#include "Module.hpp"
#include "UUID.hpp"

#include <BS_thread_pool.hpp>
#include <nlohmann/json_fwd.hpp>

#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>

using json        = nlohmann::json;
using thread_pool = BS::thread_pool<>;

FLOW_NAMESPACE_START

class Node;
class NodeFactory;

/**
 * @brief Flow environment
 *
 * @details Env represents the environment in which all flows run. It holds the thread pool, provides easy access to
 *          system env vars, and more.
 */
class Env : public std::enable_shared_from_this<Env>
{
    explicit Env(std::shared_ptr<NodeFactory> factory);

  public:
    using VisitorFunction = std::function<void(const SharedNode&)>;

    Env(const Env&) = delete;

    /**
     * @brief Creator method which constructs only shared pointers.
     */
    static std::shared_ptr<Env> Create(std::shared_ptr<NodeFactory> factory)
    {
        return std::shared_ptr<Env>(new Env(std::move(factory)));
    }

    /**
     * @brief Load custom Flow module.
     * @param module_file The module file to load.
     */
    const std::shared_ptr<Module>& LoadModule(const std::filesystem::path& module_file);

    /**
     * @brief Unloads a specified Flow module that has previously been loaded.
     * @param module_filename The module filename to unload.
     */
    void UnloadModule(const std::filesystem::path& module_filename);

    /**
     * @brief Gets the current factory for building nodes.
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
     * @tparam F The task type
     * @tparam Args Variadic list of argument types for the task.
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
     * @tparam F The task type
     * @tparam Args Variadic list of argument types for the task.
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
     * @tparam F The task type
     * @tparam Args Variadic list of argument types for the task.
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
     * @param varname The name of a system environment variable.
     * @returns The value of the environment variable.
     */
    [[nodiscard]] std::string GetVar(const std::string& varname) const;

  private:
    /// The node factory to use for constructing available nodes.
    std::shared_ptr<NodeFactory> _factory;

    /// The thread pool to use for executing graphs.
    std::unique_ptr<thread_pool> _pool;

    /// Keyed list of loaded module handles.
    std::unordered_map<std::string, std::shared_ptr<Module>> _loaded_modules;
};

FLOW_NAMESPACE_END
