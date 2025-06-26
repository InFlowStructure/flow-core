// Copyright (c) 2024, Cisco Systems, Inc.
// All rights reserved.

#include "flow/core/Module.hpp"

#include "flow/core/NodeFactory.hpp"

#include <nlohmann/json.hpp>
#include <zip.h>

#include <bit>
#include <format>
#include <fstream>
#include <regex>
#include <stdexcept>
#ifdef FLOW_WINDOWS
#include <windows.h>
#else
#include <dlfcn.h>
#endif

FLOW_NAMESPACE_BEGIN

const std::string Module::FileExtension = "flowmod";

#ifdef FLOW_WINDOWS
constexpr const char* platform = "windows";
#elif defined(FLOW_APPLE)
constexpr const char* platform = "macos";
#else
constexpr const char* platform = "linux";
#endif

#ifdef FLOW_X86_64
constexpr const char* architecture = "x86_64";
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

std::filesystem::path GetModuleMetaDataPath(const std::filesystem::path& dir) { return dir / "module.json"; }

std::filesystem::path GetTempModulePath()
{
    auto temp_path = std::filesystem::temp_directory_path() / "flow_modules";
    std::filesystem::create_directories(temp_path);
    return temp_path;
}

void Module::HandleUnloader::operator()(void* handle)
{
#ifdef FLOW_WINDOWS
    if (!FreeLibrary(std::bit_cast<HINSTANCE>(handle)))
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
        throw std::runtime_error(std::format("Path does not exist. (file={})", path.string()));
    }

    if (!std::filesystem::is_regular_file(path))
    {
        throw std::runtime_error(std::format("Path is not a file. (file={})", path.string()));
    }

    zip_t* archive = zip_open(path.string().c_str(), ZIP_RDONLY, nullptr);
    if (!archive)
    {
        throw std::runtime_error(std::format("Failed to open module archive. (file={})", path.string()));
    }

    const zip_int64_t num_entries = zip_get_num_entries(archive, 0);
    for (zip_int64_t i = 0; i < num_entries; ++i)
    {
        zip_stat_t file_info;
        zip_stat_init(&file_info);

        if (zip_stat_index(archive, i, 0, &file_info) != 0)
        {
            zip_close(archive);
            throw std::runtime_error(
                std::format("Failed to get file info from module archive. (file={})", path.string()));
        }

        auto file_path = GetTempModulePath() / file_info.name;
        auto file      = zip_fopen(archive, file_info.name, 0);
        if (!file)
        {
            zip_close(archive);
            throw std::runtime_error(
                std::format("Failed to open file in module archive. (file={})", file_path.string()));
        }

        std::vector<char> buffer(file_info.size);
        zip_fread(file, buffer.data(), file_info.size);
        zip_fclose(file);

        std::filesystem::create_directories(file_path.parent_path());
        if (std::filesystem::is_directory(file_path))
        {
            continue;
        }

        std::ofstream out_file(file_path, std::ios::binary);
        if (!out_file)
        {
            zip_close(archive);
            throw std::runtime_error(
                std::format("Failed to create file from module archive. (file={})", file_path.string()));
        }

        out_file.write(buffer.data(), buffer.size());
        out_file.close();
    }

    auto module_metadata_path = GetModuleMetaDataPath(GetTempModulePath() / path.stem());
    std::ifstream metadata_fs(module_metadata_path);
    json module_j = json::parse(metadata_fs);
    ValidateMetaData(module_j);
    _metadata = module_j;

    auto module_binary_path = GetModuleBinaryPath(GetTempModulePath() / path.stem());

#ifdef UNICODE
    auto handle = LoadModuleLibrary(module_binary_path.wstring());
#else
    auto handle = LoadModuleLibrary(module_binary_path.string());
#endif
    if (!handle)
    {
        throw std::runtime_error(
            std::format("Failed to load module binary. (binary_path={})", module_binary_path.string()));
    }

#ifdef FLOW_WINDOWS
    auto register_func = GetProcAddress(handle, NodeFactory::RegisterModuleFuncName);
#else
    auto register_func = dlsym(handle, NodeFactory::RegisterModuleFuncName);
#endif
    if (auto RegisterModule_func = std::bit_cast<NodeFactory::ModuleMethod_t>(register_func))
    {
        RegisterModule_func(_factory);
        _handle.reset(std::bit_cast<void*>(handle));
        return true;
    }

    HandleUnloader{}(handle);

    throw std::runtime_error(std::format("Failed to load symbols for RegisterModule. (file={})", path.string()));
}

bool Module::Unload()
{
    if (!_handle)
    {
        return false;
    }

#ifdef FLOW_WINDOWS
    auto unregister = GetProcAddress(std::bit_cast<HINSTANCE>(_handle.get()), NodeFactory::UnregisterModuleFuncName);
#else
    auto unregister = dlsym(_handle.get(), NodeFactory::UnregisterModuleFuncName);
#endif
    if (auto UnregisterModule_func = std::bit_cast<NodeFactory::ModuleMethod_t>(unregister))
    {
        UnregisterModule_func(_factory);
    }

    _handle.reset();
    return true;
}

void Module::ValidateMetaData(const json& mod_j)
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
        throw std::invalid_argument(
            std::format("Module metadata 'Version' field is not a valid semantic version. (version={})", version));
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

FLOW_NAMESPACE_END
