// Copyright (c) 2024, Cisco Systems, Inc.
// All rights reserved.

#include "flow/core/Port.hpp"

FLOW_NAMESPACE_BEGIN

Port::Port(const IndexableName& key, const std::string& caption, std::string_view type, SharedNodeData data,
           bool required, std::size_t index)
    : _data{std::move(data)}, _key{key}, _caption{caption}, _type{type}, _required{required}, _index{index}
{
}

bool Port::Connect() noexcept
{
    if (IsConnected()) return false;

    _connected = true;
    return true;
}

bool Port::Disconnect() noexcept
{
    if (!IsConnected()) return false;

    _connected = false;
    return true;
}

void Port::SetData(SharedNodeData data, bool output)
{
    if (!data && IsRequired())
    {
        return;
    }

    if (!_data || !data || output)
    {
        _data = std::move(data);
    }
    else
    {
        _data->FromPointer(data->AsPointer());
    }

    if (OnSetData)
    {
        OnSetData(_key, _data, output);
    }
}

void Port::SetCaption(std::string new_caption) { _caption = std::move(new_caption); }

FLOW_NAMESPACE_END
