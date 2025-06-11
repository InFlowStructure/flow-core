// Copyright (c) 2024, Cisco Systems, Inc.
// All rights reserved.

#include "flow/core/Connection.hpp"

#include <nlohmann/json.hpp>

FLOW_NAMESPACE_BEGIN

Connection::Connection(UUID& start_node_id, const IndexableName& start_port_key, UUID& end_node_id,
                       const IndexableName& end_port_key)
    : _id{UUID{}}, _start_node_id(start_node_id), _start_port_key{start_port_key}, _end_node_id{end_node_id},
      _end_port_key{end_port_key}
{
}

json Connection::Save() const
{
    return {
        {"in_id", std::string(_start_node_id)},
        {"in_var_name", std::string(_start_port_key)},
        {"out_id", std::string(_end_node_id)},
        {"out_var_name", std::string(_end_port_key)},
    };
}

void Connection::Restore(const json& j)
{
    _start_node_id  = j["in_id"].get_ref<const std::string&>();
    _start_port_key = {std::string_view(j["in_var_name"].get_ref<const std::string&>())};
    _end_node_id    = j["out_id"].get_ref<const std::string&>();
    _end_port_key   = {std::string_view(j["out_var_name"].get_ref<const std::string&>())};
}

FLOW_NAMESPACE_END
