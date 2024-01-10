#include <network-monitor/stomp-frame-builder.hpp>

using namespace NetworkMonitor;

namespace {
    void EmplaceIfValueNotEmpty(StompFrame::Headers& headers, StompHeader header, const std::string& value)
    {
        if (!value.empty()) {
            headers.emplace(header, value);
        }
    }
}

std::string ParseCommand(StompCommand command) {
    return ToString(command) + '\n';
}

std::string ParseHeaders(const StompFrame::Headers& headers) {
    std::string parsed{};

    for (const auto [header, value] : headers) {
        parsed.append(ToString(header));
        parsed.append(":");
        parsed.append(value);
        parsed.append("\n");
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

StompFrame stomp_frame::MakeConnectedFrame(
    const std::string& version,
    const std::string& session,
    const std::string& server,
    const std::string& heart_beat)
{
    BuildParameters parameters(StompCommand::Connected);
    parameters.headers.emplace(StompHeader::Version, version);
    EmplaceIfValueNotEmpty(parameters.headers, StompHeader::Session, session);
    EmplaceIfValueNotEmpty(parameters.headers, StompHeader::Server, server);
    EmplaceIfValueNotEmpty(parameters.headers, StompHeader::HeartBeat, heart_beat);

    StompError error;
    return Build(error, parameters);
}

StompFrame stomp_frame::MakeErrorFrame(
    const std::string& message,
    const std::string& body)
{
    BuildParameters parameters(StompCommand::Error);
    EmplaceIfValueNotEmpty(parameters.headers, StompHeader::Message, message);
    if (!body.empty()) {
        parameters.body = body;
    }

    StompError error;
    return Build(error, parameters);
}

StompFrame MakeReceiptFrame(
    const std::string& receipt_id)
{
    // TODO
    return {};
}

StompFrame MakeMessageFrame(
    const std::string& destination,
    const std::string& message_id,
    const std::string& subscription,
    const std::string& ack,
    const std::string& body,
    const std::string& content_length,
    const std::string& content_type)
{
    // TODO
    return {};
}