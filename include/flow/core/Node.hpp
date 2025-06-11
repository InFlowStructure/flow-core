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

#include <nlohmann/json_fwd.hpp>

#include <mutex>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>

FLOW_NAMESPACE_BEGIN

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

    /**
     * @brief Get all input ports for this node.
     *
     * @returns Map of input ports indexed by their keys.
     */
    [[nodiscard]] const PortMap& GetInputPorts() const noexcept { return _input_ports; }

    /**
     * @brief Get all output ports for this node.
     *
     * @returns Map of output ports indexed by their keys.
     */
    [[nodiscard]] const PortMap& GetOutputPorts() const noexcept { return _output_ports; }

    /**
     * @brief Get the data from an input port.
     *
     * @param key The unique identifier of the input port.
     *
     * @returns Reference to the shared data in the port.
     * @throws std::out_of_range if port not found.
     */
    [[nodiscard]] const SharedNodeData& GetInputData(const IndexableName& key) const;

    /**
     * @brief Get the data from an output port.
     *
     * @param key The unique identifier of the output port.
     *
     * @returns Reference to the shared data in the port.
     * @throws std::out_of_range if port not found.
     */
    [[nodiscard]] const SharedNodeData& GetOutputData(const IndexableName& key) const;

    /**
     * @brief Get typed data from an input port.
     *
     * @tparam T The expected type of the data.
     *
     * @param key The unique identifier of the input port.
     *
     * @returns Typed shared pointer to the data, or nullptr if type mismatch.
     */
    template<typename T>
    [[nodiscard]] auto GetInputData(const IndexableName& key) const noexcept
    {
        return CastNodeData<T>(this->GetInputData(key));
    }

    /**
     * @brief Get typed data from an output port.
     *
     * @tparam T The expected type of the data.
     *
     * @param key The unique identifier of the output port.
     *
     * @returns Typed shared pointer to the data, or nullptr if type mismatch.
     */
    template<typename T>
    [[nodiscard]] auto GetOutputData(const IndexableName& key) const noexcept
    {
        return CastNodeData<T>(this->GetOutputData(key));
    }

    /**
     * @brief Set data on an input port.
     *
     * @param key The unique identifier of the input port.
     * @param data The data to set.
     * @param compute Whether to trigger computation after setting data.
     */
    void SetInputData(const IndexableName& key, SharedNodeData data, bool compute = true);

    /**
     * @brief Set data on an output port.
     *
     * @param key The unique identifier of the output port.
     * @param data The data to set, nullptr to clear.
     * @param emit Whether to emit the update to connected nodes.
     */
    void SetOutputData(const IndexableName& key, SharedNodeData data = nullptr, bool emit = true);

    /**
     * @brief Get an input port by its key.
     *
     * @param key The unique identifier of the input port.
     * @returns Reference to the port.
     * @throws std::out_of_range if port not found.
     */
    const SharedPort& GetInputPort(const IndexableName& key) const;

    /**
     * @brief Get an output port by its key.
     *
     * @param key The unique identifier of the output port.
     * @returns Reference to the port.
     * @throws std::out_of_range if port not found.
     */
    const SharedPort& GetOutputPort(const IndexableName& key) const;

    /**
     * @brief Serialize node state to JSON.
     *
     * @returns JSON object containing node state.
     */
    json Save() const;

    /**
     * @brief Restore node state from JSON.
     *
     * @param j JSON object containing node state.
     */
    void Restore(const json& j);

    /**
     * @brief Lock the node mutex.
     */
    void lock() { _mutex.lock(); }

    /**
     * @brief Unlock the node mutex.
     */
    void unlock() { _mutex.unlock(); }

  protected:
    virtual void Compute() = 0;

    virtual json SaveInputs() const;
    virtual void RestoreInputs(const json&);

  protected:
    void AddInput(std::string_view key, const std::string& caption, std::string_view type, SharedNodeData data);

    void AddOutput(std::string_view key, const std::string& caption, std::string_view type, SharedNodeData data);

    /**
     * @brief Add an input port with a template type.
     *
     * @tparam T The type of data for this input port
     *
     * @param key The unique identifier for this port
     * @param caption A friendly display name for the port
     * @param data Optional initial data
     */
    template<typename T>
    void AddInput(std::string_view key, const std::string& caption, SharedNodeData data = nullptr)
    {
        return AddInput(key, caption, TypeName_v<T>, std::move(data));
    }

    /**
     * @brief Add a required input port with a template type.
     *
     * @tparam T The type of data for this input port
     * @param key The unique identifier for this port
     *
     * @param caption A friendly display name for the port
     * @param data A reference to the data to be used as initial value
     */
    template<typename T>
    void AddRequiredInput(std::string_view key, const std::string& caption, std::remove_reference_t<T>& data)
    {
        return AddInput(key, caption, TypeName_v<T>, MakeRefNodeData<T>(data));
    }

    /**
     * @brief Add an output port with a template type.
     *
     * @tparam T The type of data for this output port
     *
     * @param key The unique identifier for this port
     * @param caption A friendly display name for the port
     * @param data Optional initial data
     */
    template<typename T>
    void AddOutput(std::string_view key, const std::string& caption, SharedNodeData data = nullptr)
    {
        return AddOutput(key, caption, TypeName_v<T>, std::move(data));
    }

    /**
     * @brief Emit an update for the given output port.
     *
     * @param key The port identifier to emit from
     * @param data The data to emit
     */
    void EmitUpdate(const IndexableName& key, const SharedNodeData& data);

  public:
    /// Event triggered when Compute() is called
    EventDispatcher<> OnCompute;

    /// Event triggered when an input port receives new data
    EventDispatcher<const IndexableName&, const SharedNodeData&> OnSetInput;

    /// Event triggered when an output port's data is updated
    EventDispatcher<const IndexableName&, const SharedNodeData&> OnSetOutput;

    /// Event triggered when Compute() throws an exception
    EventDispatcher<const std::exception&> OnError;

    /// Event triggered when an output update is emitted through the graph
    EventDispatcher<const UUID&, const IndexableName&, const SharedNodeData&> OnEmitOutput;

  protected:
    /// Mutex for thread-safe operations on node data
    mutable std::mutex _mutex;

    /// Event for graph to handle output propagation
    Event<const UUID&, const IndexableName&, const SharedNodeData&> _propagate_output_update;

    friend class Graph;

  private:
    /// Unique identifier for this node
    UUID _id;

    /// Type name of the concrete node class
    std::string _class_name;

    /// User-friendly display name
    std::string _name;

    /// Shared environment reference
    std::shared_ptr<Env> _env;

    /// Collection of input ports mapped by their keys
    PortMap _input_ports;

    /// Collection of output ports mapped by their keys
    PortMap _output_ports;
};

FLOW_NAMESPACE_END
