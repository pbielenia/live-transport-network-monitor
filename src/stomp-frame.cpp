#include <algorithm>
#include <boost/bimap.hpp>
#include <network-monitor/stomp-frame.hpp>

using namespace NetworkMonitor;

template <typename L, typename R>
boost::bimap<L, R> MakeBimap(
    std::initializer_list<typename boost::bimap<L, R>::value_type> list)
{
    return boost::bimap<L, R>(list.begin(), list.end());
}

static const auto stomp_commands_strings{MakeBimap<StompCommand, std::string_view>({
    // clang-format off
    {StompCommand::Abort,           "ABORT"             },
    {StompCommand::Ack,             "ACK"               },
    {StompCommand::Begin,           "BEGIN"             },
    {StompCommand::Commit,          "COMMIT"            },
    {StompCommand::Connect,         "CONNECT"           },
    {StompCommand::Connected,       "CONNECTED"         },
    {StompCommand::Disconnect,      "DISCONNECT"        },
    {StompCommand::Error,           "ERROR"             },
    {StompCommand::Invalid,         "INVALID_COMMAND"   },
    {StompCommand::Message,         "MESSAGE"           },
    {StompCommand::NAck,            "NACK"              },
    {StompCommand::Receipt,         "RECEIPT"           },
    {StompCommand::Send,            "SEND"              },
    {StompCommand::Stomp,           "STOMP"             },
    {StompCommand::Subscribe,       "SUBSCRIBE"         },
    {StompCommand::Unsubscribe,     "UNSUBSCRIBE"       },
    // clang-format on
})};

static const auto stomp_headers_strings{MakeBimap<StompHeader, std::string_view>({
    // clang-format off
    {StompHeader::AcceptVersion,    "accept-version"    },
    {StompHeader::Ack,              "ack"               },
    {StompHeader::ContentLength,    "content-length"    },
    {StompHeader::ContentType,      "content-type"      },
    {StompHeader::Destination,      "destination"       },
    {StompHeader::HeartBeat,        "heart-beat"        },
    {StompHeader::Host,             "host"              },
    {StompHeader::Id,               "id"                },
    {StompHeader::Invalid,          "invalid-header"    },
    {StompHeader::Login,            "login"             },
    {StompHeader::Message,          "message"           },
    {StompHeader::MessageId,        "message-id"        },
    {StompHeader::Passcode,         "passcode"          },
    {StompHeader::Receipt,          "receipt"           },
    {StompHeader::ReceiptId,        "receipt-id"        },
    {StompHeader::Session,          "session"           },
    {StompHeader::Server,           "server"            },
    {StompHeader::Subscription,     "subscription"      },
    {StompHeader::Transaction,      "transaction"       },
    {StompHeader::Version,          "version"           },
    // clang-format on
})};

static const auto stomp_errors_strings{MakeBimap<StompError, std::string_view>({
    // clang-format off
    {StompError::Ok,                            "Ok"                            },
    {StompError::UndefinedError,                "UndefinedError"                },
    {StompError::InvalidCommand,                "InvalidCommand"                },
    {StompError::InvalidHeader,                 "InvalidHeader"                 },
    {StompError::InvalidHeaderValue,            "InvalidHeaderValue"            },
    {StompError::NoHeaderValue,                 "NoHeaderValue"                 },
    {StompError::EmptyHeaderValue,              "EmptyHeaderValue"              },
    {StompError::NoNewlineCharacters,           "NoNewlineCharacters"           },
    {StompError::MissingLastHeaderNewline,      "MissingLastHeaderNewline"      },
    {StompError::MissingBodyNewline,            "MissingBodyNewline"            },
    {StompError::MissingClosingNullCharacter,   "MissingClosingNullCharacter"   },
    {StompError::JunkAfterBody,                 "JunkAfterBody"                 },
    {StompError::ContentLengthsDontMatch,       "ContentLengthsDontMatch"       },
    {StompError::MissingRequiredHeader,         "MissingRequiredHeader"         },
    {StompError::NoData,                        "NoData"                        },
    {StompError::MissingCommand,                "MissingCommand"                },
    {StompError::NoHeaderName,                  "NoHeaderName"                  }
    // clang-format on
})};

std::string_view ToStringView(const StompCommand& command)
{
    const auto string_representation{stomp_commands_strings.left.find(command)};
    if (string_representation == stomp_commands_strings.left.end()) {
        return stomp_commands_strings.left.find(StompCommand::Invalid)->second;
    } else {
        return string_representation->second;
    }
}

