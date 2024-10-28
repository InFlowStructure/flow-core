// Copyright (c) 2024, Cisco Systems, Inc.
// All rights reserved.

#include "Env.hpp"

#include "Connection.hpp"
#include "Log.hpp"
#include "Node.hpp"
#include "NodeData.hpp"
#include "NodeFactory.hpp"
#include "UUID.hpp"

#include <spdlog/spdlog.h>

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
try
{
    if (_loaded_modules.contains(file.filename().string()))
    {
        FLOW_WARN("Module {0} already loaded, reloading...", file.filename().string());
        UnloadModule(file.filename().string());
        _loaded_modules.erase(file.filename().string());
    }

    const auto new_module_file = std::filesystem::current_path() / "modules" / file.filename();
    if (new_module_file != file)
    {
        std::filesystem::copy_file(file, new_module_file, std::filesystem::copy_options::overwrite_existing);
    }

#ifdef FLOW_WINDOWS
    HINSTANCE handle = LoadLibrary(new_module_file.string().c_str());
    if (!handle)
    {
        FLOW_ERROR("Error loading file: {0}", new_module_file.string().c_str());
        return;
    }

    auto register_func = GetProcAddress(handle, NodeFactory::RegisterModuleFuncName);
#else
    void* handle = dlopen(new_module_file.c_str(), RTLD_LAZY);
    if (!handle)
    {
        FLOW_ERROR("Error loading file: {0}", dlerror());
        return;
    }

    auto register_func = dlsym(handle, NodeFactory::RegisterModuleFuncName);
#endif
    if (auto RegisterModule_func = std::bit_cast<NodeFactory::RegisterModuleFunc>(register_func))
    {
        RegisterModule_func(_factory);

        _loaded_modules.emplace(new_module_file.filename().string(), std::bit_cast<void*>(handle));

        FLOW_INFO("Loaded module '{0}'", new_module_file.filename().string());
        return;
    }

    FLOW_ERROR("Error loading symbol for RegisterModule from {0}", new_module_file.filename().string());
#ifdef FLOW_WINDOWS
    FreeLibrary(handle);
#else
    dlclose(handle);
#endif
}
catch (const std::exception& e)
{
    FLOW_ERROR("Caught exception while loading extensions: {0}", e.what());
}
catch (...)
{
    FLOW_ERROR("Caught unknown exception while loading extensions");
}

void Env::LoadModules(const std::filesystem::path& extension_path)
{
    for (const auto& file : std::filesystem::directory_iterator(extension_path))
    {
        LoadModule(file);
    }
}

void Env::UnloadModule(const std::filesystem::path& module_file)
{
    const auto& handle = _loaded_modules.at(module_file.filename().string());
#ifdef FLOW_WINDOWS
    FreeLibrary(std::bit_cast<HINSTANCE>(handle));
#else
    dlclose(handle);
#endif
    FLOW_INFO("Unloaded module '{0}'", module_file.filename().string());
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
