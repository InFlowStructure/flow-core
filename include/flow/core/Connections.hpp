// Copyright (c) 2024, Cisco Systems, Inc.
// All rights reserved.

#pragma once

#include "Connection.hpp"
#include "Core.hpp"

#include <mutex>
#include <string_view>
#include <unordered_map>

FLOW_NAMESPACE_BEGIN

/**
 * @brief Container for connections.
 *
 * @details Maps connections together. A port that produce output can have multiple connections to several inputs, but
 *          a port that takes input can only have one conneciton. Thus, connections are mapped with the output Port's
 *          UUID being the key.
 */
class Connections
{
  public:
    Connections() = default;

    /**
     * @brief Creates a new connection and adds it to the container.
     *
     * @details Adds a new onncetion to the container, keyed by the start_id. If the key already exists, adds the new
     *          connection to the multimap. Otherwise, creates the new entry with 1 connection in the valu list.
     *
     * @param start_id The UUID of the node from which the data flows.
     * @param start_port_key The key of the Port from which the data flows.
     * @param end_id The UUID of the node to which emitted data flows.
     * @param end_port_key The key of the Port to which data flows.
     *
     * @returns A reference to the newly created connection.
     */
    SharedConnection& Add(UUID start_id, const IndexableName& start_port_key, UUID end_id,
                          const IndexableName& end_port_key);

    /**
     * @brief Remove the connection by its given UUID.
     * @param id The UUID of the connection to remove.
     */
    void Remove(const UUID& id);

    /**
     * @brief Remove all connections under the key of id.
     * @param id The UUID of the node from which data flows.
     */
    void RemoveByNodeID(const UUID& id);

    /**
     * @brief Remove a specific connection.
     * @param start_id The UUID of the ndoe from which data flows.
     * @param end_id The UUID of the node to which fata flows.
     */
    void Remove(const UUID& start_id, const UUID& end_id);

    /**
     * @brief Find all connections for a given Node.
     * @param id The UUID of the node from which data flows.
     * @returns All connections connected to the given Node.
     */
    std::vector<SharedConnection> FindConnections(const UUID& id) const;

    /**
     * @brief Find all connections for a given Node that flow from the Port matching the given key.
     * @param id The UUID of the node from which data flows.
     * @param key The Port key from which data flows.
     * @returns All connections connected to the given Node matching the Port key.
     */
    std::vector<SharedConnection> FindConnections(const UUID& id, const IndexableName& key) const;

    /**
     * @brief Remove all connections.
     */
    void Clear() noexcept;

    /**
     * @brief Get the total number of connections.
     * @returns The total number of connections in the container.
     */
    std::size_t Size() const noexcept;

    auto begin() noexcept { return _connections.begin(); }
    auto end() noexcept { return _connections.end(); }
    auto begin() const noexcept { return _connections.begin(); }
    auto end() const noexcept { return _connections.end(); }

  private:
    mutable std::mutex _mutex;
    std::unordered_multimap<UUID, SharedConnection> _connections;
};

FLOW_NAMESPACE_END
