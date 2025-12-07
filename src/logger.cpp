#include "network_monitor/logger.hpp"

#include <memory>
#include <spdlog/common.h>
#include <spdlog/logger.h>

#include "spdlog/sinks/stdout_color_sinks.h"

namespace network_monitor {

std::shared_ptr<spdlog::logger> GetLogger() {
  static auto logger = []() {
    auto console_sink = std::make_shared<spdlog::sinks::stderr_color_sink_mt>();
    console_sink->set_pattern("%^[%Y-%m-%d %H:%M:%S.%e][%l][%s:%#][%!] %v%$");
    console_sink->set_level(spdlog::level::debug);

    auto logger =
        std::make_shared<spdlog::logger>("network-monitor", console_sink);
    logger->set_level(spdlog::level::debug);
    logger->flush_on(spdlog::level::warn);

    return logger;
  }();

  return logger;
}

}  // namespace network_monitor
