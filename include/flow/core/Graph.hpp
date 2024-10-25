// Copyright (c) 2024, Cisco Systems, Inc.
// All rights reserved.

#pragma once

#include "Connections.hpp"
#include "Core.hpp"
#include "IndexableName.hpp"
#include "Node.hpp"

#include <mutex>
#include <unordered_map>
#include <vector>

FLOW_NAMESPACE_START

class Graph
{
  public:
    using VisitorFunction = std::function<void(const SharedNode&)>;

    Graph(const std::string& name, std::shared_ptr<Env> env);

    const UUID& ID() const noexcept { return _id; }

    void Run();

    void Visit(const VisitorFunction& visitor);

    void AddNode(SharedNode node);

    void RemoveNode(const SharedNode& node);
    void RemoveNodeByUUID(const UUID& uuid);

    [[nodiscard]] SharedNode GetNode(const UUID& uuid) const;
    [[nodiscard]] const Connections& GetConnections() const noexcept { return _connections; }

    [[nodiscard]] std::size_t Size() const noexcept { return _nodes.size(); }
    [[nodiscard]] std::size_t ConnectionCount() const noexcept { return _connections.Size(); }

    [[nodiscard]] const std::string& GetName() const noexcept { return _name; }
    [[nodiscard]] const std::shared_ptr<Env>& GetEnv() const noexcept { return _env; }
    [[nodiscard]] std::vector<SharedNode> GetSourceNodes() const;
    [[nodiscard]] std::vector<SharedNode> GetLeafNodes() const;
    [[nodiscard]] std::vector<SharedNode> GetOrphanNodes() const;

    void Clear() noexcept { _connections.Erase(), _nodes.clear(); }

    SharedConnection ConnectNodes(const UUID& in, const IndexableName& in_port_key, const UUID& out,
                                  const IndexableName& out_port_key);

    void DisconnectNodes(const UUID& in, const IndexableName& in_port_key, const UUID& out,
                         const IndexableName& out_port_key);

    void PropagateConnectionsData(const UUID& id, const IndexableName& key, SharedNodeData data);

    void SetName(std::string new_name) noexcept { _name = std::move(new_name); }

    friend void to_json(json& j, const Graph& g);
    friend void from_json(const json& j, Graph& g);

  protected:
    mutable std::mutex _nodes_mutex;

    const UUID _id;
    std::string _name;
    std::shared_ptr<Env> _env;

    Connections _connections;
    std::unordered_map<UUID, SharedNode> _nodes;
};

FLOW_NAMESPACE_END
