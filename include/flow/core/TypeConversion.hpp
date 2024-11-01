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

/**
 * @brief Registry for all known types.
 *
 * @details Holds a registry of known types and their associated conversions. Types that are not registered still
 *          function, but will not be convertible to other types.
 */
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
    /**
     * @brief Type alias for conversion function type.
     */
    using ConversionFunc = std::function<SharedNodeData(const SharedNodeData& data)>;

    /**
     * @brief Register a unidirectional conversion.
     *
     * @tparam From The type to convert from.
     * @tparam To The type to convert to.
     *
     * @param converter The conversion function for converting From to To.
     */
    template<typename From, typename To>
    void RegisterUnidirectionalConversion(const ConversionFunc& converter = Convert<From, To>);

    /**
     * @brief Register a bidirectional conversion.
     *
     * @details Registers two conversions; 1) T to U, and 2) U to T.
     *
     * @tparam T The first type.
     * @tparam U The second type.
     *
     * @param from_to_converter Conversion function for T to U.
     * @param to_from_converter Conversion function for U to T.
     */
    template<typename T, typename U>
    void RegisterBidirectionalConversion(const ConversionFunc& from_to_converter = Convert<T, U>,
                                         const ConversionFunc& to_from_converter = Convert<U, T>);

    /**
     * @brief Default conversion method that is used when converting.
     *
     * @param data The data to convert.
     * @param to_type The type to convert the data to.
     *
     * @returns The converted data.
     */
    SharedNodeData Convert(const SharedNodeData& data, std::string_view to_type) const;

    /**
     * @brief Check if given types have a registered conversion.
     *
     * @param from_type The type name to convert from.
     * @param to_type The type name to convert to.
     *
     * @returns true if \p from_type is convertible to \p to_type, false otherwise.
     */
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
