#include "flow/core/Module.hpp"

#include "flow/core/NodeFactory.hpp"

#include <nlohmann/json.hpp>

#include <bit>
#include <format>
#include <fstream>
#include <stdexcept>

#ifdef FLOW_WINDOWS
#include <windows.h>
#else
#include <dlfcn.h>
#endif

FLOW_NAMESPACE_START

const std::string Module::FileExtension = "flowmod";

#ifdef FLOW_WINDOWS
const std::string Module::BinaryExtension = ".dll";
#elif defined(FLOW_APPLE)
const std::string Module::BinaryExtension = ".dylib";
#else
const std::string Module::BinaryExtension = ".so";
#endif

Module::Module(const std::filesystem::path& dir, std::shared_ptr<NodeFactory> factory) : _factory(std::move(factory))
{
    auto result = Load(dir);
    switch (result)
    {
    case LoadingError::NotAModule:
        throw std::runtime_error(std::format("Directory is not a module. (dir={})", dir.string()));
    case LoadingError::FailedFileLoad:
        throw std::runtime_error(std::format("Failed to load module from dir. (dir={})", dir.string()));
    case LoadingError::RegisterFuncFailed:
        throw std::runtime_error(std::format("Failed to load symbols for RegisterModule. (dir={})", dir.string()));
    case LoadingError::None:
        [[fallthrough]];
    default:
        break;
    }
}

Module::~Module() { Unload(); }

Module::LoadingError Module::Load(const std::filesystem::path& filename)
{
    if (!std::filesystem::exists(filename) || !std::filesystem::is_regular_file(filename) ||
        filename.extension() != "." + FileExtension)
    {
        return LoadingError::NotAModule;
    }

    std::ifstream module_fs(filename);
    json module_j = json::parse(module_fs);
    _name         = module_j["name"];
    _version      = module_j["version"];
    _author       = module_j["author"];
    _description  = module_j["description"];

    const std::string module_file_name = _name + BinaryExtension;
    std::filesystem::path module_binary_path;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(filename.parent_path()))
    {
        if (!std::filesystem::is_regular_file(entry) || entry.path().filename() != module_file_name)
        {
            continue;
        }

        module_binary_path = entry;
        break;
    }

    if (!std::filesystem::exists(module_binary_path))
    {
        return LoadingError::FailedFileLoad;
    }

#ifdef FLOW_WINDOWS
#ifdef UNICODE
    HINSTANCE handle = LoadLibrary(module_binary_path.wstring().c_str());
#else
    HINSTANCE handle = LoadLibrary(module_binary_path.string().c_str());
#endif
    if (!handle)
    {
        return LoadingError::FailedFileLoad;
    }

    auto register_func = GetProcAddress(handle, NodeFactory::RegisterModuleFuncName);
#else
    void* handle = dlopen(module_binary_path.c_str(), RTLD_LAZY);
    if (!handle)
    {
        return LoadingError::FailedFileLoad;
    }

    auto register_func = dlsym(handle, NodeFactory::RegisterModuleFuncName);
#endif
    if (auto RegisterModule_func = std::bit_cast<NodeFactory::ModuleMethod_t>(register_func))
    {
        RegisterModule_func(_factory);
        _handle = std::bit_cast<void*>(handle);
        return LoadingError::None;
    }

#ifdef FLOW_WINDOWS
    FreeLibrary(handle);
#else
    dlclose(handle);
#endif

    return LoadingError::RegisterFuncFailed;
}

void Module::Unload()
{
#ifdef FLOW_WINDOWS
    auto unregister_func = GetProcAddress(std::bit_cast<HINSTANCE>(_handle), NodeFactory::UnregisterModuleFuncName);
#else
    auto unregister_func = dlsym(_handle, NodeFactory::UnregisterModuleFuncName);
#endif
    if (auto UnregisterModule_func = std::bit_cast<NodeFactory::ModuleMethod_t>(unregister_func))
    {
        UnregisterModule_func(_factory);
    }

#ifdef FLOW_WINDOWS
    FreeLibrary(std::bit_cast<HINSTANCE>(_handle));
#else
    dlclose(_handle);
#endif
}

FLOW_NAMESPACE_END