std::string_view ToStringView(const StompHeader& header)
{
    const auto string_representation{stomp_headers_strings.left.find(header)};
    if (string_representation == stomp_headers_strings.left.end()) {
        return stomp_headers_strings.left.find(StompHeader::Invalid)->second;
    } else {
        return string_representation->second;
    }
}

std::string_view ToStringView(const StompError& error)
{
    const auto string_representation{stomp_errors_strings.left.find(error)};
    if (string_representation == stomp_errors_strings.left.end()) {
        return stomp_errors_strings.left.find(StompError::UndefinedError)->second;
    } else {
        return string_representation->second;
    }
}

std::ostream& NetworkMonitor::operator<<(std::ostream& os, const StompCommand& command)
{
    os << ToStringView(command);
    return os;
}

std::ostream& NetworkMonitor::operator<<(std::ostream& os, const StompHeader& header)
{
    os << ToStringView(header);
    return os;
}

std::ostream& NetworkMonitor::operator<<(std::ostream& os, const StompError& error)
{
    os << ToStringView(error);
    return os;
}

std::string NetworkMonitor::ToString(const StompCommand& command)
{
    return std::string(ToStringView(command));
}

std::string NetworkMonitor::ToString(const StompHeader& header)
{
    return std::string(ToStringView(header));
}

std::string NetworkMonitor::ToString(const StompError& error)
{
    return std::string(ToStringView(error));
}

StompFrame::StompFrame()
{
    // TODO
}

StompFrame::StompFrame(StompError& error_code, const std::string& content)
{
    // TODO: Avoid unnecessary re-scans of the same plain-text STOMP frame, since a single
    //       scan should be enough to gather all the information we need.
    // CTOR SHOULD:
    //       - check that it is formally correct according to the protocol,
    //       - checks that it contains the expected headers for the specific command,
    //       - and interprets the data for later use.
    //
    // - Convert the STOMP command, which is a string, into the corresponding StompCommand
    //   enum.
    // - Organize the STOMP headers in a map so that it's easier to access a specific
    //   header and its value when needed.
    // - Provide a direct view into the body of the frame, which is what the user will
    //   care about when processing the frame.

    // TODO: If either a parser or validation error occur, break further parsings. No need
    //       to reset any already set field. Return a descriptive error is sufficent.

    error_code = ParseFrame(content);
    if (error_code != StompError::Ok) {
        return;
    }
    error_code = ValidateFrame();
}

StompFrame::StompFrame(StompError& error_code, std::string&& content)
{
    error_code = ParseFrame(content);
    if (error_code != StompError::Ok) {
        return;
    }
    error_code = ValidateFrame();
}

