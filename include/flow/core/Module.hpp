// Copyright (c) 2024, Cisco Systems, Inc.
// All rights reserved.

#pragma once

#include "Core.hpp"

#include <nlohmann/json.hpp>

#include <filesystem>
#include <string>
#include <vector>

FLOW_NAMESPACE_BEGIN

using json = nlohmann::json;

class NodeFactory;

struct ModuleMetaData
{
    std::string Name;
    std::string Version;
    std::string Author;
    std::string Description;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ModuleMetaData, Name, Version, Author, Description);

class Module
{
    struct HandleUnloader
    {
        void operator()(void*);
    };

  public:
    /**
     * @brief Constructs a flow module from a given directory.
     * @param dir The directory to try to load.
     * @param factory The factory to load registered nodes into.
     */
    Module(const std::filesystem::path& dir, std::shared_ptr<NodeFactory> factory);

    Module(const json& module_json, const std::filesystem::path& dir, std::shared_ptr<NodeFactory> factory);

    ~Module();

    /**
     * @brief Loads a module from a given directory.
     * @param dir The directory to try to load.
     */
    bool Load(const std::filesystem::path& dir);

    /**
     * @brief Unloads the currently loaded module handle.
     */
    bool Unload();

    bool IsLoaded() const noexcept { return _handle != nullptr; }

    const ModuleMetaData& GetMetaData() const noexcept { return _metadata; }

  private:
    void ValidateMetaData(const json& module_json);

  public:
    static const std::string FileExtension;
    static const std::string BinaryExtension;

  private:
    ModuleMetaData _metadata;
    std::vector<std::string> _dependencies;

    std::shared_ptr<NodeFactory> _factory;
    std::unique_ptr<void, HandleUnloader> _handle;
};

FLOW_NAMESPACE_END
