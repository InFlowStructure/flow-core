// Copyright (c) 2024, Cisco Systems, Inc.
// All rights reserved.

#include "Log.hpp"

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/callback_sink.h>
#include <spdlog/sinks/dist_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

static std::shared_ptr<spdlog::logger> _core_logger;
static std::shared_ptr<spdlog::sinks::dist_sink<std::mutex>> _dist_sink;
static std::filesystem::path _log_path;

constexpr const char* LoggerName = "FLOW";

FLOW_NAMESPACE_START

void Log::Init(int log_level, std::filesystem::path log_file, bool output_to_console) noexcept
{
    _dist_sink = std::make_shared<spdlog::sinks::dist_sink<std::mutex>>();
    _log_path  = log_file;

    if (output_to_console)
    {
        auto sink = std::make_shared<spdlog::sinks::stderr_color_sink_mt>();
        sink->set_pattern("%^[%T]: %v%$");
        _dist_sink->add_sink(std::move(sink));
    }

    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(_log_path.string(), true);
    file_sink->set_pattern("[%T] [%l]: %v");
    _dist_sink->add_sink(std::move(file_sink));

    auto logger = std::make_shared<spdlog::logger>(LoggerName, _dist_sink);
    logger->set_level(static_cast<spdlog::level::level_enum>(log_level));
    logger->flush_on(static_cast<spdlog::level::level_enum>(log_level));

    spdlog::register_logger(logger);
}

std::shared_ptr<spdlog::logger> Log::GetCoreLogger() noexcept
{
    return spdlog::get(LoggerName) ? spdlog::get(LoggerName) : std::make_shared<spdlog::logger>(LoggerName);
}

const std::filesystem::path& Log::GetLogPath() noexcept { return _log_path; }

void Log::AddLogCallback(const std::function<void(const spdlog::details::log_msg&)>& callback) noexcept
{
    auto sink = std::make_shared<spdlog::sinks::callback_sink_mt>(callback);
    sink->set_pattern("%T [%l]: %v");
    _dist_sink->add_sink(std::move(sink));
}

FLOW_NAMESPACE_END
