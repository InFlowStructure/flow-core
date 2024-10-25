// Copyright (c) 2024, Cisco Systems, Inc.
// All rights reserved.

#include "TypeConversion.hpp"

FLOW_NAMESPACE_START

SharedNodeData TypeRegistry::Convert(const SharedNodeData& data, std::string_view to_type) const
{
    if (!data) return nullptr;

    if (data->Type() == to_type || to_type == TypeName_v<std::any>)
    {
        return data;
    }

    if (!_conversions.contains(data->Type()))
    {
        return data;
    }

    const auto& from_conversions = _conversions.at(data->Type());
    if (!from_conversions.contains(to_type))
    {
        return data;
    }

    if (!from_conversions.at(to_type))
    {
        throw std::runtime_error("could not convert " + std::string{to_type} + " to " + std::string{data->Type()});
    }

    return from_conversions.at(to_type)(data);
}

bool TypeRegistry::IsConvertible(std::string_view from_type, std::string_view to_type) const
{
    if (from_type == to_type || to_type == TypeName_v<std::any>)
    {
        return true;
    }

    if (!_conversions.contains(from_type))
    {
        return false;
    }

    return _conversions.at(from_type).contains(to_type);
}

FLOW_NAMESPACE_END
