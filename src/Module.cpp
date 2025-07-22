// Copyright (c) 2024, Cisco Systems, Inc.
// All rights reserved.

#include "flow/core/Module.hpp"

#include "flow/core/NodeFactory.hpp"

#include <Zipper/Unzipper.hpp>
#include <nlohmann/json.hpp>

#include <bit>
#include <format>
#include <fstream>
#include <regex>
#include <stdexcept>
#ifdef FLOW_WINDOWS
#include <windows.h>
#else
#include "Module.hpp"
#include <dlfcn.h>
#endif

FLOW_NAMESPACE_BEGIN

using namespace zipper;

struct formatted_error : public std::runtime_error
{
    template<typename... Args>
    formatted_error(const std::format_string<Args...> fmt, Args&&... args)
        : runtime_error(std::format(fmt, std::forward<Args>(args)...))
    {
    }
};

struct runtime_error : public formatted_error
{
    using formatted_error::formatted_error;
};

struct invalid_argument : public formatted_error
{
    using formatted_error::formatted_error;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ModuleMetaData, Name, Version, Author, Description);

const std::string Module::FileExtension = "fmod";

#ifdef FLOW_WINDOWS
constexpr const char* platform = "windows";
#elif defined(FLOW_APPLE)
constexpr const char* platform = "macos";
#else
constexpr const char* platform = "linux";
#endif

#ifdef FLOW_X86_64
constexpr const char* architecture = "x86_64";
#elif defined(FLOW_X86)
constexpr const char* architecture = "x86";
#elif defined(FLOW_ARM)
constexpr const char* architecture = "arm64";
#else
#error "Unsupported architecture"
#endif

#ifdef FLOW_WINDOWS
constexpr const char* library_extension = ".dll";
#elif defined(FLOW_APPLE)
constexpr const char* library_extension = ".dylib";
#else
constexpr const char* library_extension = ".so";
#endif

/**
 * @brief Get the Module Binary Path object.
 *
 * @param dir The directory of the module files.
 * @returns The path to the module binary.
 */
std::filesystem::path GetModuleBinaryPath(const std::filesystem::path& dir)
{
    return dir / platform / architecture / dir.stem().replace_extension(library_extension);
}

/**
 * @brief Get the Module Meta Data Path object.
 *
 * @param dir The directory of the module files.
 * @returns The path to the module metadata file.
 */
std::filesystem::path GetModuleMetaDataPath(const std::filesystem::path& dir) { return dir / "module.json"; }

/**
 * @brief Get the temporary module path.
 *
 * This function creates a directory for temporary modules if it does not exist.
 * The path is platform-specific and uses the system's temporary directory.
 *
 * @returns The path to the temporary module directory.
 */
std::filesystem::path GetTempModulePath()
{
    auto temp_path = std::filesystem::temp_directory_path() / "flow_modules";
    std::filesystem::create_directories(temp_path);
    return temp_path;
}

void ModuleMetaData::Validate(const json& mod_j)
{
    if (!mod_j.contains("Name") || !mod_j["Name"].is_string())
    {
        throw std::invalid_argument("Module metadata is missing 'Name' field or it is not a string.");
    }

    if (!mod_j.contains("Version") || !mod_j["Version"].is_string())
    {
        throw std::invalid_argument("Module metadata is missing 'Version' field or it is not a string.");
    }

    std::regex semver_regex(R"(^(0|[1-9]\d*)\.(0|[1-9]\d*)\.(0|[1-9]\d*)$)");
    const std::string& version = mod_j["Version"].get_ref<const std::string&>();

    if (!std::regex_match(version, semver_regex))
    {
        throw flow::invalid_argument("Module metadata 'Version' field is not a valid semantic version. (version={})",
                                     version);
    }

    if (!mod_j.contains("Author") || !mod_j["Author"].is_string())
    {
        throw std::invalid_argument("Module metadata is missing 'Author' field or it is not a string.");
    }

    if (!mod_j.contains("Description") || !mod_j["Description"].is_string())
    {
        throw std::invalid_argument("Module metadata is missing 'Description' field or it is not a string.");
    }
}

void Module::HandleUnloader::operator()(void* handle)
{
#ifdef FLOW_WINDOWS
    if (!FreeLibrary(reinterpret_cast<HINSTANCE>(handle)))
#else
    if (dlclose(handle) != 0)
#endif
    {
        throw std::runtime_error("Module handle failed to unload");
    }
}

