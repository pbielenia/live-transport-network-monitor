#pragma once

#include <ostream>

#include "network-monitor/stomp-frame.hpp"

namespace network_monitor {

std::ostream& operator<<(std::ostream& os, const StompFrame::Headers& headers);

}  // namespace network_monitor
