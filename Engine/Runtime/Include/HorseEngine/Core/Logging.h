#pragma once

#include "HorseEngine/Core.h"
#include <memory>
#include <spdlog/spdlog.h>

namespace Horse {

enum class LogChannel { Core, Render, Asset, Script };

class HORSE_API Logger {
public:
  static void Initialize();
  static void Shutdown();
  static void AddSink(std::shared_ptr<spdlog::sinks::sink> sink);

  static std::shared_ptr<spdlog::logger> GetLogger(LogChannel channel);

  template <typename... Args>
  static void LogCore(spdlog::level::level_enum level,
                      spdlog::format_string_t<Args...> fmt, Args &&...args) {
    GetLogger(LogChannel::Core)->log(level, fmt, std::forward<Args>(args)...);
  }

  template <typename... Args>
  static void LogRender(spdlog::level::level_enum level,
                        spdlog::format_string_t<Args...> fmt, Args &&...args) {
    GetLogger(LogChannel::Render)->log(level, fmt, std::forward<Args>(args)...);
  }
};

} // namespace Horse

// Convenience macros
#define HORSE_LOG_CORE_TRACE(...)                                              \
  Horse::Logger::LogCore(spdlog::level::trace, __VA_ARGS__)
#define HORSE_LOG_CORE_INFO(...)                                               \
  Horse::Logger::LogCore(spdlog::level::info, __VA_ARGS__)
#define HORSE_LOG_CORE_WARN(...)                                               \
  Horse::Logger::LogCore(spdlog::level::warn, __VA_ARGS__)
#define HORSE_LOG_CORE_ERROR(...)                                              \
  Horse::Logger::LogCore(spdlog::level::err, __VA_ARGS__)

#define HORSE_LOG_RENDER_TRACE(...)                                            \
  Horse::Logger::LogRender(spdlog::level::trace, __VA_ARGS__)
#define HORSE_LOG_RENDER_INFO(...)                                             \
  Horse::Logger::LogRender(spdlog::level::info, __VA_ARGS__)
#define HORSE_LOG_RENDER_WARN(...)                                             \
  Horse::Logger::LogRender(spdlog::level::warn, __VA_ARGS__)
#define HORSE_LOG_RENDER_ERROR(...)                                            \
  Horse::Logger::LogRender(spdlog::level::err, __VA_ARGS__)
