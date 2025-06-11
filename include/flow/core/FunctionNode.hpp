#pragma once

#include "Env.hpp"
#include "Node.hpp"
#include "NodeFactory.hpp"

#include <nlohmann/json.hpp>

FLOW_NAMESPACE_BEGIN

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
                return GetEnv()->GetFactory()->template Convert<arg_t<Idx>>(
                    GetOutputData(IndexableName{input_names[Idx]}));
            }
            else
            {
                return GetEnv()->GetFactory()->template Convert<arg_t<Idx>>(
                    GetInputData(IndexableName{input_names[Idx]}));
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

template<concepts::Function F, F Func>
void NodeFactory::RegisterFunction(const std::string& category, const std::string& name)
{
    return RegisterNodeClass<FunctionNode<F, Func>>(category, name);
}

FLOW_NAMESPACE_END
