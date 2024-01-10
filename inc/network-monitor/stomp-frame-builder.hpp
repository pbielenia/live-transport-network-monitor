#pragma once

#include <network-monitor/stomp-frame.hpp>

namespace NetworkMonitor {

namespace stomp_frame {

struct BuildParameters {
    BuildParameters(StompCommand command) : command{command} {}

    StompCommand command;
    // TODO: StompFrame::Headers may be used possibly
    std::unordered_map<StompHeader, std::string> headers;
    std::string body;
};

// TODO: make a version involving less copying
// StompFrame Build(BuildParameters&& parameters);

// TODO: rename to `BuildFrame` or `MakeFrame`
StompFrame Build(StompError& error_code, const BuildParameters& parameters);

// StompFrame MakeConnectedFrame()

}  // namespace stomp_frame
}  // namespace NetworkMonitor
