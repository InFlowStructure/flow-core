// Copyright (c) 2024, Cisco Systems, Inc.
// All rights reserved.

#include "flow/core/Env.hpp"

#include "flow/core/Connection.hpp"
#include "flow/core/Node.hpp"
#include "flow/core/NodeData.hpp"
#include "flow/core/NodeFactory.hpp"
#include "flow/core/UUID.hpp"

#include <chrono>

FLOW_NAMESPACE_START

Env::Env(std::shared_ptr<NodeFactory> factory) : _factory{std::move(factory)}, _pool{std::make_unique<thread_pool>(10)}
{

    _factory->RegisterCompleteConversion<int, std::int8_t, std::int16_t, std::int32_t, std::int64_t, std::uint8_t,
                                         std::uint16_t, std::uint32_t, std::uint64_t, float, double, long double>();

    _factory->RegisterCompleteConversion<std::chrono::nanoseconds, std::chrono::microseconds, std::chrono::milliseconds,
                                         std::chrono::seconds, std::chrono::minutes, std::chrono::hours,
                                         std::chrono::days, std::chrono::months, std::chrono::years>();
}

void Env::Wait() { _pool->wait(); }

std::string Env::GetVar(const std::string& varname) const
{
    if (auto env_var = std::getenv(varname.c_str()))
    {
        return env_var;
    }

    return "";
}

FLOW_NAMESPACE_END
