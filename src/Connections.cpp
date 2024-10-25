// Copyright (c) 2024, Cisco Systems, Inc.
// All rights reserved.

#include "Connections.hpp"

FLOW_NAMESPACE_START

SharedConnection& Connections::Add(UUID start_id, const IndexableName& start_port_key, UUID end_id,
                                   const IndexableName& end_port_key)
{
    auto connection = std::make_shared<Connection>(start_id, start_port_key, end_id, end_port_key);

    std::lock_guard<std::mutex> _(_mutex);
    auto new_conn = _connections.emplace(start_id, std::move(connection));

    return new_conn->second;
}

void Connections::Remove(const UUID& uuid)
{
    std::lock_guard<std::mutex> _(_mutex);

    auto [begin, end] = _connections.equal_range(uuid);
    if (begin == end) return;

    _connections.erase(begin, end);
}

void Connections::Remove(const UUID& uuid, const UUID& end_uuid)
{
    std::lock_guard<std::mutex> _(_mutex);

    auto [begin, end] = _connections.equal_range(uuid);
    if (begin == end) return;

    auto found = std::find_if(begin, end, [&](const auto& conn) { return conn.second->EndNodeID() == end_uuid; });
    if (found == end) return;

    _connections.erase(found);
}

void Connections::Erase() noexcept
{
    std::lock_guard<std::mutex> _(_mutex);
    _connections.clear();
}

std::vector<SharedConnection> Connections::FindConnections(const UUID& id) const
{
    std::lock_guard<std::mutex> _(_mutex);

    auto [begin, end] = _connections.equal_range(id);
    if (begin == end) return {};

    std::vector<SharedConnection> conns;
    std::transform(begin, end, std::back_inserter(conns), [](auto&& e) { return e.second; });
    return conns;
}

std::vector<SharedConnection> Connections::FindConnections(const UUID& id, const IndexableName& key) const
{
    auto connections = FindConnections(id);
    std::erase_if(connections, [&](const auto& c) { return c->StartPortKey() != key; });

    return connections;
}

std::size_t Connections::Size() const noexcept
{
    std::lock_guard _(_mutex);
    return _connections.size();
}

FLOW_NAMESPACE_END
