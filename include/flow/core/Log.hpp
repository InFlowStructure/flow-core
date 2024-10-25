// Copyright (c) 2024, Cisco Systems, Inc.
// All rights reserved.

#pragma once

#include "Core.hpp"

#include <spdlog/spdlog.h>

#include <filesystem>
#include <functional>
#include <memory>

FLOW_NAMESPACE_START

/**
 * @brief Static manager for logging in the core library
 *
 * @details Static manager for logging in the core library. It requires initialising at the beginning of the use of
 *          any Env or Graph.
 */
class Log
{
  public:
    /**
     * @brief Initialises the logging for the library.
     *
     * @param log_level The level of logging to be used. Matches the levels found in spdlog (trace = 0, debug = 1,
     *                  info = 2, warn = 3, err = 4, critical = 5, off).
     * @param log_file The file to log to
     * @param output_to_console Flag to mark whether or not the library should output logs in the console.
     */
    static void Init(int log_level, std::filesystem::path log_file, bool output_to_console) noexcept;

    /**
     * @brief Returns the core logger. If no logger was created, a default one will be created.
     * @returns The core logger.
     */
    static std::shared_ptr<spdlog::logger> GetCoreLogger() noexcept;

    /**
     * @brief Get the path of the log file.
     * @returns The specified logging path from Init.
     */
    static const std::filesystem::path& GetLogPath() noexcept;

    /**
     * @brief Adds a callback to the logger for when a message is logged.
     *
     * @param callback The callback to add to the callback sink.
     */
    static void AddLogCallback(const std::function<void(const spdlog::details::log_msg&)>& callback) noexcept;
};

FLOW_NAMESPACE_END

#define FLOW_TRACE(...) SPDLOG_LOGGER_TRACE(::FLOW_NAMESPACE::Log::GetCoreLogger(), __VA_ARGS__);
#define FLOW_DEBUG(...) SPDLOG_LOGGER_DEBUG(::FLOW_NAMESPACE::Log::GetCoreLogger(), __VA_ARGS__)
#define FLOW_INFO(...) SPDLOG_LOGGER_INFO(::FLOW_NAMESPACE::Log::GetCoreLogger(), __VA_ARGS__)
#define FLOW_WARN(...) SPDLOG_LOGGER_WARN(::FLOW_NAMESPACE::Log::GetCoreLogger(), __VA_ARGS__)
#define FLOW_ERROR(...) SPDLOG_LOGGER_ERROR(::FLOW_NAMESPACE::Log::GetCoreLogger(), __VA_ARGS__)
#define FLOW_CRITICAL(...) SPDLOG_LOGGER_CRITICAL(::FLOW_NAMESPACE::Log::GetCoreLogger(), __VA_ARGS__)