StompError StompFrame::ParseFrame(const std::string_view frame)
{
    static const char newline_character{'\n'};
    static const char colon_character{':'};
    static const char null_character{'\0'};

    // Run pre-checks

    if (frame.empty()) {
        return StompError::NoData;
    }

    if (frame.at(0) == newline_character) {
        return StompError::MissingCommand;
    }

    if (frame.back() != null_character) {
        return StompError::MissingClosingNullCharacter;
    }

    const auto command_end{frame.find(newline_character)};
    if (command_end == std::string::npos) {
        return StompError::NoNewlineCharacters;
    }

    // If no "\n\n" pattern, then the body's newline is missing.
    const auto double_newline_pointer = std::adjacent_find(
        frame.begin(), frame.end(),
        [](const auto& a, const auto& b) { return a == b && a == '\n'; });
    if (double_newline_pointer == frame.end()) {
        return StompError::MissingBodyNewline;
    }

    // Parse command
    auto command_plain{frame.substr(0, command_end)};
    auto command{stomp_commands_strings.right.find(command_plain)};
    if (command == stomp_commands_strings.right.end()) {
        return StompError::InvalidCommand;
    }
    command_ = command->second;

    // Parse headers
    // Headers are optional.
    auto next_line_start{command_end + 1};
    while (1) {
        // Check if there's something more in the frame.
        if (next_line_start >= frame.size()) {
            return StompError::MissingBodyNewline;
        }

        if (frame.at(next_line_start) == newline_character) {
            // CONNECT\n
            // \n
            //
            // No headers, go parse the body.
            break;
        }
        if (frame.at(next_line_start) == colon_character) {
            // CONNECT\n
            // :
            return StompError::NoHeaderName;
        }
        if (frame.at(next_line_start) == null_character) {
            // CONNECT\n
            // \0
            return StompError::MissingBodyNewline;
        }

        const auto next_colon_position{frame.find(colon_character, next_line_start)};
        const auto next_newline_position{frame.find(newline_character, next_line_start)};

        if (next_colon_position == std::string::npos) {
            // CONNECT\n
            // header-1:value\n
            // header-2\n
            //       ^ no header value
            return StompError::NoHeaderValue;
        }
        if (next_newline_position == std::string::npos) {
            // CONNECT\n
            // header:value
            //     <-- missing newline
            // \0
            return StompError::MissingLastHeaderNewline;
        }
        if (next_newline_position < next_colon_position) {
            // CONNECT\n
            // header-1\n
            //         ^ missing colon
            // header-2:value\n
            // \0
            return StompError::NoHeaderValue;
        }
        if (frame.at(next_colon_position + 1) == frame.at(next_newline_position)) {
            // CONNECT\n
            // header:\n
            //       ^ ^ missing header value
            return StompError::EmptyHeaderValue;
        }

        // CONNECT\n
        // header-1:

        // Read header.
        const auto header_plain{
            frame.substr(next_line_start, next_colon_position - next_line_start)};
        const auto header{stomp_headers_strings.right.find(header_plain)};
        if (header == stomp_headers_strings.right.end()) {
            return StompError::InvalidHeader;
        }

        // Find the value. It's known '\n' is in a further part, so no frame end
        // concerns. Also the value will be no empty.
        const auto value{frame.substr(next_colon_position + 1,
                                      next_newline_position - next_colon_position - 1)};

        headers_.emplace(header->second, value);
        next_line_start = next_newline_position + 1;
    }

    // Parse body

    // CONNECT\n
    // header-1:value
    // \n <-- check if the newline is present
    if (frame.at(next_line_start) != newline_character) {
        return StompError::MissingBodyNewline;
    }
    next_line_start++;

    if (next_line_start >= frame.size()) {
        // CONNECT\n
        // \n
        //     <-- missing null
        return StompError::MissingClosingNullCharacter;
    }
    const auto null_position{frame.find(null_character)};
    if (null_position == std::string::npos) {
        // CONNECT\n
        // \n
        // Frame body
        //            ^ missing null
        return StompError::MissingClosingNullCharacter;
    }

    if (HasHeader(StompHeader::ContentLength)) {
        // -2 excludes the closing null character.
        body_ = frame.substr(next_line_start, frame.size() - next_line_start - 1);
    } else {
        if (null_position + 1 != frame.size()) {
            // CONNECT\n
            // \n
            // Frame body\0junk
            //             ^ unexpected characters
            return StompError::JunkAfterBody;
        }
        body_ = frame.substr(next_line_start, null_position - next_line_start);
    }

    return StompError::Ok;
}

StompError StompFrame::ValidateFrame()
{
    // Check if content-length match body_'s length.
    if (HasHeader(StompHeader::ContentLength)) {
        int expected_content_length{};
        try {
            expected_content_length =
                std::stoi(GetHeaderValue(StompHeader::ContentLength).data());
        } catch (const std::exception& exception) {
            return StompError::InvalidHeaderValue;
        }
        if (expected_content_length != body_.length()) {
            return StompError::ContentLengthsDontMatch;
        }
    }

    // Check if required headers are present.
    // if (body_.length())
    // return StompError::ContentLengthsDontMatch;

    return StompError::Ok;
}

StompFrame::StompFrame(const StompFrame& other)
{
    //
}

StompFrame::StompFrame(StompFrame&& other)
{
    //
}

StompFrame& StompFrame::operator=(const StompFrame& other)
{
    //
    return *this;
}

StompFrame& StompFrame::operator=(StompFrame&& other)
{
    //
    return *this;
}

StompCommand StompFrame::GetCommand() const
{
    return command_;
}

const bool StompFrame::HasHeader(const StompHeader& header) const
{
    return static_cast<bool>(headers_.count(header));
}

const std::string_view& StompFrame::GetHeaderValue(const StompHeader& header) const
{
    static const std::string_view empty_header_value{""};
    if (HasHeader(header)) {
        return headers_.at(header);
    }
    return empty_header_value;
}

const std::string_view& StompFrame::GetBody() const
{
    return body_;
}
