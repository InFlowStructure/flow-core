// Copyright (c) 2024, Cisco Systems, Inc.
// All rights reserved.

#pragma once

#include "Concepts.hpp"
#include "Core.hpp"
#include "Node.hpp"
#include "TypeConversion.hpp"
#include "TypeName.hpp"
#include "UUID.hpp"

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
    void RegisterNodeClass(const std::string& category, const std::string& name = std::string{TypeName_v<T>});

    /**
     * @brief Removes a node's construction method from the factory.
     * @tparam T The node type to unregister.
     */
    template<concepts::NodeType T>
    void UnregisterNodeClass(const std::string& category);

    template<concepts::Function F>
    void RegisterFunction(std::add_pointer_t<F> func, const std::string& category,
                          const std::string& name = std::string{TypeName_v<T>});

    /**
     * @brief Removes all nodes added by the given category object.
     * @param category The category object to cleanup.
     */
    void UnregisterCategory(const class Category& category);

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
    using ModuleMethod_t = std::add_pointer_t<void FLOW_CORE_CALL(std::shared_ptr<NodeFactory>)>;

    /**
     * @brief The name of the entry point function for modules.
     */
    static constexpr const char* RegisterModuleFuncName = "RegisterModule";

    /**
     * @brief The name of the exit point function for modules.
     */
    static constexpr const char* UnregisterModuleFuncName = "UnregisterModule";

  protected:
    template<concepts::NodeType T>
    static void* ConstructorHelper(const std::string& uuid_str, const std::string& name, std::shared_ptr<Env> env);

  private:
    void UnregisterNodeClass(const std::string& category, const std::string& class_name);

    template<typename>
    void RegisterCompleteConversion()
    {
    }

    template<typename>
    void RegisterUnidirectionalConversion()
    {
    }

    template<typename>
    void RegisterBidirectionalConversion()
    {
    }

  public:
    /**
     * @brief Event dispatcher that runs every time a new node class is registered.
     */
    EventDispatcher<std::string_view> OnNodeClassRegistered;

    /**
     * @brief Event dispatcher that runs every time a registered node class is unregistered.
     */
    EventDispatcher<std::string_view> OnNodeClassUnregistered;

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
    Category(const std::string& name);
    Category(const Category& parent, const std::string& name);
    ~Category() = default;

    /**
     * @brief Registers a node's construction method by it's friendly name and a category.
     *
     * @tparam T The node type to register.
     * @param factory THe factory to register to.
     * @param name The friendly name of the node to register it under.
     */
    template<concepts::NodeType T>
    void RegisterNodeClass(const std::shared_ptr<NodeFactory>& factory,
                           const std::string& friendly_name = std::string{TypeName_v<T>})
    {
        factory->RegisterNodeClass<T>(_category_name, friendly_name);
        _classes.insert(std::make_pair(std::string{TypeName_v<T>}, friendly_name));
    }

    /**
     * @brief Unregisters a node's construction method.
     * @tparam T The Node type to unregiter.
     * @param factory THe factory to unregister from.
     */
    template<concepts::NodeType T>
    void UnregisterNodeClass(const std::shared_ptr<NodeFactory>& factory) const
    {
        factory->UnregisterNodeClass<T>(_category_name);

        const std::string class_name{TypeName_v<T>};
        std::erase_if(_classes, [&](const auto& c) { return c == class_name; });
    }

  private:
    std::weak_ptr<NodeFactory> _factory;
    std::string _category_name;
    std::map<std::string, std::string> _classes;

    friend NodeFactory;
};

template<concepts::NodeType T>
void NodeFactory::RegisterNodeClass(const std::string& category, const std::string& name)
{
    const std::string_view class_name = TypeName_v<std::remove_cvref_t<T>>;

    _constructor_map.emplace(class_name, ConstructorHelper<std::remove_cvref_t<T>>);
    _category_map.emplace(category, class_name);
    _friendly_names.emplace(class_name, name);

    OnNodeClassRegistered.Broadcast(std::string_view{class_name});
}

template<concepts::NodeType T>
void NodeFactory::UnregisterNodeClass(const std::string& category)
{
    const std::string class_name{TypeName_v<std::remove_cvref_t<T>>};
    return UnregisterNodeClass(category, class_name);
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
    RegisterUnidirectionalConversion<From, Ts...>();
}

