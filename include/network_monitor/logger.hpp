#pragma once

#include "spdlog/spdlog.h"

#define LOG_DEBUG(...) (SPDLOG_LOGGER_DEBUG(GetLogger(), __VA_ARGS__))
#define LOG_INFO(...) (SPDLOG_LOGGER_INFO(GetLogger(), __VA_ARGS__))
#define LOG_WARN(...) (SPDLOG_LOGGER_WARN(GetLogger(), __VA_ARGS__))
#define LOG_ERROR(...) (SPDLOG_LOGGER_ERROR(GetLogger(), __VA_ARGS__))

namespace network_monitor {

std::shared_ptr<spdlog::logger> GetLogger();

}
