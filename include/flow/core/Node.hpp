// Copyright (c) 2024, Cisco Systems, Inc.
// All rights reserved.

#pragma once

#include "Concepts.hpp"
#include "Core.hpp"
#include "Event.hpp"
#include "IndexableName.hpp"
#include "NodeData.hpp"
#include "Port.hpp"
#include "UUID.hpp"

#include <nlohmann/json.hpp>

#include <mutex>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>

FLOW_NAMESPACE_START

class Env;

/**
 * @brief Type alias for a shared pointer to a Node.
 */
using SharedNode = std::shared_ptr<class Node>;

/**
 * @brief The executable node of a graph.
 *
 * @details The core class, a Node defines a portion of executable code in the graph.
 */
class Node
{
    using PortMap = std::unordered_map<IndexableName, SharedPort>;

  protected:
    /**
     * @brief Protected constructor for nodes.
     *
     * @param uuid The UUID for the node.
     * @param class_name The class name of the derived type.
     * @param name The friendly name of the node.
     * @param env The shared environment.
     */
    explicit Node(const UUID& uuid, std::string_view class_name, std::string_view name, std::shared_ptr<Env> env);

  public:
    virtual ~Node() = default;

    /**
     * @brief Get a reference to the shared Env pointer.
     * @returns The shared pointer to the Env used by the graph.
     */
    [[nodiscard]] const std::shared_ptr<Env>& GetEnv() const { return _env; }

    /**
     * @brief Get the ID for the node.
     * @returns The UUID of the node in the graph.
     */
    [[nodiscard]] const UUID& ID() const noexcept { return _id; }

    /**
     * @brief Get the friendly display name.
     * @returns A string representing the friendly name of the node that can be displayed.
     */
    [[nodiscard]] const std::string& GetName() const noexcept { return _name; }

    /**
     * @brief Get the name of the node class
     * @returns A string representing the name of the node class that is being used.
     */
    [[nodiscard]] const std::string& GetClass() const noexcept { return _class_name; }

    /**
     * @brief Set the friendly name of the node.
     * @param new_name The new friendly name of the node.
     */
    void SetName(std::string new_name) { _name = std::move(new_name); }

    /**
     * @brief Overridable method that runs after the creation but before execution of a node.
     */
    virtual void Start() {}

    /**
     * @brief Overridable method that can be run after execution of a node.
     */
    virtual void Stop() {}

    /**
     * @brief   The public facing API that calls compute for the node.
     *
     * @details Invokes the Compute method, and handles errors with grace. Forwards events related to compute
     *          completion, as well as errors.
     *
     * @note    Calls OnCompute and OnError events. This method is intended to catch all exceptions gracefully, and thus
     *          does not throw.
     */
    void InvokeCompute() noexcept;

    [[nodiscard]] const PortMap& GetInputPorts() const noexcept { return _input_ports; }
    [[nodiscard]] const PortMap& GetOutputPorts() const noexcept { return _output_ports; }

    [[nodiscard]] const SharedNodeData& GetInputData(const IndexableName& key) const;
    [[nodiscard]] const SharedNodeData& GetOutputData(const IndexableName& key) const;

    template<typename T>
    [[nodiscard]] auto GetInputData(const IndexableName& key) const noexcept
    {
        return CastNodeData<T>(this->GetInputData(key));
    }

    template<typename T>
    [[nodiscard]] auto GetOutputData(const IndexableName& key) const noexcept
    {
        return CastNodeData<T>(this->GetOutputData(key));
    }

    void SetInputData(const IndexableName& key, SharedNodeData data, bool compute = true);

    void SetOutputData(const IndexableName& key, SharedNodeData data = nullptr, bool emit = true);

    const SharedPort& GetInputPort(const IndexableName& key) const;
    const SharedPort& GetOutputPort(const IndexableName& key) const;

    json Save() const;
    void Restore(const json& j);

    void lock() { _mutex.lock(); }
    void unlock() { _mutex.unlock(); }

  protected:
    virtual void Compute() = 0;

    virtual json SaveInputs() const;
    virtual void RestoreInputs(const json&);

  protected:
    void AddInput(std::string_view key, const std::string& caption, std::string_view type, SharedNodeData data);

    void AddOutput(std::string_view key, const std::string& caption, std::string_view type, SharedNodeData data);

    template<typename T>
    void AddInput(std::string_view key, const std::string& caption, SharedNodeData data = nullptr)
    {
        return AddInput(key, caption, TypeName_v<T>, std::move(data));
    }

    template<typename T>
    void AddRequiredInput(std::string_view key, const std::string& caption, T& data)
    {
        return AddInput(key, caption, TypeName_v<T&>, MakeRefNodeData<T>(data));
    }

    template<typename T>
    void AddOutput(std::string_view key, const std::string& caption, SharedNodeData data = nullptr)
    {
        return AddOutput(key, caption, TypeName_v<T>, std::move(data));
    }

    void EmitUpdate(const IndexableName& key, const SharedNodeData& data);

  public:
    /// Event broadcasted called on Compute.
    EventDispatcher<> OnCompute;

    /// Event broadcasted on setting an input.
    EventDispatcher<const IndexableName&, const SharedNodeData&> OnSetInput;

    /// Event broadcasted on setting an output.
    EventDispatcher<const IndexableName&, const SharedNodeData&> OnSetOutput;

