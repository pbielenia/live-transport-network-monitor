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
    {StompError::Ok,                "Ok"                },
    {StompError::UndefinedError,    "UndefinedError"    },
    // clang-format off
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
    //
}

StompFrame::StompFrame(StompError& error_code, const std::string& content)
{
    //
}

StompFrame::StompFrame(StompError& error_code, std::string&& content)
{
    //
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
    //
    return StompCommand::Invalid;
}

const bool StompFrame::HasHeader(const StompHeader& header) const
{
    //
    return false;
}

const std::string_view& StompFrame::GetHeaderValue(const StompHeader& header) const
{
    //
    return ToStringView(StompHeader::Invalid);
}

const std::string_view& StompFrame::GetBody() const
{
    //
    return body_;
}
