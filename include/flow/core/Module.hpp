// Copyright (c) 2024, Cisco Systems, Inc.
// All rights reserved.

#pragma once

#include "Core.hpp"

#include <nlohmann/json_fwd.hpp>

#include <filesystem>
#include <string>
#include <vector>

FLOW_NAMESPACE_BEGIN

using json = nlohmann::json;

class NodeFactory;

class Module
{
    struct HandleDelete
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

    const std::string& GetName() const noexcept { return _name; }

    const std::string& GetVersion() const noexcept { return _version; }

    const std::string& GetAuthor() const noexcept { return _author; }

    const std::string& GetDescription() const noexcept { return _description; }

    const std::vector<std::string>& GetDependencies() const noexcept { return _dependencies; }

  private:
    void Validate(const json& module_json);

  public:
    static const std::string FileExtension;
    static const std::string BinaryExtension;

  private:
    std::string _name;
    std::string _version;
    std::string _author;
    std::string _description;
    std::vector<std::string> _dependencies;

    std::shared_ptr<NodeFactory> _factory;
    std::unique_ptr<void, HandleDelete> _handle;
};

FLOW_NAMESPACE_END
