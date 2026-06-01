// Copyright (c) 2024, Cisco Systems, Inc.
// All rights reserved.

#pragma once

#include "Env.hpp"
#include "Node.hpp"
#include "NodeFactory.hpp"

#include <nlohmann/json.hpp>

#include <vector>

FLOW_NAMESPACE_BEGIN

using json = nlohmann::json;

template<typename T>
concept JsonSerialisable = requires(T t) {
    {
        json(t)
    };
};

/**
 * @brief Extract function signature information at compile-time
 *
 * @tparam F Function type to analyze
 */
template<class F>
struct FunctionTraits;

/**
 * @brief Specialization for regular function signatures
 *
 * @tparam R Return type
 * @tparam Args Argument types
 */
template<typename R, typename... Args>
struct FunctionTraits<R(Args...)>
{
    using ReturnType                       = R;
    using ArgTypes                         = std::tuple<Args...>;
    static constexpr std::size_t arg_count = sizeof...(Args);
};

template<typename R, typename C, typename... Args>
struct FunctionTraits<R (C::*)(Args...)> : FunctionTraits<R(Args...)>
{
};

template<typename R, typename C, typename... Args>
struct FunctionTraits<R (C::*)(Args...) const> : FunctionTraits<R(Args...)>
{
};

template<class F>
struct FunctionTraits : FunctionTraits<decltype(&F::operator())>
{
};

template<typename R, typename... Args>
struct FunctionTraits<std::function<R(Args...)>> : FunctionTraits<R(Args...)>
{
};

/**
 * @brief Node that wraps a function into the graph system
 *
 * @details Automatically creates input/output ports based on function signature:
 *          - Input ports for each function parameter
 *          - Output port named "return" for the return value
 *          - Reference parameters become output ports
 *
 * @tparam Func Pointer to concrete function implementation
 */
template<auto Func>
class FunctionNode : public Node
{
    /// Helper to decay tuple types while preserving references
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
    using F        = decltype(Func);
    using traits   = FunctionTraits<std::remove_pointer_t<std::decay_t<F>>>;
    using output_t = typename traits::ReturnType;
    using arg_ts   = typename traits::ArgTypes;

    template<std::size_t Idx>
    using arg_t = typename std::tuple_element_t<Idx, arg_ts>;

    /// Name of the return value output port
    static constexpr const char* return_output_name = "return";

  private:
    template<int... Idx>
    void ParseArguments(std::integer_sequence<int, Idx...>, std::vector<std::string> arg_names)
    {
        if (!arg_names.empty())
        {
            if (arg_names.size() != sizeof...(Idx))
            {
                throw std::invalid_argument("list of argument names must match the number of arguments");
            }

            std::copy(arg_names.begin(), arg_names.end(), _arg_names.begin());
        }
        else
        {
            ((void)(_arg_names[Idx] = 'a' + Idx), ...);
        }

        (
            [&, this] {
                auto& arg_name = _arg_names[Idx];
                if constexpr (std::is_lvalue_reference_v<arg_t<Idx>> &&
                              !std::is_const_v<std::remove_reference_t<arg_t<Idx>>>)
                {
                    AddOutput<arg_t<Idx>>({arg_name}, "", MakeRefNodeData<arg_t<Idx>>(std::get<Idx>(_arguments)));
                }
                else
                {
                    AddInput<arg_t<Idx>>({arg_name}, "");
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
                return GetEnv()->GetFactory()->template Convert<arg_t<Idx>>(
                    GetOutputData(IndexableName{_arg_names[Idx]}));
            }
            else
            {
                return GetEnv()->GetFactory()->template Convert<arg_t<Idx>>(
                    GetInputData(IndexableName{_arg_names[Idx]}));
            }
        }()...);
    }

    template<int... Idx>
    json SaveInputs(std::integer_sequence<int, Idx...>) const
    {
        json inputs_json = json::object();

        const auto& inputs = GetInputPorts();
        (
            [&, this] {
                if constexpr (!JsonSerialisable<arg_t<Idx>>)
                {
                    return;
                }
                else
                {
                    const auto& key = _arg_names[Idx];
                    if (!inputs.contains(IndexableName{key}))
                    {
                        return;
                    }

                    if (auto x = GetInputData<arg_t<Idx>>(IndexableName{key}))
                    {
                        inputs_json[key] = x->Get();
                    }
                    else if (auto y = GetInputData<std::decay_t<arg_t<Idx>>>(IndexableName{key}))
                    {
                        inputs_json[key] = y->Get();
                    }
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
                const auto& key = _arg_names[Idx];
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
    explicit FunctionNode(const UUID& uuid, const std::string& name, std::shared_ptr<Env> env,
                          std::vector<std::string> arg_names = {})
        : Node(uuid, TypeName_v<FunctionNode<Func>>, name, std::move(env)), _func{Func}
    {
        ParseArguments(std::make_integer_sequence<int, std::tuple_size_v<arg_ts>>{}, arg_names);

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
            this->SetOutputData(return_output_name, MakeNodeData<decltype(result)&&>(std::move(result)), false);
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
    std::decay_t<F> _func;
    static inline std::array<std::string, std::tuple_size_v<arg_ts>> _arg_names{""};
    decayed_tuple_t<arg_ts> _arguments;
};

template<auto Func>
void NodeFactory::RegisterFunction(const std::string& category, const std::string& name,
                                   std::vector<std::string> arg_names)
{
    constexpr std::string_view class_name = TypeName_v<FunctionNode<Func>>;

    _constructor_map.emplace(
        class_name,
        [names = std::move(arg_names)](const std::string& uuid_str, const std::string& name, std::shared_ptr<Env> env) {
            return new FunctionNode<Func>(uuid_str, name, std::move(env), std::move(names));
        });
    _category_map.emplace(category, class_name);
    _friendly_names.emplace(class_name, name);

    OnNodeClassRegistered.Broadcast(std::string_view{class_name});
}

template<auto Func>
SharedNode NodeFactory::CreateFunctionNode(const UUID& uuid, const std::string& name, std::shared_ptr<Env> env)
{
    return NodeFactory::CreateNode<FunctionNode<Func>>(uuid, name, env);
}

FLOW_NAMESPACE_END
