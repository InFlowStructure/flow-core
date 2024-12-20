// Copyright (c) 2024, Cisco Systems, Inc.
// All rights reserved.

#include "Env.hpp"

#include "Connection.hpp"
#include "Node.hpp"
#include "NodeData.hpp"
#include "NodeFactory.hpp"
#include "UUID.hpp"

#include <chrono>
#include <fstream>

#ifdef FLOW_WINDOWS
#include <windows.h>
#else
#include <dlfcn.h>
#endif

FLOW_NAMESPACE_START

Env::Env(std::shared_ptr<NodeFactory> factory) : _factory{std::move(factory)}, _pool{std::make_unique<thread_pool>(10)}
{

    _factory->RegisterCompleteConversion<int, std::int8_t, std::int16_t, std::int32_t, std::int64_t, std::uint8_t,
                                         std::uint16_t, std::uint32_t, std::uint64_t, float, double, long double>();

    _factory->RegisterCompleteConversion<std::chrono::nanoseconds, std::chrono::microseconds, std::chrono::milliseconds,
                                         std::chrono::seconds, std::chrono::minutes, std::chrono::hours,
                                         std::chrono::days, std::chrono::months, std::chrono::years>();
}

void Env::LoadModule(const std::filesystem::path& file)
{
    if (_loaded_modules.contains(file.filename().string()))
    {
        UnloadModule(file.filename().string());
        _loaded_modules.erase(file.filename().string());
    }

#ifdef FLOW_WINDOWS
    HINSTANCE handle = LoadLibrary(file.string().c_str());
    if (!handle)
    {
        throw std::runtime_error("Error loading file: " + file.string());
    }

    auto register_func = GetProcAddress(handle, NodeFactory::RegisterModuleFuncName);
#else
    void* handle = dlopen(file.c_str(), RTLD_LAZY);
    if (!handle)
    {
        throw std::runtime_error("Error loading file: " + std::string(dlerror()));
        return;
    }

    auto register_func = dlsym(handle, NodeFactory::RegisterModuleFuncName);
#endif
    if (auto RegisterModule_func = std::bit_cast<NodeFactory::ModuleMethod_t>(register_func))
    {
        RegisterModule_func(_factory);
        _loaded_modules.emplace(file.filename().string(), std::bit_cast<void*>(handle));
        return;
    }

#ifdef FLOW_WINDOWS
    FreeLibrary(handle);
#else
    dlclose(handle);
#endif
    throw std::runtime_error("Error loading symbol for RegisterModule from " + file.filename().string());
}

void Env::LoadModules(const std::filesystem::path& extension_path)
{
    if (!std::filesystem::exists(extension_path)) return;

    for (const auto& file : std::filesystem::directory_iterator(extension_path))
    {
        LoadModule(file);
    }
}

void Env::UnloadModule(const std::filesystem::path& module_file)
{
    const auto& handle = _loaded_modules.at(module_file.filename().string());

#ifdef FLOW_WINDOWS
    auto unregister_func = GetProcAddress(std::bit_cast<HINSTANCE>(handle), NodeFactory::UnregisterModuleFuncName);
#else
    auto unregister_func = dlsym(handle, NodeFactory::UnregisterModuleFuncName);
#endif
    if (auto UnregisterModule_func = std::bit_cast<NodeFactory::ModuleMethod_t>(unregister_func))
    {
        UnregisterModule_func(_factory);
        _loaded_modules.emplace(module_file.filename().string(), std::bit_cast<void*>(handle));
        return;
    }

#ifdef FLOW_WINDOWS
    FreeLibrary(std::bit_cast<HINSTANCE>(handle));
#else
    dlclose(handle);
#endif
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