    /// Event broadcasted on Compute  throwing an error.
    EventDispatcher<const std::exception&> OnError;

    /// Event broadcasted on output updates being emitted.
    EventDispatcher<const UUID&, const IndexableName&, const SharedNodeData&> OnEmitOutput;

  protected:
    mutable std::mutex _mutex;

    /// Event to be used by the graph to propagate output updates.
    Event<const UUID&, const IndexableName&, const SharedNodeData&> _propagate_output_update;

    friend class Graph;

  private:
    UUID _id;
    std::string _class_name;
    std::string _name;

    std::shared_ptr<Env> _env;

    PortMap _input_ports;
    PortMap _output_ports;
};

/**
 * @brief Structure that reveals the types of the return value and arguments of a function at compile-time.
 * @tparam F The function type being analyzed.
 */
template<typename F>
struct FunctionTraits;

template<typename R, typename... Args>
struct FunctionTraits<R(Args...)>
{
    using ReturnType = std::invoke_result_t<R(Args...), Args...>;
    using ArgTypes   = std::tuple<Args...>;
};

/**
 * @brief Node class that wraps a declared function.
 *
 * @details Wraps a declared function as a node type with inputs labeled in increasing alphabetical order starting at
 *          'a'. Requires that the function node be overloaded. If the function is overloaded, then supplying the
 *          specific overload to wrap is required.
 *
 * @tparam F
 * @tparam Func
 */

template<typename F, std::add_pointer_t<std::remove_pointer_t<F>> Func>
class FunctionWrapperNode : public Node
{
  protected:
    using traits   = FunctionTraits<std::remove_pointer_t<F>>;
    using output_t = typename traits::ReturnType;
    using arg_ts   = typename traits::ArgTypes;

  private:
    template<int... Idx>
    void ParseArguments(std::integer_sequence<int, Idx...>)
    {
        (
            [&] {
                if constexpr (std::is_lvalue_reference_v<std::tuple_element_t<Idx, arg_ts>> &&
                              !std::is_const_v<std::remove_reference_t<std::tuple_element_t<Idx, arg_ts>>>)
                {
                    AddOutput<std::tuple_element_t<Idx, arg_ts>>({output_names[Idx] = 'a' + Idx}, "");
                }
                else
                {
                    AddInput<std::tuple_element_t<Idx, arg_ts>>({input_names[Idx] = 'a' + Idx}, "");
                }
            }(),
            ...);
    }

    template<int... Idx>
    auto GetInputs(std::integer_sequence<int, Idx...>)
    {
        return std::make_tuple(GetInputData<std::tuple_element_t<Idx, arg_ts>>(IndexableName{input_names[Idx]})...);
    }

    template<int... Idx>
    json SaveInputs(std::integer_sequence<int, Idx...>) const
    {
        json inputs_json;
        (
            [&, this] {
                const auto& key = input_names[Idx];
                if (auto x = GetInputData<std::tuple_element_t<Idx, arg_ts>>(IndexableName{key}))
                {
                    inputs_json[key] = x->Get();
                }
            }(),
            ...);

        return inputs_json;
    }

    template<int... Idx>
    void RestoreInputs(json& j, std::integer_sequence<int, Idx...>)
    {
        (
            [&, this] {
                const auto& key = input_names[Idx];
                if (!j.contains(key))
                {
                    return;
                }

                if constexpr (!std::is_lvalue_reference_v<std::tuple_element_t<Idx, arg_ts>>)
                {
                    SetInputData(IndexableName{key}, MakeNodeData<std::tuple_element_t<Idx, arg_ts>>(j[key]), false);
                }
            }(),
            ...);
    }

  public:
    explicit FunctionWrapperNode(const UUID& uuid, const std::string& name, std::shared_ptr<Env> env)
        : Node(uuid, TypeName_v<FunctionWrapperNode<F, Func>>, name, std::move(env)), _func{Func}
    {
        ParseArguments(std::make_integer_sequence<int, std::tuple_size_v<arg_ts>>{});
        AddOutput<output_t>("result", "result");
    }

    virtual ~FunctionWrapperNode() = default;

  protected:
    void Compute() override
    {
        auto inputs = GetInputs(std::make_integer_sequence<int, std::tuple_size_v<arg_ts>>{});

        if (std::apply([](auto&&... args) { return (!args || ...); }, inputs))
        {
            return;
        }

        auto result = std::apply([&](auto&&... args) { return _func(args->Get()...); }, inputs);
        this->SetOutputData("result", MakeNodeData(std::move(result)));
    }

    json SaveInputs() const override
    {
        return SaveInputs(std::make_integer_sequence<int, std::tuple_size_v<arg_ts>>{});
    }

    void RestoreInputs(const json& j) override
    {
        RestoreInputs(const_cast<json&>(j), std::make_integer_sequence<int, std::tuple_size_v<arg_ts>>{});
    }

  private:
    std::add_pointer_t<std::remove_pointer_t<F>> _func;
    static inline std::array<std::string, std::tuple_size_v<arg_ts>> input_names{""};
    static inline std::array<std::string, std::tuple_size_v<arg_ts>> output_names{""};
};

#define DECLARE_FUNCTION_NODE_TYPE(func) FunctionWrapperNode<decltype(func), func>

FLOW_NAMESPACE_END
