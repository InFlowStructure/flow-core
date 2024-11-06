// Copyright (c) 2024, Cisco Systems, Inc.
// All rights reserved.

#include "Graph.hpp"

#include "Env.hpp"
#include "IndexableName.hpp"
#include "NodeFactory.hpp"

#include <algorithm>
#include <set>

FLOW_NAMESPACE_START

Graph::Graph(const std::string& name, std::shared_ptr<Env> env) : _name{name}, _env{std::move(env)} {}

void Graph::Run()
{
    for (const auto& node : GetSourceNodes())
    {
        GetEnv()->AddTask([=] {
            std::lock_guard _(*node);
            node->InvokeCompute();
        });
    }
}

void Graph::Visit(const VisitorFunction& visitor)
{
    if (_nodes.empty())
    {
        return;
    }

    std::set<UUID> visited_nodes;

    auto&& source_nodes = GetSourceNodes();

    for (const auto& node : source_nodes)
    {
        if (!node) return;

        visitor(node);
        visited_nodes.insert(node->ID());
    }

    std::unordered_map<UUID, SharedNode> remaining_nodes = _nodes;
    for (std::size_t i = 0; i < visited_nodes.size(); ++i)
    {
        auto&& it = std::next(visited_nodes.begin(), i);
        if (it == visited_nodes.end()) break;

        auto connections = _connections.FindConnections(*it);
        for (auto connection : connections)
        {
            const auto& child_node_id = connection->EndNodeID();

            auto found = std::find(visited_nodes.begin(), visited_nodes.end(), child_node_id);
            if (found != visited_nodes.end()) continue;

            auto child_node = GetNode(child_node_id);

            visitor(child_node);
            visited_nodes.insert(child_node_id);
            remaining_nodes.erase(child_node_id);
        }

        remaining_nodes.erase(*it);
    }

    for (const auto& [id, node] : remaining_nodes)
    {
        visitor(node);
        visited_nodes.insert(id);
    }

    if (visited_nodes.size() != _nodes.size())
    {
        OnError.Broadcast(std::runtime_error("Failed to visit some nodes in the graph"));
    }
}

void Graph::AddNode(SharedNode node)
{
    if (!node) return;

    std::lock_guard _(_nodes_mutex);

    node->OnEmitOutput.Bind("PropagateConnectionsData",
                            [this](const UUID& id, const IndexableName& key, SharedNodeData data) {
                                this->PropagateConnectionsData(id, key, std::move(data));
                            });

    UUID id = node->ID();
    _nodes.emplace(id, std::move(node));
}

void Graph::RemoveNode(const SharedNode& node)
{
    if (!node) return;

    RemoveNodeByID(node->ID());
}

void Graph::RemoveNodeByID(const UUID& uuid)
{
    std::lock_guard _(_nodes_mutex);

    _connections.RemoveByNodeID(uuid);

    auto found = _nodes.find(uuid);
    if (found != _nodes.end())
    {
        found->second->Stop();
        _nodes.erase(found);
    }
}

SharedNode Graph::GetNode(const UUID& uuid) const
{
    std::lock_guard _(_nodes_mutex);

    auto found = _nodes.find(uuid);
    if (found != _nodes.end())
    {
        return found->second;
    }

    return nullptr;
}

const auto& is_port_connected = [](const auto& port) { return port.second->IsConnected(); };

std::vector<SharedNode> Graph::GetSourceNodes() const
{
    std::lock_guard _(_nodes_mutex);

    std::vector<SharedNode> sources;
    for (const auto& [__, node] : _nodes)
    {
        const auto& inputs       = node->GetInputPorts();
        const auto& outputs      = node->GetOutputPorts();
        const bool has_no_inputs = inputs.empty() || std::none_of(inputs.begin(), inputs.end(), is_port_connected);
        const bool has_outputs   = !outputs.empty() && std::any_of(outputs.begin(), outputs.end(), is_port_connected);

        if (has_outputs && has_no_inputs)
        {
            sources.push_back(node);
        }
    }

    return sources;
}

std::vector<SharedNode> Graph::GetLeafNodes() const
{
    std::lock_guard _(_nodes_mutex);

    std::vector<SharedNode> leaves;
    for (const auto& [__, node] : _nodes)
    {
        const auto& inputs        = node->GetInputPorts();
        const auto& outputs       = node->GetOutputPorts();
        const bool has_no_outputs = outputs.empty() || std::none_of(outputs.begin(), outputs.end(), is_port_connected);
        const bool has_inputs     = !inputs.empty() && std::any_of(inputs.begin(), inputs.end(), is_port_connected);

        if (has_inputs && has_no_outputs)
        {
            leaves.push_back(node);
        }
    }

    return leaves;
}

std::vector<SharedNode> Graph::GetOrphanNodes() const
{
    std::lock_guard _(_nodes_mutex);

    std::vector<SharedNode> orphans;
    for (const auto& [__, node] : _nodes)
    {
        const auto& inputs        = node->GetInputPorts();
        const auto& outputs       = node->GetOutputPorts();
        const bool has_no_inputs  = inputs.empty() || std::none_of(inputs.begin(), inputs.end(), is_port_connected);
        const bool has_no_outputs = outputs.empty() || std::none_of(outputs.begin(), outputs.end(), is_port_connected);

        if (has_no_inputs && has_no_outputs)
        {
            orphans.push_back(node);
        }
    }

    return orphans;
}

