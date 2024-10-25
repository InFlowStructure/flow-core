// Copyright (c) 2024, Cisco Systems, Inc.
// All rights reserved.

#pragma once

#include "Core.hpp"
#include "IndexableName.hpp"
#include "NodeData.hpp"

#include <nlohmann/json.hpp>

#include <stdint.h>
#include <string_view>

using json = nlohmann::json;

FLOW_NAMESPACE_START

class Port
{
  public:
    Port(const IndexableName& key, const std::string& caption, std::string_view type, SharedNodeData data,
         bool required, std::size_t index);
    Port(const Port&)            = default;
    Port(Port&&)                 = default;
    Port& operator=(const Port&) = default;
    Port& operator=(Port&&)      = default;

    bool IsConnected() const noexcept { return _connected; }
    bool Connect() noexcept;
    bool Disconnect() noexcept;

    const SharedNodeData& GetData() const noexcept { return _data; }
    const IndexableName& GetKey() const noexcept { return _key; }
    std::string_view GetVarName() const noexcept { return std::string_view(_key); }
    std::string_view GetCaption() const noexcept { return _caption; }
    NodeDataType GetDataType() const noexcept { return _data ? _data->Type() : _type; }
    bool IsRequired() const noexcept { return _required; }
    std::size_t Index() const noexcept { return _index; }

    void SetData(SharedNodeData data, bool output = false);
    void SetCaption(std::string new_caption);

  private:
    std::shared_ptr<INodeData> _data;

    IndexableName _key   = IndexableName::None;
    std::string _caption = "";
    std::string _type    = "";
    bool _required       = false;
    bool _connected      = false;
    std::size_t _index   = 0;
};

using SharedPort = std::shared_ptr<Port>;

FLOW_NAMESPACE_END

template<>
struct std::less<FLOW_NAMESPACE::Port>
{
    std::uint64_t operator()(const FLOW_NAMESPACE::Port& lhs, const FLOW_NAMESPACE::Port& rhs) const
    {
        return lhs.Index() < rhs.Index();
    }
};

template<>
struct std::less<FLOW_NAMESPACE::SharedPort>
{
    std::uint64_t operator()(const FLOW_NAMESPACE::SharedPort& lhs, const FLOW_NAMESPACE::SharedPort& rhs) const
    {
        return lhs->Index() < rhs->Index();
    }
};
