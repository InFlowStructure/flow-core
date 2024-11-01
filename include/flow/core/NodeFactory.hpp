// Copyright (c) 2024, Cisco Systems, Inc.
// All rights reserved.

#pragma once

#include "Concepts.hpp"
#include "Core.hpp"
#include "Log.hpp"
#include "Node.hpp"
#include "TypeConversion.hpp"
#include "TypeName.hpp"
#include "UUID.hpp"

#include <spdlog/spdlog.h>

#include <cstdlib>
#include <functional>
#include <memory>
#include <mutex>
#include <type_traits>
#include <unordered_map>
#include <vector>

FLOW_NAMESPACE_START

class Node;
class Env;

using CategoryMap = std::unordered_multimap<std::string, std::string>;

/**
 * @brief Factory for building nodes.
 *
 * @details Defines a factory type which, at its core, constructs nodes based on registered node types. It can also be
 *          inherited from to be a factory for creating visual representations of nodes.
 */
class NodeFactory
{
    using ConstructorCallback = std::function<void*(const UUID&, const std::string&, std::shared_ptr<Env>)>;

  public:
    virtual ~NodeFactory() = default;

    /**
     * @brief Registers a node's construction method by it's friendly name and a category.
     *
     * @tparam T The node type to register.
     * @param category The category under which the name will be registered.
     * @param name The friendly name of the node to register it under.
     */
    template<concepts::NodeType T>
    void RegisterNodeClass(const std::string& category, const std::string& name);

    /**
     * @brief Creates a node based on a registered classname.
     *
     * @param class_name The name of the class of node to construct. MUST be registered.
     * @param uuid The UUID for the new node.
     * @param name The friendly name of the node.
     * @param env Shared reference to the environment.
     *
     * @returns A newly constructed node of a registered node class.
     */
    SharedNode CreateNode(const std::string& class_name, const UUID& uuid, const std::string& name,
                          std::shared_ptr<Env> env);

    const CategoryMap& GetCategories() const;

    std::string GetFriendlyName(const std::string& class_name) const;

    /**
     * @brief Registers unidirectional conversions to several types.
     *
     * @tparam From The type to convert from.
     * @tparam To The first type to convert to.
     * @tparam Ts The remaining types to convert to.
     *
     * @param converter The conversion function to use.
     */
    template<typename From, typename To, typename... Ts>
    void RegisterUnidirectionalConversion(
        const TypeRegistry::ConversionFunc& converter = TypeRegistry::Convert<From, To>);

    /**
     * @brief Registers bidirectional conversions between several types.
     * @tparam From The type to convert from and back into.
     * @tparam To The first type to convert to and from.
     * @tparam Ts The remaining types to convert to and from.
     *
     * @param from_to_converter The conversion function to use From to To/Ts.
     * @param to_from_converter The conversion function to use to convert To/Ts to From.
     */
    template<typename From, typename To, typename... Ts>
    void RegisterBidirectionalConversion(
        const TypeRegistry::ConversionFunc& from_to_converter = TypeRegistry::Convert<From, To>,
        const TypeRegistry::ConversionFunc& to_from_converter = TypeRegistry::Convert<To, From>);

    /**
     * @brief Registers conversions between all given types.
     *
     * @tparam From The first type to convert.
     * @tparam To The second type to convert.
     * @tparam Ts The remaining types to convert.
     */
    template<typename From, typename To, typename... Ts>
    void RegisterCompleteConversion();

    /**
     * @brief Converts the given data to the specified typename.
     *
     * @param data The data to convert.
     * @param to_type The name of the type to convert to.
     *
     * @returns The converted data.
     */
    SharedNodeData Convert(const SharedNodeData& data, std::string_view to_type);

    /**
     * @brief Converts the given data to the specified type.
     *
     * @tparam To The type to convert to.
     * @param data The data to convert.
     *
     * @returns The converted data.
     */
    template<typename To>
    TSharedNodeData<To> Convert(const SharedNodeData& data);

    /**
     * @brief Check if given types have a registered conversion.
     *
     * @param from_type The type name to convert from.
     * @param to_type The type name to convert to.
     *
     * @returns true if \p from_type is convertible to \p to_type, false otherwise.
     */
    bool IsConvertible(std::string_view from_type, std::string_view to_type) const;

    /**
     * @brief Check if given types have a registered conversion.
     *
     * @tparam To The type to convert to.
     * @param from_type The type name to convert from.
     *
     * @returns true if \p from_type is convertible to \p To, false otherwise.
     */
    template<typename To>
    bool IsConvertible(std::string_view from_type) const;

