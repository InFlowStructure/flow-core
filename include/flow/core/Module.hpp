// Copyright (c) 2024, Cisco Systems, Inc.
// All rights reserved.

#pragma once

#include "Core.hpp"

#include <filesystem>
#include <string>
#include <vector>

FLOW_NAMESPACE_START

class NodeFactory;

class Module
{
  public:
    /**
     * Error status when loading a module.
     */
    enum class LoadingError
    {
        None = 0,
        NotAModule,
        FailedFileLoad,
        RegisterFuncFailed,
    };

  public:
    /**
     * @brief Constructs a flow module from a given directory.
     * @param dir The directory to try to load.
     * @param factory The factory to load registered nodes into.
     */
    Module(const std::filesystem::path& dir, std::shared_ptr<NodeFactory> factory);

    ~Module();

    /**
     * @brief Loads a module from a given directory.
     * @param dir The directory to try to load.
     * @returns The result of loading the module.
     */
    LoadingError Load(const std::filesystem::path& dir);

    /**
     * @brief Unloads the currently loaded module handle.
     */
    void Unload();

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
    void* _handle;
};

FLOW_NAMESPACE_END
