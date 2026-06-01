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

    // In-place value update preserves _data identity (downstream weak_ptrs
    // and observers keep their handle), but is only valid when the incoming
    // data is the SAME concrete type as the existing storage.  FromPointer
    // does `*reinterpret_cast<T*>(other_ptr)` (NodeData.hpp), which silently
    // reads the wrong number of bytes when widths differ — e.g. writing a
    // NodeData<double> to a NodeData<float> port reads only the low 4 bytes
    // of the double, so 440.0 (0x407B800000000000) becomes +0.0f.  On a
    // type mismatch, replace the SharedNodeData instead.  Identity is lost
    // for that single transition, but values are preserved correctly.
    if (!_data || !data || output || _data->Type() != data->Type())
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
