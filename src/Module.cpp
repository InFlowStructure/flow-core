#include "flow/core/Module.hpp"

#include "flow/core/NodeFactory.hpp"

#include <nlohmann/json.hpp>

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
const std::string Module::BinaryExtension = ".dll";
#elif defined(FLOW_APPLE)
const std::string Module::BinaryExtension = ".dylib";
#else
const std::string Module::BinaryExtension = ".so";
#endif

namespace {
    // Get last system error message
    std::string GetLastErrorMsg() {
#ifdef FLOW_WINDOWS
        DWORD error = GetLastError();
        LPSTR messageBuffer = nullptr;
        FormatMessageA(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL, error, 0, (LPSTR)&messageBuffer, 0, NULL);
        std::string message(messageBuffer);
        LocalFree(messageBuffer);
        return message;
#else
        return dlerror();
#endif
    }
}

void Module::HandleDelete::operator()(void* handle)
{
#ifdef FLOW_WINDOWS
    if (!FreeLibrary(std::bit_cast<HINSTANCE>(handle)))
#else
    if (dlclose(handle) != 0)
#endif
    {
        throw std::runtime_error(std::format("Module handle failed to unload: {}", GetLastErrorMsg()));
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

Module::Module(const json& module_j, const std::filesystem::path& dir, std::shared_ptr<NodeFactory> factory)
    : _factory(std::move(factory))
{
    Validate(module_j);

    _name        = module_j["Name"];
    _version     = module_j["Version"];
    _author      = module_j["Author"];
    _description = module_j["Description"];

    Load(dir);
}

Module::~Module() { Unload(); }

bool Module::Load(const std::filesystem::path& path)
{
    if (_handle) return false;

    try {
        ValidatePath(path);
        LoadMetadata(path);
        LoadBinary(path.parent_path());
        return true;
    }
    catch(const std::exception& e) {
        throw std::runtime_error(std::format("Failed to load module: {} (path={})", 
            e.what(), path.string()));
    }
}

void Module::ValidatePath(const std::filesystem::path& path) const
{
    if (!std::filesystem::exists(path)) {
        throw std::runtime_error(std::format("Path does not exist (path={})", 
            path.string()));
    }

    if (std::filesystem::is_regular_file(path) && 
        path.extension() != ("." + FileExtension)) {
        throw std::runtime_error(std::format(
            "Invalid module extension (path={}, required={})", 
            path.string(), FileExtension));
    }
}

void Module::LoadMetadata(const std::filesystem::path& path)
{
    if (!std::filesystem::is_regular_file(path)) return;

    std::ifstream module_fs(path);
    if (!module_fs) {
        throw std::runtime_error(std::format("Failed to open module file (path={})",
            path.string()));
    }

    try {
        json module_j = json::parse(module_fs);
        Validate(module_j);
        
        _name = module_j["Name"];
        _version = module_j["Version"];
        _author = module_j["Author"];
        _description = module_j["Description"];
    }
    catch(const json::exception& e) {
        throw std::runtime_error(std::format("Invalid module JSON: {}", e.what()));
    }
}

void Module::LoadBinary(const std::filesystem::path& dir)
{
    const std::string module_file_name = _name + BinaryExtension;
    std::filesystem::path module_binary_path;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(dir))
    {
        if (!std::filesystem::is_regular_file(entry) ||
            (entry.path().filename() != module_file_name && entry.path().filename() != ("lib" + module_file_name)))
        {
            continue;
        }

        module_binary_path = entry;
        break;
    }

    if (!std::filesystem::exists(module_binary_path))
    {
        throw std::runtime_error(
            std::format("Module binary does not exist. (binary_path={})", module_binary_path.string()));
    }

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
        return;
    }

    HandleDelete{}(handle);

    throw std::runtime_error(std::format("Failed to load symbols for RegisterModule. (file={})", dir.string()));
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

void Module::Validate(const json& mod_j)
{
    if (!mod_j.contains("Name") || !mod_j.contains("Author") || !mod_j.contains("Version") ||
        !mod_j.contains("Description"))
    {
        throw std::invalid_argument("JSON is not a valid flow::Module");
    }

    std::regex semver_regex(R"(^(0|[1-9]\d*)\.(0|[1-9]\d*)\.(0|[1-9]\d*)$)");
    const std::string& version = mod_j["Version"].get_ref<const std::string&>();

    if (!std::regex_match(version, semver_regex))
    {
        throw std::invalid_argument(std::format("Version is not in numeric only format (version={})", version));
    }
}

FLOW_NAMESPACE_END
