// Copyright (c) 2024, Cisco Systems, Inc.
// All rights reserved.

#pragma once

#include "Core.hpp"
#include "NodeData.hpp"
#include "TypeName.hpp"

#include <functional>
#include <set>
#include <string>
#include <unordered_map>

FLOW_NAMESPACE_START

class TypeRegistry
{
    template<typename T>
    using TypeMap = std::unordered_map<std::string_view, T>;

    template<typename From, typename To>
    static SharedNodeData Convert(const SharedNodeData& data)
    {
        if (auto from_data = CastNodeData<From>(data))
        {
            return MakeNodeData<To>(*from_data);
        }
        return data;
    }
    friend class NodeFactory;

  public:
    using ConversionFunc = std::function<SharedNodeData(const SharedNodeData& data)>;

    template<typename From, typename To>
    void RegisterUnidirectionalConversion(const ConversionFunc& converter = Convert<From, To>);

    template<typename From, typename To>
    void RegisterBidirectionalConversion(const ConversionFunc& from_to_converter = Convert<From, To>,
                                         const ConversionFunc& to_from_converter = Convert<To, From>);

    SharedNodeData Convert(const SharedNodeData& data, std::string_view to_type) const;

    bool IsConvertible(std::string_view from_type, std::string_view to_type) const;

  private:
    TypeMap<TypeMap<ConversionFunc>> _conversions;
};

template<typename From, typename To>
void TypeRegistry::RegisterUnidirectionalConversion(const ConversionFunc& converter)
{
    _conversions[TypeName_v<From>].emplace(TypeName_v<To>, converter);
}

template<typename From, typename To>
void TypeRegistry::RegisterBidirectionalConversion(const ConversionFunc& from_to_converter,
                                                   const ConversionFunc& to_from_converter)
{
    _conversions[TypeName_v<From>].emplace(TypeName_v<To>, from_to_converter);
    _conversions[TypeName_v<To>].emplace(TypeName_v<From>, to_from_converter);
}

FLOW_NAMESPACE_END
