// Copyright (c) 2024, Cisco Systems, Inc.
// All rights reserved.

#pragma once

#include "Connections.hpp"
#include "Core.hpp"
#include "Event.hpp"
#include "IndexableName.hpp"
#include "Node.hpp"

#include <nlohmann/json_fwd.hpp>

#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

FLOW_NAMESPACE_BEGIN

/**
 * @brief The core of the engine, holds nodes and their connections.
 */
class Graph
{
  public:
    /**
     * @brief Alias for the function type pass to the Visit method.
     */
    using VisitorFunction = std::function<void(const SharedNode&)>;

    /**
     * @brief Constructs a graph.
     * @param name The name of the graph.
     * @param env The shared environment.
     */
    Graph(const std::string& name, std::shared_ptr<Env> env);

    /**
     * @brief Get a reference to the UUID of the Graph.
     * @returns The UUID of the graph.
     */
    [[nodiscard]] const UUID& ID() const noexcept { return _id; }

    /**
     * @brief Runs compute on the source nodes of the graph, starting the flow.
     *
     * @details Finds all of the source nodes of the graph, and runs compute. The compute of the source nodes will then
     *          propagate through the rest of the graph and execute the entire flow.
     */
    void Run();

    /**
     * @brief Visits each node int he graph breadth-wise.
     * @param visitor The visitor function to run when visiting each node.
     *
     * @note This also visits orphan nodes.
     */
    void Visit(const VisitorFunction& visitor);

    /**
     * @brief Adds a new node to the graph.
     * @param node The node to be added.
     */
    void AddNode(SharedNode node);

    /**
     * @brief Removes a given node form the graph.
     * @param node The node to remove.
     */
    void RemoveNode(const SharedNode& node);

    /**
     * @brief Removes a node from the graph by its UUID.
     * @param uuid The UUID of the node.
     */
    void RemoveNodeByID(const UUID& uuid);

    /**
     * @brief Get a node by its UUID.
     * @param uuid The UUID of the node.
     * @returns The node if found, nullptr otherwise.
     */
    [[nodiscard]] SharedNode GetNode(const UUID& uuid) const;

    /**
     * @brief Get all connections for the graph.
     * @returns All connections that have been made within the graph.
     */
    [[nodiscard]] const Connections& GetConnections() const noexcept { return _connections; }

    /**
     * @brief Gets the size of the graph.
     * @returns The total number of nodes in the graph.
     */
    [[nodiscard]] std::size_t Size() const noexcept { return _nodes.size(); }

    /**
     * @brief Get the number of connections in the graph.
     * @returns The total number of connections that have been made in the graph.
     */
    [[nodiscard]] std::size_t ConnectionCount() const noexcept { return _connections.Size(); }

    /**
     * @brief Get the friendly name of the graph.
     * @returns The set name of the graph.
     */
    [[nodiscard]] const std::string& GetName() const noexcept { return _name; }

    /**
     * @brief Get a reference to the shared environment.
     * @returns The shared environment pointer.
     */
    [[nodiscard]] const std::shared_ptr<Env>& GetEnv() const noexcept { return _env; }

    /**
     * @brief Get all nodes in the graph
     * @returns Const reference to the node map
     */
    [[nodiscard]] const auto& GetNodes() const noexcept { return _nodes; }

    /**
     * @brief Get all source nodes in the graph.
     * @details Gets all source nodes. A source node is a node that has no or only output connections.
     * @returns A list of all source nodes.
     */
    [[nodiscard]] std::vector<SharedNode> GetSourceNodes() const;

    /**
     * @brief Get all leaf nodes in the graph.
     * @details Gets all leaf nodes. A leaf node is a node that has at least one connection, and only input connections.
     * @returns A list of all leaf nodes.
     */
    [[nodiscard]] std::vector<SharedNode> GetLeafNodes() const;

    /**
     * @brief Get all orphan nodes in the graph.
     * @details Gets all orphan nodes. A orphan node is a node that has no connections.
     * @returns A list of all orphan nodes.
     */
    [[nodiscard]] std::vector<SharedNode> GetOrphanNodes() const;

    /**
     * @brief Removes all connections and nodes from the graph.
     */
    void Clear() noexcept { _connections.Clear(), _nodes.clear(); }

    /**
     * @brief Check if the nodes can be connected
     */
    bool CanConnectNode(const UUID& start, const IndexableName& start_key, const UUID& end,
                                  const IndexableName& end_key);

    /**
     * @brief Connects 2 nodes by their IDs and Port keys.
     *
     * @param start The node which has the output port.
     * @param start_key The Port key of the output port on the starting node.
     * @param end The node which takes in data as input.
     * @param end_key The Port key of the input node on the end node.
     *
     * @returns The created connection.
     */
    SharedConnection ConnectNodes(const UUID& start, const IndexableName& start_key, const UUID& end,
                                  const IndexableName& end_key);

    /**
     * @brief Disconnects 2 nodes by their IDs and Port keys.
     *
     * @param start The node which has the output port.
     * @param start_key The Port key of the output port on the starting node.
     * @param end The node which takes in data as input.
     * @param end_key The Port key of the input node on the end node.
     */
    void DisconnectNodes(const UUID& start, const IndexableName& start_key, const UUID& end,
                         const IndexableName& end_key);

    /**
     * @brief Propagates data through the connections of the given ID.
     *
     * @param id The ID of the connection where the data came from.
     * @param key The name of the port from which data is flowing.
     * @param data The data to propagate.
     */
    void PropagateConnectionsData(const UUID& id, const IndexableName& key, SharedNodeData data);

    /**
     * @brief Sets the name of the graph.
     * @param new_name The new Name of the graph.
     */
    void SetName(std::string new_name) noexcept { _name = std::move(new_name); }

    /**
     * @brief Internal helper to validate node before operations
     * @param node Node to validate
     * @returns true if node is valid and belongs to this graph
     */
    [[nodiscard]] bool ValidateNode(const SharedNode& node) const noexcept
    {
        return node && _nodes.contains(node->ID());
    }

    /**
     * @brief Convert graph state to JSON.
     * @param j JSON object to store state in.
     * @param g Graph to serialize.
     */
    friend void to_json(json& j, const Graph& g);

    /**
     * @brief Restore graph state from JSON.
     * @param j JSON object containing serialized state.
     * @param g Graph to restore state into.
     */
    friend void from_json(const json& j, Graph& g);

  public:
    /// Event run on Graph errors being thrown.
    EventDispatcher<const std::exception&> OnError;

    /// Event run on Graph when a new node is added.
    EventDispatcher<const SharedNode&> OnNodeAdded;

    /// Event run on Graph when a new node is removed.
    EventDispatcher<const SharedNode&> OnNodeRemoved;

    /// Event run when 2 nodes are connected.
    EventDispatcher<const SharedConnection&> OnNodesConnected;

    /// Event run on Graph when a connection is removed.
    EventDispatcher<const SharedConnection&> OnNodesDisconnected;

  protected:
    /// Mutex for thread-safe node operations
    mutable std::mutex _nodes_mutex;

    /// Unique identifier for this graph instance
    const UUID _id;

    /// User-friendly name for the graph
    std::string _name;

    /// Shared environment for all nodes in the graph
    std::shared_ptr<Env> _env;

    /// Storage for all connections between nodes
    Connections _connections;

    /// Map of node UUIDs to node instances
    std::unordered_map<UUID, SharedNode> _nodes;
};

FLOW_NAMESPACE_END
