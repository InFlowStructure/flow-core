// Copyright (c) 2024, Cisco Systems, Inc.
// All rights reserved.

#include "flow/core/Node.hpp"

#include "flow/core/Env.hpp"

#include <nlohmann/json.hpp>

#include <memory>
#include <stdarg.h>
#include <utility>
#include <vector>

FLOW_NAMESPACE_START

Node::Node(const UUID& uuid, std::string_view class_name, std::string_view name, std::shared_ptr<Env> env)
    : _id{uuid}, _class_name{class_name}, _name{name}, _env{std::move(env)}
{
}

void Node::InvokeCompute() noexcept
try
{
    Compute();
    OnCompute.Broadcast();
}
catch (const std::exception& e)
{
    OnError.Broadcast(e);
}
catch (const std::string& e)
{
    OnError.Broadcast(std::runtime_error(e));
}
catch (const char* e)
{
    OnError.Broadcast(std::runtime_error(e));
}
catch (int e)
{
    OnError.Broadcast(std::runtime_error(std::to_string(e)));
}
catch (...)
{
    OnError.Broadcast(std::exception());
}

json Node::Save() const
{
    return {
        {"id", std::string(_id)},
        {"class", _class_name},
        {"name", _name},
        {"inputs", SaveInputs()},
    };
}

void Node::Restore(const json& p)
{
    if (!(p.contains("id") && p.contains("class") && p.contains("name")))
    {
        throw std::runtime_error("Invalid node in flow");
    }

    _id         = p["id"].get_ref<const std::string&>();
    _class_name = p["class"].get_ref<const std::string&>();
    _name       = p["name"].get_ref<const std::string&>();

    if (p.contains("inputs"))
    {
        RestoreInputs(p["inputs"]);
    }
}

json Node::SaveInputs() const { return {}; }

void Node::RestoreInputs(const json&) {}

void Node::AddInput(std::string_view key, const std::string& caption, std::string_view type, SharedNodeData data)
{
    _input_ports.emplace(key, std::make_shared<Port>(key, caption, type, std::move(data),
                                                     type.at(type.length() - 1) == '&', _input_ports.size()));
}

void Node::AddOutput(std::string_view key, const std::string& caption, std::string_view type, SharedNodeData data)
{
    _output_ports.emplace(key, std::make_shared<Port>(key, caption, type, std::move(data),
                                                      type.at(type.length() - 1) == '&', _output_ports.size()));
}

const SharedPort& Node::GetInputPort(const IndexableName& key) const { return _input_ports.at(key); }

const SharedPort& Node::GetOutputPort(const IndexableName& key) const { return _output_ports.at(key); }

const SharedNodeData& Node::GetInputData(const IndexableName& key) const { return _input_ports.at(key)->GetData(); }

const SharedNodeData& Node::GetOutputData(const IndexableName& key) const { return _output_ports.at(key)->GetData(); }

void Node::SetInputData(const IndexableName& key, SharedNodeData data, bool compute)
{
    _input_ports.at(key)->SetData(data);

    OnSetInput.Broadcast(key, data);

    if (compute)
    {
        InvokeCompute();
    }
}

void Node::SetOutputData(const IndexableName& key, SharedNodeData data, bool emit)
{
    _output_ports.at(key)->SetData(data, true);

    OnSetOutput.Broadcast(key, data);

    if (emit)
    {
        EmitUpdate(key, data);
    }
}

void Node::EmitUpdate(const IndexableName& key, const SharedNodeData& data)
{
    _propagate_output_update(ID(), key, data);
    OnEmitOutput.Broadcast(ID(), key, data);
}

FLOW_NAMESPACE_END
