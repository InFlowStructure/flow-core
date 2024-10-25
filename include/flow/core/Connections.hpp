// Copyright (c) 2024, Cisco Systems, Inc.
// All rights reserved.

#pragma once

#include "Connection.hpp"
#include "Core.hpp"

#include <string_view>

FLOW_NAMESPACE_START

class Connections
{
  public:
    Connections() = default;

    SharedConnection& Add(UUID start_id, const IndexableName& start_port_key, UUID end_id,
                          const IndexableName& end_port_key);

    void Remove(const UUID& id);
    void Remove(const UUID& start_id, const UUID& end_id);

    std::vector<SharedConnection> FindConnections(const UUID& id) const;
    std::vector<SharedConnection> FindConnections(const UUID& id, const IndexableName& key) const;

    void Erase() noexcept;

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
