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

FLOW_NAMESPACE_BEGIN

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

    /**
     * @brief Default conversion implementation between types.
     *
     * @tparam From The source type to convert from
     * @tparam To The destination type to convert to
     * @param data The shared node data to convert
     *
     * @returns Converted data if successful, original data otherwise
     */
    template<typename From, typename To>
    static SharedNodeData Convert(const SharedNodeData& data)
    {
        if (auto from_data = CastNodeData<From>(data))
        {
            if constexpr (std::is_rvalue_reference_v<To>)
            {
                return MakeNodeData<To, From>(std::move(*from_data));
            }
            else
            {
                return MakeNodeData<To, From>(*from_data);
            }
        }
        return data;
    }

    /// Allow NodeFactory to access protected members
    friend class NodeFactory;

  public:
    /// Function type for implementing custom type conversions
    using ConversionFunc = std::function<SharedNodeData(const SharedNodeData& data)>;

    /**
     * @brief Register a one-way conversion between types.
     *
     * @details When From and To are the same type, automatically registers additional
     *          conversions for different reference and const qualifiers:
     *          - lvalue references
     *          - const lvalue references
     *          - rvalue references
     *
     * @tparam From Source type to convert from
     * @tparam To Destination type to convert to
     *
     * @param converter Custom conversion function, defaults to built-in Convert
     */
    template<typename From, typename To>
    void RegisterUnidirectionalConversion(const ConversionFunc& converter = Convert<From, To>);

    /**
     * @brief Register conversions in both directions between two types.
     *
     * @details Registers both T->U and U->T conversions. For same-type conversions,
     *          automatically handles reference and const variations through
     *          RegisterUnidirectionalConversion.
     *
     * @tparam T First type in the conversion pair
     * @tparam U Second type in the conversion pair
     *
     * @param from_to_converter Conversion function from T to U
     * @param to_from_converter Conversion function from U to T
     */
    template<typename T, typename U>
    void RegisterBidirectionalConversion(const ConversionFunc& from_to_converter = Convert<T, U>,
                                         const ConversionFunc& to_from_converter = Convert<U, T>);

    /**
     * @brief Convert data to a different type at runtime.
     *
     * @details Attempts to convert the given data using registered conversions.
     *          Returns original data if:
     *          - Input is null
     *          - Types are the same
     *          - Target type is std::any
     *          - No conversion exists
     *
     * @param data The data to convert
     * @param to_type Target type name to convert to
     *
     * @returns Converted data if successful, original data otherwise
     * @throws std::runtime_error if conversion exists but fails
     */
    SharedNodeData Convert(const SharedNodeData& data, std::string_view to_type) const;

    /**
     * @brief Check if conversion exists between two types.
     *
     * @details Returns true if either:
     *          - Types are the same
     *          - Target type is std::any
     *          - A registered conversion exists
     *
     * @param from_type Source type name
     * @param to_type Target type name
     *
     * @returns true if conversion is possible, false otherwise
     */
    bool IsConvertible(std::string_view from_type, std::string_view to_type) const;

  protected:
    /**
     * @brief Internal helper to register a type conversion.
     *
     * @tparam From Source type
     * @tparam To Target type
     *
     * @param converter The conversion function to register
     */
    template<typename From, typename To>
    void RegisterConversion(const ConversionFunc& converter = Convert<From, To>);

  private:
    /// Storage for registered type conversion functions
    TypeMap<TypeMap<ConversionFunc>> _conversions;
};

template<typename From, typename To>
void TypeRegistry::RegisterConversion(const ConversionFunc& converter)
{
    _conversions[TypeName_v<From>].emplace(TypeName_v<To>, converter);
}

template<typename From, typename To>
void TypeRegistry::RegisterUnidirectionalConversion(const ConversionFunc& converter)
{
    RegisterConversion<std::decay_t<From>, std::decay_t<To>>(converter);

    if constexpr (std::is_same_v<std::decay_t<From>, std::decay_t<To>>)
    {
        RegisterConversion<std::decay_t<From>, std::add_lvalue_reference_t<std::decay_t<To>>>();
        RegisterConversion<std::decay_t<From>, std::add_lvalue_reference_t<std::add_const_t<std::decay_t<To>>>>();
        RegisterConversion<std::decay_t<From>, std::add_rvalue_reference_t<std::decay_t<To>>>();
        RegisterConversion<std::add_const_t<std::decay_t<From>>,
                           std::add_lvalue_reference_t<std::add_const_t<std::decay_t<To>>>>();
        RegisterConversion<std::add_lvalue_reference_t<std::decay_t<From>>, std::decay_t<To>>();
        RegisterConversion<std::add_lvalue_reference_t<std::decay_t<From>>,
                           std::add_lvalue_reference_t<std::add_const_t<std::decay_t<To>>>>();
        RegisterConversion<std::add_lvalue_reference_t<std::add_const_t<std::decay_t<From>>>, std::decay_t<To>>();
        RegisterConversion<std::add_rvalue_reference_t<std::decay_t<From>>, std::decay_t<To>>();
    }
}

template<typename From, typename To>
void TypeRegistry::RegisterBidirectionalConversion(const ConversionFunc& from_to_converter,
                                                   const ConversionFunc& to_from_converter)
{
    RegisterUnidirectionalConversion<From, To>(from_to_converter);
    RegisterUnidirectionalConversion<To, From>(to_from_converter);
}

FLOW_NAMESPACE_END