    /**
     * @brief Check if given types have a registered conversion.
     *
     * @tparam From The type to convert from.
     * @tparam To The type to convert to.
     *
     * @returns true if \p From is convertible to \p To, false otherwise.
     */
    template<typename From, typename To>
    bool IsConvertible() const;

    /**
     * @brief Alias type for the entry point function signature for modules.
     */
    using RegisterModuleFunc = std::add_pointer_t<void FLOW_CORE_CALL(std::shared_ptr<NodeFactory>)>;

    /**
     * @brief The name of the entry point function for modules.
     */
    static constexpr const char* RegisterModuleFuncName = "RegisterModule";

  protected:
    template<concepts::NodeType T>
    static void* ConstructorHelper(const std::string& uuid_str, const std::string& name, std::shared_ptr<Env> env);

  private:
    template<typename>
    void RegisterCompleteConversion()
    {
    }

  private:
    std::unordered_map<std::string, ConstructorCallback> _constructor_map;
    CategoryMap _category_map;
    std::unordered_map<std::string, std::string> _friendly_names;
    TypeRegistry _conversion_registry;

    mutable std::mutex _mutex;
};

/**
 * @brief Organised way of registering nodes under related names.
 */
class Category
{
  public:
    Category()                = delete;
    Category(const Category&) = delete;
    Category(Category&&)      = delete;
    Category(std::shared_ptr<NodeFactory> factory, const std::string& name);
    Category(const Category& parent, const std::string& name);

    ~Category() = default;
    /**
     * @brief Get the factory used for registering nodes.
     * @returns The current node factory.
     */
    std::shared_ptr<NodeFactory> GetFactory() const noexcept { return _factory.lock(); }

    /**
     * @brief Registers a nodes construction method by it's friendly name and a category.
     *
     * @tparam T The node type to register.
     * @param name The friendly name of the node to register it under.
     */
    template<concepts::NodeType T>
    void RegisterNodeClass(const std::string& name) const
    {
        if (auto factory = _factory.lock())
        {
            factory->RegisterNodeClass<T>(_category_name, name);
        }
    }

  private:
    std::weak_ptr<NodeFactory> _factory;
    std::string _category_name;
};

template<concepts::NodeType T>
void NodeFactory::RegisterNodeClass(const std::string& category, const std::string& name)
{
    std::string_view class_name = TypeName_v<std::remove_cvref_t<T>>;

    _constructor_map.emplace(class_name, ConstructorHelper<std::remove_cvref_t<T>>);
    _category_map.emplace(category, class_name);
    _friendly_names.emplace(class_name, name);

    FLOW_TRACE("Registered {0}, {1}", class_name, category);
}

template<concepts::NodeType T>
void* NodeFactory::ConstructorHelper(const std::string& uuid_str, const std::string& name, std::shared_ptr<Env> env)
{
    return new T(uuid_str, name, std::move(env));
}

template<typename From, typename To, typename... Ts>
void NodeFactory::RegisterUnidirectionalConversion(const TypeRegistry::ConversionFunc& converter)
{
    _conversion_registry.RegisterUnidirectionalConversion<From, To>(converter);
    (_conversion_registry.RegisterUnidirectionalConversion<From, Ts>(converter), ...);
}

template<typename From, typename To, typename... Ts>
void NodeFactory::RegisterBidirectionalConversion(const TypeRegistry::ConversionFunc& from_to_converter,
                                                  const TypeRegistry::ConversionFunc& to_from_converter)
{
    _conversion_registry.RegisterBidirectionalConversion<From, To>(from_to_converter, to_from_converter);
    (_conversion_registry.RegisterBidirectionalConversion<From, Ts>(from_to_converter, to_from_converter), ...);
}

template<typename From, typename To, typename... Ts>
void NodeFactory::RegisterCompleteConversion()
{
    RegisterBidirectionalConversion<From, To, Ts...>();
    RegisterCompleteConversion<To, Ts...>();
}

template<typename T>
TSharedNodeData<T> NodeFactory::Convert(const SharedNodeData& data)
{
    return CastNodeData<T>(_conversion_registry.Convert(data, TypeName_v<T>));
}

template<typename To>
bool NodeFactory::IsConvertible(std::string_view from_type) const
{
    return _conversion_registry.IsConvertible(from_type, TypeName_v<To>);
}

template<typename From, typename To>
bool NodeFactory::IsConvertible() const
{
    return _conversion_registry.IsConvertible(TypeName_v<From>, TypeName_v<To>);
}

FLOW_NAMESPACE_END

#define REGISTER_NODES(Extension)                                                                                      \
    extern "C"                                                                                                         \
    {                                                                                                                  \
        namespace Extension                                                                                            \
        {                                                                                                              \
        void RegisterModule(std::shared_ptr<NodeFactory>);                                                             \
        }                                                                                                              \
    }