template<typename From, typename To, typename... Ts>
void NodeFactory::RegisterBidirectionalConversion(const TypeRegistry::ConversionFunc& from_to_converter,
                                                  const TypeRegistry::ConversionFunc& to_from_converter)
{
    _conversion_registry.RegisterBidirectionalConversion<From, To>(from_to_converter, to_from_converter);
    RegisterBidirectionalConversion<From, Ts...>();
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

/**
 * @brief Structure that reveals the types of the return value and arguments of a function at compile-time.
 * @tparam F The function type being analyzed.
 */
template<typename F>
struct FunctionTraits;

template<typename R, typename... Args>
struct FunctionTraits<R(Args...)>
{
    using ReturnType = R;
    using ArgTypes   = std::tuple<Args...>;
};

/**
 * @brief Node class that wraps a declared function.
 *
 * @details Wraps a declared function as a node type with inputs labeled in increasing alphabetical order starting at
 *          'a'. Requires that the function node be overloaded. If the function is overloaded, then supplying the
 *          specific overload to wrap is required.
 *
 * @tparam F
 * @tparam Func
 */
template<typename F, std::add_pointer_t<std::remove_pointer_t<F>> Func>
class FunctionNode : public Node
{
    template<typename Tuple>
    struct decayed_tuple;

    template<typename... Types>
    struct decayed_tuple<std::tuple<Types...>>
    {
        using type = std::tuple<std::conditional_t<std::is_reference_v<Types>, std::decay_t<Types>, std::byte>...>;
    };

    template<typename Tuple>
    using decayed_tuple_t = typename decayed_tuple<Tuple>::type;

  protected:
    using traits   = FunctionTraits<std::remove_pointer_t<F>>;
    using output_t = typename traits::ReturnType;
    using arg_ts   = typename traits::ArgTypes;

    template<std::size_t Idx>
    using arg_t = typename std::tuple_element_t<Idx, arg_ts>;

    static constexpr const char* return_output_name = "return";

  private:
    template<int... Idx>
    void ParseArguments(std::integer_sequence<int, Idx...>)
    {
        (
            [&, this] {
                if constexpr (std::is_lvalue_reference_v<arg_t<Idx>> &&
                              !std::is_const_v<std::remove_reference_t<arg_t<Idx>>>)
                {
                    AddOutput<arg_t<Idx>>({input_names[Idx] = 'a' + Idx}, "",
                                          MakeRefNodeData<arg_t<Idx>>(std::get<Idx>(_arguments)));
                }
                else
                {
                    AddInput<arg_t<Idx>>({input_names[Idx] = 'a' + Idx}, "");
                }
            }(),
            ...);
    }

    template<int... Idx>
    auto GetInputs(std::integer_sequence<int, Idx...>)
    {
        return std::make_tuple([&, this] {
            if constexpr (std::is_lvalue_reference_v<arg_t<Idx>> &&
                          !std::is_const_v<std::remove_reference_t<arg_t<Idx>>>)
            {
                return GetEnv()->GetFactory()->Convert<arg_t<Idx>>(GetOutputData(IndexableName{input_names[Idx]}));
            }
            else
            {
                return GetEnv()->GetFactory()->Convert<arg_t<Idx>>(GetInputData(IndexableName{input_names[Idx]}));
            }
        }()...);
    }

    template<int... Idx>
    json SaveInputs(std::integer_sequence<int, Idx...>) const
    {
        json inputs_json;

        const auto& inputs = GetInputPorts();
        (
            [&, this] {
                const auto& key = input_names[Idx];
                if (!inputs.contains(IndexableName{key}))
                {
                    return;
                }

                if (auto x = GetInputData<arg_t<Idx>>(IndexableName{key}))
                {
                    inputs_json[key] = x->Get();
                }
            }(),
            ...);

        return inputs_json;
    }

    template<int... Idx>
    void RestoreInputs(json& j, std::integer_sequence<int, Idx...>)
    {
        (
            [&, this] {
                const auto& key = input_names[Idx];
                if (!j.contains(key))
                {
                    return;
                }

                if constexpr (!std::is_lvalue_reference_v<arg_t<Idx>>)
                {
                    SetInputData(IndexableName{key}, MakeNodeData<arg_t<Idx>>(j[key]), false);
                }
            }(),
            ...);
    }

  public:
    explicit FunctionNode(const UUID& uuid, const std::string& name, std::shared_ptr<Env> env)
        : Node(uuid, TypeName_v<FunctionNode<F, Func>>, name, std::move(env)), _func{Func}
    {
        ParseArguments(std::make_integer_sequence<int, std::tuple_size_v<arg_ts>>{});

        if (!std::is_void_v<output_t>)
        {
            AddOutput<output_t>(return_output_name, return_output_name);
        }
    }

    virtual ~FunctionNode() = default;

  protected:
    void Compute() override
    {
        auto inputs = GetInputs(std::make_integer_sequence<int, std::tuple_size_v<arg_ts>>{});

        if (std::apply([](auto&&... args) { return (!args || ...); }, inputs))
        {
            return;
        }

        if constexpr (std::is_void_v<output_t>)
        {
            std::apply([&](auto&&... args) { return _func(args->Get()...); }, inputs);
        }
        else
        {
            auto result = std::apply([&](auto&&... args) { return _func(args->Get()...); }, inputs);
            this->SetOutputData(return_output_name, MakeNodeData(std::move(result)), false);
        }

        const auto& outputs = GetOutputPorts();
        for (const auto& [key, port] : outputs)
        {
            OnSetOutput.Broadcast(key, port->GetData());
            EmitUpdate(key, port->GetData());
        }
    }

    json SaveInputs() const override
    {
        return SaveInputs(std::make_integer_sequence<int, std::tuple_size_v<arg_ts>>{});
    }

    void RestoreInputs(const json& j) override
    {
        RestoreInputs(const_cast<json&>(j), std::make_integer_sequence<int, std::tuple_size_v<arg_ts>>{});
    }

  private:
    std::add_pointer_t<std::remove_pointer_t<F>> _func;
    static inline std::array<std::string, std::tuple_size_v<arg_ts>> input_names{""};
    decayed_tuple_t<arg_ts> _arguments;
};

#define DECLARE_FUNCTION_NODE_TYPE(func) FunctionNode<decltype(func), func>

FLOW_NAMESPACE_END