SharedConnection Graph::ConnectNodes(const UUID& start_id, const IndexableName& start_port_key, const UUID& end_id,
                                     const IndexableName& end_port_key)
{
    auto in_node  = GetNode(start_id);
    auto out_node = GetNode(end_id);

    if (!(in_node && out_node)) return nullptr;

    const auto start_port = in_node->GetOutputPort(start_port_key);
    const auto end_port   = out_node->GetInputPort(end_port_key);

    start_port->Connect();
    if (!end_port->Connect())
    {
        auto conns      = _connections.FindConnections(start_id, start_port_key);
        auto found_conn = std::find_if(conns.begin(), conns.end(), [&](const auto& conn) {
            return conn->EndNodeID() == end_id && conn->EndPortKey() == end_port_key;
        });

        if (found_conn != conns.end())
        {
            return *found_conn;
        }

        return nullptr;
    }

    auto&& conn = _connections.Add(start_id, start_port->GetVarName(), end_id, end_port->GetVarName());
    if (auto data = in_node->GetOutputData(start_port_key))
    {
        PropagateConnectionsData(start_id, start_port_key, std::move(data));
    }

    return conn;
}

void Graph::DisconnectNodes(const UUID& start_id, const IndexableName& start_port_key, const UUID& end_id,
                            const IndexableName& end_port_key)
{
    _connections.Remove(start_id, end_id);

    auto in_node  = GetNode(start_id);
    auto out_node = GetNode(end_id);

    if (!(in_node && out_node)) return;

    const auto start_port = in_node->GetOutputPort(start_port_key);
    const auto end_port   = out_node->GetInputPort(end_port_key);

    auto&& start_conns = _connections.FindConnections(start_id, start_port_key);
    if (start_conns.empty())
    {
        start_port->Disconnect();
    }

    end_port->Disconnect();

    out_node->SetInputData(end_port_key, nullptr);
}

void Graph::PropagateConnectionsData(const UUID& id, const IndexableName& key, SharedNodeData data)
{
    const auto& factory = GetEnv()->GetFactory();

    auto connections = _connections.FindConnections(id, key);
    for (auto it = connections.begin(); it != connections.end(); ++it)
    {
        std::weak_ptr<Connection> connection = *it;

        _env->AddTask([=, this, in_data = data] {
            try
            {
                auto conn = connection.lock();
                if (!conn)
                {
                    return;
                }

                std::lock_guard conn_lock(*conn);

                auto node = GetNode(conn->EndNodeID());
                if (!node)
                {
                    return;
                }

                std::lock_guard _(*node);
                const auto& port    = node->GetInputPort(conn->EndPortKey());
                auto converted_data = factory->Convert(in_data, port->GetDataType());

                node->SetInputData(conn->EndPortKey(), std::move(converted_data));
            }
            catch (const std::exception& e)
            {
                OnError.Broadcast(e);
            }
        });
    }
}

void to_json(json& j, const Graph& g)
{
    std::vector<json> nodes_json;

    for (const auto& [_, node] : g._nodes)
    {
        nodes_json.push_back(node->Save());
    }

    std::vector<json> connections_json;
    for (const auto& [_, connection] : g._connections)
    {
        connections_json.push_back(json{
            {"in_id", std::string(connection->StartNodeID())},
            {"in_var_name", std::string(connection->StartPortKey())},
            {"out_id", std::string(connection->EndNodeID())},
            {"out_var_name", std::string(connection->EndPortKey())},
        });
    }

    j = {
        {"nodes", nodes_json},
        {"connections", connections_json},
    };
}

void from_json(const json& j, Graph& g)
{
    json nodes = j["nodes"];
    for (auto& el : nodes.items())
    {
        json node_json;

        // Handle Legacy flow json
        if (el.value().contains("model"))
        {
            node_json["id"]       = el.value()["id"];
            node_json["class"]    = el.value()["model"]["class"];
            node_json["name"]     = el.value()["model"]["name"];
            node_json["position"] = el.value()["position"];
        }
        else
        {
            node_json = el.value();
        }

        auto node = g.GetNode(UUID{node_json["id"]});
        if (!node)
        {
            node = g.GetEnv()->GetFactory()->CreateNode(
                node_json["class"], UUID{node_json["id"].get_ref<const std::string&>()}, node_json["name"], g.GetEnv());
        }

        if (!node)
        {
            continue;
        }

        node->Restore(node_json);
        g.AddNode(node);
    }

    const json& connections = j["connections"];
    for (auto& el : connections.items())
    {
        UUID inUUID(el.value()["in_id"]), outUUID(el.value()["out_id"]);

        IndexableName inKey{
            std::string(el.value().contains("in_key") ? el.value()["in_key"] : el.value()["in_var_name"])};
        IndexableName outKey{
            std::string(el.value().contains("out_key") ? el.value()["out_key"] : el.value()["out_var_name"])};

        g.ConnectNodes(inUUID, inKey, outUUID, outKey);
    }
}

FLOW_NAMESPACE_END
