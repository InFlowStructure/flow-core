// Copyright (c) 2024, Cisco Systems, Inc.
// All rights reserved.

#pragma once

#include "Core.hpp"
#include "IndexableName.hpp"
#include "Port.hpp"
#include "UUID.hpp"

#include <nlohmann/json.hpp>

#include <mutex>
#include <string_view>

FLOW_NAMESPACE_START

using json = nlohmann::json;

/**
 * @brief Defines a connection between ports on different nodes.
 *
 * @details Connection is the representation of the connection between 2 ports, each on a separate Node. A connection
 *          can be locked to preserve calculation ordering.
 */
class Connection
{
  public:
    Connection()  = delete;
    ~Connection() = default;

    Connection(UUID& start_node_id, const IndexableName& start_port_key, UUID& end_node_id,
               const IndexableName& end_port_key);

    void lock() { _mutex.lock(); }
    void unlock() noexcept { _mutex.unlock(); }
    [[nodiscard]] bool try_lock() noexcept { return _mutex.try_lock(); }

    [[nodiscard]] const UUID& StartNodeID() const noexcept { return _start_node_id; }
    [[nodiscard]] const IndexableName& StartPortKey() const noexcept { return _start_port_key; }

    [[nodiscard]] const UUID& EndNodeID() const noexcept { return _end_node_id; }
    [[nodiscard]] const IndexableName& EndPortKey() const noexcept { return _end_port_key; }

    [[nodiscard]] const UUID& ID() const noexcept { return _id; }

    json Save() const;

    void Restore(const json& j);

  private:
    std::mutex _mutex;

    UUID _id;

    UUID _start_node_id;
    IndexableName _start_port_key;

    UUID _end_node_id;
    IndexableName _end_port_key;
};

using SharedConnection = std::shared_ptr<Connection>;

FLOW_NAMESPACE_END