#ifdef UNICODE
auto LoadModuleLibrary(const std::wstring& name)
#else
auto LoadModuleLibrary(const std::string& name)
#endif
{
#ifdef FLOW_WINDOWS
    return LoadLibrary(name.c_str());
#else
    return dlopen(name.c_str(), RTLD_LAZY);
#endif
}

Module::Module(std::shared_ptr<NodeFactory> factory) : _factory(std::move(factory)) {}

Module::Module(const std::filesystem::path& dir, std::shared_ptr<NodeFactory> factory) : _factory(std::move(factory))
{
    Load(dir);
}

Module::~Module() { Unload(); }

bool Module::Load(const std::filesystem::path& path)
{
    if (_handle)
    {
        return false;
    }

    if (!std::filesystem::exists(path))
    {
        throw flow::runtime_error("Path does not exist. (file={})", path.string());
    }

    if (!std::filesystem::is_regular_file(path))
    {
        throw flow::runtime_error("Path is not a file. (file={})", path.string());
    }

    Unzipper unzipper(path.string());
    if (!unzipper.isOpened())
    {
        throw flow::runtime_error("Failed to open module archive. (file={})", path.string());
    }

    unzipper.extractAll(GetTempModulePath().string(), Unzipper::OverwriteMode::Overwrite);
    unzipper.close();

    json module_j = json::parse(std::ifstream(GetModuleMetaDataPath(GetTempModulePath() / path.stem())));
    ModuleMetaData::Validate(module_j);
    _metadata = module_j;

    auto binary_path = GetModuleBinaryPath(GetTempModulePath() / path.stem());

#ifdef UNICODE
    auto handle = LoadModuleLibrary(binary_path.wstring());
#else
    auto handle = LoadModuleLibrary(binary_path.string());
#endif
    if (!handle)
    {
        throw flow::runtime_error("Failed to load module binary. (binary_path={})", binary_path.string());
    }

    _handle.reset(reinterpret_cast<void*>(handle));

    RegisterModuleNodes(_factory);

    return true;
}

bool Module::Unload()
{
    if (!_handle)
    {
        return false;
    }

    UnregisterModuleNodes(_factory);

    _handle.reset();
    return true;
}

void Module::RegisterModuleNodes() { RegisterModuleNodes(_factory); }

void Module::UnregisterModuleNodes() { UnregisterModuleNodes(_factory); }

void Module::RegisterModuleNodes(const std::shared_ptr<NodeFactory>& factory)
{
    if (!_handle)
    {
        throw std::runtime_error("Module is not loaded, cannot register nodes.");
    }

    if (!factory)
    {
        throw std::invalid_argument("NodeFactory is null, cannot register nodes.");
    }

#ifdef FLOW_WINDOWS
    auto register_func =
        GetProcAddress(reinterpret_cast<HINSTANCE>(_handle.get()), NodeFactory::RegisterModuleFuncName);
#else
    auto register_func = dlsym(_handle.get(), NodeFactory::RegisterModuleFuncName);
#endif
    if (auto RegisterModule_func = reinterpret_cast<NodeFactory::ModuleMethod_t>(register_func)) [[likely]]
    {
        return RegisterModule_func(factory);
    }

    throw std::runtime_error("Failed to load symbols for RegisterModule.");
}

void Module::UnregisterModuleNodes(const std::shared_ptr<NodeFactory>& factory)
{
    if (!_handle)
    {
        throw std::runtime_error("Module is not loaded, cannot unregister nodes.");
    }

    if (!factory)
    {
        throw std::invalid_argument("NodeFactory is null, cannot unregister nodes.");
    }

#ifdef FLOW_WINDOWS
    auto unregister = GetProcAddress(reinterpret_cast<HINSTANCE>(_handle.get()), NodeFactory::UnregisterModuleFuncName);
#else
    auto unregister = dlsym(_handle.get(), NodeFactory::UnregisterModuleFuncName);
#endif
    if (auto UnregisterModule_func = reinterpret_cast<NodeFactory::ModuleMethod_t>(unregister)) [[likely]]
    {
        return UnregisterModule_func(factory);
    }

    throw std::runtime_error("Failed to load symbols for UnregisterModule.");
}

FLOW_NAMESPACE_END
