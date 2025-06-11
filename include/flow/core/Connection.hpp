// Copyright (c) 2024, Cisco Systems, Inc.
// All rights reserved.

#pragma once

#include "Core.hpp"
#include "IndexableName.hpp"
#include "Port.hpp"
#include "UUID.hpp"

#include <nlohmann/json_fwd.hpp>

#include <mutex>
#include <string_view>

FLOW_NAMESPACE_BEGIN

using json = nlohmann::json;

/**
 * @brief Type alias for a shared pointer to a Connection.
 */
using SharedConnection = std::shared_ptr<class Connection>;

/**
 * @brief Defines a connection between ports on different nodes.
 *
 * @details Connection is the representation of the connection between 2 ports, each on a separate Node. A connection
 *          can be locked to preserve calculation ordering. A connection between 2 ports MUST be unique.
 */
class Connection
{
  public:
    Connection()  = delete;
    ~Connection() = default;

    /**
     * @brief Constructs a connection between two ports on two different nodes.
     * @param start_node_id The UUID of the node from which output is being emitted.
     * @param start_port_key The Port key of the output port.
     * @param end_node_id The UUID of the node to which data will flow.
     * @param end_port_key The Port key of the input port.
     */
    Connection(UUID& start_node_id, const IndexableName& start_port_key, UUID& end_node_id,
               const IndexableName& end_port_key);

    /**
     * @brief Locks the connection.
     *
     * @details Locks the connection and prevents it from being used to flow data on until the previous computation is
     *          complete.
     */
    void lock() { _mutex.lock(); }

    /**
     * @brief Unlocks the connection.
     *
     * @details Frees the connection to be used to flow data from another computation.
     */
    void unlock() noexcept { _mutex.unlock(); }

    /**
     * @brief Attempts to lock a connection. Returns immediately.
     * @returns true if the connection was locked successfully, false otherwise.
     */
    [[nodiscard]] bool try_lock() noexcept { return _mutex.try_lock(); }

    /**
     * @brief Gets a reference to the UUID of the node from which the data flows.
     * @returns The outputting node's UUID.
     */
    [[nodiscard]] const UUID& StartNodeID() const noexcept { return _start_node_id; }

    /**
     * @brief Gets a reference to the key of  Port from which the data flows.
     * @returns The Port key of the output Port.
     */
    [[nodiscard]] const IndexableName& StartPortKey() const noexcept { return _start_port_key; }

    /**
     * @brief Gets a reference to the UUID of the node to which data flows.
     * @returns The receiving node's UUID.
     */
    [[nodiscard]] const UUID& EndNodeID() const noexcept { return _end_node_id; }

    /**
     * @brief Gets a reference to the key of the Port to which data flows.
     * @returns The Port key of the input Port.
     */
    [[nodiscard]] const IndexableName& EndPortKey() const noexcept { return _end_port_key; }

    /**
     * @brief Get a reference to the UUID of the connection.
     * @returns The UUID of the connection.
     */
    [[nodiscard]] const UUID& ID() const noexcept { return _id; }

    /**
     * @brief Converts the connection into a JSON object.
     * @returns The constructed JSON object.
     */
    json Save() const;

    /**
     * @brief Sets the values of the connection from a JSON object matching the format from Save.
     * @param j The JSON object representing the connection.
     */
    void Restore(const json& j);

  private:
    std::mutex _mutex;

    UUID _id;

    UUID _start_node_id;
    IndexableName _start_port_key;

    UUID _end_node_id;
    IndexableName _end_port_key;
};

FLOW_NAMESPACE_END
