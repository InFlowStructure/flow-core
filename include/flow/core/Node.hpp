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

    virtual json SaveInputs() const { return {}; }
    virtual void RestoreInputs(const json&) {}

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
    EventDispatcher<> OnCompute;
    EventDispatcher<const IndexableName&, const SharedNodeData&> OnSetInput;
    EventDispatcher<const IndexableName&, const SharedNodeData&> OnSetOutput;
    EventDispatcher<const std::exception&> OnError;
    EventDispatcher<const UUID&, const IndexableName&, const SharedNodeData&> OnEmitOutput;

  protected:
    mutable std::mutex _mutex;

  private:
    UUID _id;
    std::string _class_name;
    std::string _name;

    std::shared_ptr<Env> _env;

    PortMap _input_ports;
    PortMap _output_ports;
};

#define OVERLOAD_PORT_TYPE(Original, As)                                                                               \
    template<>                                                                                                         \
    inline auto FLOW_NAMESPACE::Node::GetInputData<Original>(const IndexableName& key) const noexcept                  \
    {                                                                                                                  \
        return GetInputData<As>(key);                                                                                  \
    }                                                                                                                  \
    template<>                                                                                                         \
    inline auto FLOW_NAMESPACE::Node::GetOutputData<Original>(const IndexableName& key) const noexcept                 \
    {                                                                                                                  \
        return GetOutputData<As>(key);                                                                                 \
    }                                                                                                                  \
    template<>                                                                                                         \
    inline void FLOW_NAMESPACE::Node::AddInput<Original>(std::string_view key, const std::string& caption,             \
                                                         SharedNodeData data)                                          \
    {                                                                                                                  \
        return AddInput<As>(key, caption, std::move(data));                                                            \
    }                                                                                                                  \
    template<>                                                                                                         \
    inline void FLOW_NAMESPACE::Node::AddOutput<Original>(std::string_view key, const std::string& caption,            \
                                                          SharedNodeData data)                                         \
    {                                                                                                                  \
        return AddOutput<As>(key, caption, std::move(data));                                                           \
    }

/**
 * Specialise const char* to use std::string for type safety.
 */
OVERLOAD_PORT_TYPE(const char*, std::string);

FLOW_NAMESPACE_END
