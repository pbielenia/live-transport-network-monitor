#include <network-monitor/stomp-frame-builder.hpp>

using namespace NetworkMonitor;

std::string ParseCommand(StompCommand command) {
    return ToString(command) + '\n';
}

std::string ParseHeaders(const std::unordered_map<StompHeader, std::string> headers) {
    std::string parsed{};

    for (const auto [header, value] : headers) {
        parsed.append(ToString(header) + ':' + value + '\n');
    }
    parsed.push_back('\n');
    return parsed;
}

// StompFrame stomp_frame::Build(BuildParameters&& parameters)
// {
//     // TODO
// }

StompFrame stomp_frame::Build(StompError& error_code, const BuildParameters& parameters)
{
    std::string plain_content{};
    plain_content.append(ParseCommand(parameters.command));
    plain_content.append(ParseHeaders(parameters.headers));
    plain_content.append(parameters.body);
    plain_content.push_back('\0');
    
    return StompFrame{error_code, std::move(plain_content)};
}
