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
    using ConstructorCallback = std::function<void*(const std::string&, const UUID&, std::shared_ptr<Env>)>;

  public:
    virtual ~NodeFactory() = default;

    template<concepts::NodeType T>
    [[deprecated]] void RegisterClass(const std::string& category);

    template<concepts::NodeType T>
    void RegisterClass(const std::string& category, const std::string& name);

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

    template<typename From, typename To, typename... Ts>
    void RegisterUnidirectionalConversion(
        const TypeRegistry::ConversionFunc& converter = TypeRegistry::Convert<From, To>);

    template<typename From, typename To, typename... Ts>
    void RegisterBidirectionalConversion(
        const TypeRegistry::ConversionFunc& from_to_converter = TypeRegistry::Convert<From, To>,
        const TypeRegistry::ConversionFunc& to_from_converter = TypeRegistry::Convert<To, From>);

    template<typename From, typename To, typename... Ts>
    void RegisterCompleteConversion();

    SharedNodeData Convert(const SharedNodeData& data, std::string_view to_type);

    template<typename To>
    TSharedNodeData<To> Convert(const SharedNodeData& data);

    bool IsConvertible(std::string_view from_type, std::string_view to_type) const;

    template<typename To>
    bool IsConvertible(std::string_view from_type) const;

    template<typename From, typename To>
    bool IsConvertible() const;

    using RegisterModuleFunc = std::add_pointer_t<void FLOW_CORE_CALL(std::shared_ptr<NodeFactory>)>;
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

class Category
{
  public:
    Category()                = delete;
    Category(const Category&) = delete;
    Category(Category&&)      = delete;
    Category(std::shared_ptr<NodeFactory> factory, const std::string& name);
    Category(const Category& parent, const std::string& name);

    ~Category() = default;

    std::shared_ptr<NodeFactory> GetFactory() const noexcept { return _factory.lock(); }

    template<concepts::NodeType T>
    void RegisterClass(const std::string& name) const
    {
        if (auto factory = _factory.lock())
        {
            factory->RegisterClass<T>(_category_name, name);
        }
    }

  private:
    std::weak_ptr<NodeFactory> _factory;
    std::string _category_name;
};

template<concepts::NodeType T>
void NodeFactory::RegisterClass(const std::string& category)
{
    std::string_view class_name = TypeName_v<std::remove_cvref_t<T>>;

    _constructor_map.emplace(class_name, ConstructorHelper<std::remove_cvref_t<T>>);
    _category_map.emplace(category, class_name);

    FLOW_TRACE("Registered {0}, {1}", class_name, category);
}

template<concepts::NodeType T>
void NodeFactory::RegisterClass(const std::string& category, const std::string& name)
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
