// Copyright (c) 2024, Cisco Systems, Inc.
// All rights reserved.

#pragma once

#include "Core.hpp"

#include <nlohmann/json_fwd.hpp>

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

FLOW_NAMESPACE_BEGIN

using json = nlohmann::json;

class NodeFactory;

/**
 * @brief Structure to hold metadata for a flow module.
 */
struct ModuleMetaData
{
    /// The name of the module.
    std::string Name;

    /// The version of the module in semantic versioning format (e.g., "1.0.0").
    std::string Version;

    /// The author of the module.
    std::string Author;

    /// A description of the module.
    std::string Description;
};

class Module
{
    struct HandleUnloader
    {
        void operator()(void*);
    };

  public:
    /**
     * @brief Constructs a flow module with a given NodeFactory.
     * @param factory The factory to load registered nodes into.
     */
    Module(std::shared_ptr<NodeFactory> factory);

    /**
     * @brief Constructs a flow module from a given JSON metadata and directory.
     * @param dir The directory to try to load.
     * @param factory The factory to load registered nodes into.
     */
    Module(const std::filesystem::path& dir, std::shared_ptr<NodeFactory> factory);

    /**
     * @brief Destructor for the Module class. Unloads the module if loaded.
     */
    ~Module();

    /**
     * @brief Loads a module from a given directory.
     * @param dir The directory to try to load.
     * @return True if the module was loaded successfully, false otherwise.
     */
    bool Load(const std::filesystem::path& dir);

    /**
     * @brief Unloads the currently loaded module handle.
     * @return True if the module was unloaded successfully, false otherwise.
     */
    bool Unload();

    /**
     * @brief Registers the module nodes with the provided factory.
     * @param factory The factory to register nodes with.
     */
    void RegisterModuleNodes(const std::shared_ptr<NodeFactory>& factory);

    /**
     * @brief Unregisters the module nodes from the provided factory.
     * @param factory The factory to unregister nodes from.
     */
    void UnregisterModuleNodes(const std::shared_ptr<NodeFactory>& factory);

    /**
     * @brief Checks if the module is currently loaded.
     * @return True if loaded, false otherwise.
     */
    bool IsLoaded() const noexcept { return _handle != nullptr; }

    /**
     * @brief Gets the metadata associated with the module.
     * @return A const reference to the ModuleMetaData.
     */
    const std::optional<ModuleMetaData>& GetMetaData() const noexcept { return _metadata; }

  private:
    void ValidateMetaData(const json& module_json);

  public:
    /**
     * @brief The file extension for module metadata files.
     */
    static const std::string FileExtension;

  private:
    std::optional<ModuleMetaData> _metadata;
    std::vector<std::string> _dependencies;

    std::shared_ptr<NodeFactory> _factory;
    std::unique_ptr<void, HandleUnloader> _handle;
};

FLOW_NAMESPACE_END
