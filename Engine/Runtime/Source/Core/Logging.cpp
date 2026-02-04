#include "HorseEngine/Core/Logging.h"
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/async.h>
#include <vector>

namespace Horse {

static std::shared_ptr<spdlog::logger> s_CoreLogger;
static std::shared_ptr<spdlog::logger> s_RenderLogger;
static std::shared_ptr<spdlog::logger> s_AssetLogger;
static std::shared_ptr<spdlog::logger> s_ScriptLogger;

void Logger::Initialize() {
    spdlog::init_thread_pool(8192, 1);
    
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("HorseEngine.log", true);
    
    std::vector<spdlog::sink_ptr> sinks{console_sink, file_sink};
    
    s_CoreLogger = std::make_shared<spdlog::async_logger>("CORE", sinks.begin(), sinks.end(),
                                                           spdlog::thread_pool(), spdlog::async_overflow_policy::block);
    s_RenderLogger = std::make_shared<spdlog::async_logger>("RENDER", sinks.begin(), sinks.end(),
                                                             spdlog::thread_pool(), spdlog::async_overflow_policy::block);
    s_AssetLogger = std::make_shared<spdlog::async_logger>("ASSET", sinks.begin(), sinks.end(),
                                                            spdlog::thread_pool(), spdlog::async_overflow_policy::block);
    s_ScriptLogger = std::make_shared<spdlog::async_logger>("SCRIPT", sinks.begin(), sinks.end(),
                                                             spdlog::thread_pool(), spdlog::async_overflow_policy::block);
    
    s_CoreLogger->set_level(spdlog::level::trace);
    s_RenderLogger->set_level(spdlog::level::trace);
    s_AssetLogger->set_level(spdlog::level::trace);
    s_ScriptLogger->set_level(spdlog::level::trace);
    
    spdlog::register_logger(s_CoreLogger);
    spdlog::register_logger(s_RenderLogger);
    spdlog::register_logger(s_AssetLogger);
    spdlog::register_logger(s_ScriptLogger);
    
    HORSE_LOG_CORE_INFO("Logger initialized");
}

void Logger::Shutdown() {
    spdlog::shutdown();
}

std::shared_ptr<spdlog::logger> Logger::GetLogger(LogChannel channel) {
    switch (channel) {
        case LogChannel::Core:   return s_CoreLogger;
        case LogChannel::Render: return s_RenderLogger;
        case LogChannel::Asset:  return s_AssetLogger;
        case LogChannel::Script: return s_ScriptLogger;
        default: return s_CoreLogger;
    }
}

} // namespace Horse
