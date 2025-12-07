#include "network_monitor/stomp_frame.hpp"

#include <algorithm>
#include <array>
#include <exception>
#include <expected>
#include <optional>
#include <ostream>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

using namespace network_monitor;

namespace {

constexpr char kNewlineCharacter{'\n'};
constexpr char kColonCharacter{':'};
constexpr char kNullCharacter{'\0'};

template <typename Enum, size_t N>
class EnumBimap {
 public:
  constexpr EnumBimap(std::array<std::pair<Enum, std::string_view>, N> entries)
      : entries_{std::move(entries)} {}

  constexpr std::optional<std::string_view> ToStringView(
      Enum entry) const noexcept {
    for (auto [key, value] : entries_) {
      if (key == entry) {
        return value;
      }
    }
    return std::nullopt;
  }

  constexpr std::optional<Enum> ToEnum(std::string_view entry) const noexcept {
    for (auto [key, value] : entries_) {
      if (value == entry) {
        return key;
      }
    }
    return std::nullopt;
  }

 private:
  EntriesType entries_;
};

constexpr auto kStompCommands = EnumBimap<StompCommand, 16>{{{
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
}}};

constexpr auto kStompHeaders = EnumBimap<StompHeader, 20>{{{
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
}}};

constexpr auto kStompErrors = EnumBimap<StompError, 17>{{{
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
    {StompError::NoHeaderName,                  "NoHeaderName"                  },
    // clang-format on
}}};

namespace required_headers {
constexpr auto kForConnect = std::array<StompHeader, 2>{
    StompHeader::AcceptVersion,
    StompHeader::Host,
};
constexpr auto kForConnected = std::array<StompHeader, 1>{
    StompHeader::Version,
};
constexpr auto kForSend = std::array<StompHeader, 1>{
    StompHeader::Destination,
};
constexpr auto kForSubscribe = std::array<StompHeader, 2>{
    StompHeader::Destination,
    StompHeader::Id,
};
constexpr auto kForMessage = std::array<StompHeader, 3>{
    StompHeader::Destination,
    StompHeader::MessageId,
    StompHeader::Subscription,
};
constexpr auto kForReceipt = std::array<StompHeader, 1>{
    StompHeader::ReceiptId,
};

constexpr auto kOnlyId = std::array<StompHeader, 1>{
    StompHeader::Id,
};
constexpr auto kOnlyTransaction = std::array<StompHeader, 1>{
    StompHeader::Transaction,
};
constexpr auto kNone = std::array<StompHeader, 0>{};
}  // namespace required_headers

constexpr std::span<const StompHeader> GetHeadersRequiresByCommand(
    StompCommand command) noexcept {
  switch (command) {
    case StompCommand::Connect:
      return required_headers::kForConnect;

    case StompCommand::Connected:
      return required_headers::kForConnected;

    case StompCommand::Send:
      return required_headers::kForSend;

    case StompCommand::Subscribe:
      return required_headers::kForSubscribe;

    case StompCommand::Message:
      return required_headers::kForMessage;

    case StompCommand::Receipt:
      return required_headers::kForReceipt;

    case StompCommand::Unsubscribe:
    case StompCommand::Ack:
    case StompCommand::NAck:
      return required_headers::kOnlyId;

    case StompCommand::Begin:
    case StompCommand::Commit:
    case StompCommand::Abort:
      return required_headers::kOnlyTransaction;

    case StompCommand::Disconnect:
    case StompCommand::Error:
    case StompCommand::Stomp:
    case StompCommand::Invalid:
      return required_headers::kNone;

    default:
      std::unreachable();
  }
}

std::string_view ToStringView(const StompCommand& command) {
  const auto result = kStompCommands.ToStringView(command);
  if (result.has_value()) {
    return result.value();
  }
  std::unreachable();
}

std::string_view ToStringView(const StompHeader& header) {
  const auto result = kStompHeaders.ToStringView(header);
  if (result.has_value()) {
    return result.value();
  }
  std::unreachable();
}

std::string_view ToStringView(const StompError& error) {
  const auto result = kStompErrors.ToStringView(error);
  if (result.has_value()) {
    return result.value();
  }
  std::unreachable();
}

// `current_line` - parsing start point
// Returns:
//   - `StompFrame::Headers` - parsed headers
//   - `std::string_view::size_type` - points at one character after the newline
//                                     character of the last header
auto ParseHeaders(std::string_view content,
                  std::string_view::size_type current_line)
    -> std::expected<
        std::pair<StompFrame::Headers, std::string_view::size_type>,
        StompError> {
  // Headers are optional.
  StompFrame::Headers headers;

  while (true) {
    // Check if there's something more in the frame.
    if (current_line >= content.size()) {
      return std::unexpected(StompError::MissingBodyNewline);
    }

    if (content.at(current_line) == kNewlineCharacter) {
      // CONNECT\n
      // \n
      //
      // No more headers.
      break;
    }
    if (content.at(current_line) == kColonCharacter) {
      // CONNECT\n
      // :
      return std::unexpected(StompError::NoHeaderName);
    }
    if (content.at(current_line) == kNullCharacter) {
      // CONNECT\n
      // \0
      return std::unexpected(StompError::MissingBodyNewline);
    }

    const auto next_colon_position{content.find(kColonCharacter, current_line)};
    const auto next_newline_position{
        content.find(kNewlineCharacter, current_line)};

    if (next_colon_position == std::string::npos) {
      // CONNECT\n
      // header-1:value\n
      // header-2\n
      //       ^ no header value
      return std::unexpected(StompError::NoHeaderValue);
    }
    if (next_newline_position == std::string::npos) {
      // CONNECT\n
      // header:value
      //     <-- missing newline
      // \0
      return std::unexpected(StompError::MissingLastHeaderNewline);
    }
    if (next_newline_position < next_colon_position) {
      // CONNECT\n
      // header-1\n
      //         ^ missing colon
      // header-2:value\n
      // \0
      return std::unexpected(StompError::NoHeaderValue);
    }
    if (content.at(next_colon_position + 1) ==
        content.at(next_newline_position)) {
      // CONNECT\n
      // header:\n
      //       ^ ^ missing header value
      return std::unexpected(StompError::EmptyHeaderValue);
    }

    // CONNECT\n
    // header-1:

    // Read header.
    const auto header_text{
        content.substr(current_line, next_colon_position - current_line)};
    const auto header{kStompHeaders.ToEnum(header_text)};
    if (!header.has_value()) {
      return std::unexpected(StompError::InvalidHeader);
    }

    // Find the value. It's known '\n' is in a further part, so no frame end
    // concerns. Also the value will be no empty.
    const std::string_view value{
        content.substr(next_colon_position + 1,
                       next_newline_position - next_colon_position - 1)};

    headers.emplace(header.value(), value);
    current_line = next_newline_position + 1;
  }

  return std::pair{headers, current_line};
}

}  // namespace

std::ostream& network_monitor::operator<<(std::ostream& os,
                                          const StompCommand& command) {
  os << ToStringView(command);
  return os;
}

std::ostream& network_monitor::operator<<(std::ostream& os,
                                          const StompHeader& header) {
  os << ToStringView(header);
  return os;
}

std::ostream& network_monitor::operator<<(std::ostream& os,
                                          const StompError& error) {
  os << ToStringView(error);
  return os;
}

std::string network_monitor::ToString(const StompCommand& command) {
  return std::string(ToStringView(command));
}

std::string network_monitor::ToString(const StompHeader& header) {
  return std::string(ToStringView(header));
}

std::string network_monitor::ToString(const StompError& error) {
  return std::string(ToStringView(error));
}

StompFrame::StompFrame() : stomp_error_{StompError::Ok} {}

StompFrame::StompFrame(const std::string& content)
    : plain_content_{content},
      stomp_error_{ParseFrame()} {
  if (stomp_error_ != StompError::Ok) {
    return;
  }
  stomp_error_ = ValidateFrame();
}

StompFrame::StompFrame(std::string&& content)
    : plain_content_{std::move(content)},
      stomp_error_{ParseFrame()} {
  if (stomp_error_ != StompError::Ok) {
    return;
  }
  stomp_error_ = ValidateFrame();
}

StompFrame::StompFrame(const StompFrame& other) = default;

StompFrame::StompFrame(StompFrame&& other) noexcept
    : stomp_error_{other.stomp_error_},
      plain_content_{std::move(other.plain_content_)},
      command_{other.command_},
      headers_{std::move(other.headers_)},
      body_{other.body_} {}

StompFrame& StompFrame::operator=(const StompFrame& other) {
  if (this == &other) {
    return *this;
  }

  plain_content_ = other.plain_content_;
  command_ = other.command_;
  headers_ = other.headers_;
  body_ = other.body_;
  return *this;
}

StompFrame& StompFrame::operator=(StompFrame&& other) noexcept {
  plain_content_ = std::move(other.plain_content_);
  command_ = other.command_;
  headers_ = std::move(other.headers_);
  body_ = other.body_;
  return *this;
}

StompError StompFrame::ParseFrame() {
  // Run pre-checks
  const std::string_view plain_content = plain_content_;

  if (plain_content.empty()) {
    return StompError::NoData;
  }

  if (plain_content.at(0) == kNewlineCharacter) {
    return StompError::MissingCommand;
  }

  if (plain_content.back() != kNullCharacter) {
    return StompError::MissingClosingNullCharacter;
  }

  const auto command_end{plain_content.find(kNewlineCharacter)};
  if (command_end == std::string::npos) {
    return StompError::NoNewlineCharacters;
  }

  // If no "\n\n" pattern, then the body's newline is missing.
  const auto* double_newline_pointer = std::ranges::adjacent_find(
      plain_content, [](const auto& lhs, const auto& rhs) {
        return lhs == rhs && lhs == '\n';
      });
  if (double_newline_pointer == plain_content.end()) {
    return StompError::MissingBodyNewline;
  }

  // Parse command
  auto command_text{plain_content.substr(0, command_end)};
  auto command{kStompCommands.ToEnum(command_text)};
  if (!command.has_value()) {
    return StompError::InvalidCommand;
  }
  command_ = command.value();

  // Parse headers
  auto headers = ParseHeaders(plain_content, command_end + 1);
  if (!headers.has_value()) {
    return headers.error();
  }
  headers_ = std::move(headers.value().first);
  auto current_line_position{headers.value().second};

  // Parse body

  // CONNECT\n
  // header-1:value\n
  // \n <-- check if the newline is present (empty line before the body)
  if (plain_content.at(current_line_position) != kNewlineCharacter) {
    return StompError::MissingBodyNewline;
  }
  current_line_position++;

  if (current_line_position >= plain_content.size()) {
    // CONNECT\n
    // \n
    //     <-- missing null
    return StompError::MissingClosingNullCharacter;
  }
  const auto null_position{plain_content.find(kNullCharacter)};
  if (null_position == std::string::npos) {
    // CONNECT\n
    // \n
    // Frame body
    //            ^ missing null
    return StompError::MissingClosingNullCharacter;
  }

  if (HasHeader(StompHeader::ContentLength)) {
    // -2 excludes the closing null character.
    body_ =
        plain_content.substr(current_line_position,
                             plain_content.size() - current_line_position - 1);
  } else {
    if (null_position + 1 != plain_content.size()) {
      // CONNECT\n
      // \n
      // Frame body\0junk
      //             ^ unexpected characters
      return StompError::JunkAfterBody;
    }
    body_ = plain_content.substr(current_line_position,
                                 null_position - current_line_position);
  }

  return StompError::Ok;
}

StompError StompFrame::ValidateFrame() {
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

  // Check for required headers.
  const auto& required_headers = GetHeadersRequiresByCommand(command_);
  for (const auto required_header : required_headers) {
    if (!HasHeader(required_header)) {
      return StompError::MissingRequiredHeader;
    }
  }

  return StompError::Ok;
}

StompError StompFrame::GetStompError() const {
  return stomp_error_;
}

StompCommand StompFrame::GetCommand() const {
  return command_;
}

bool StompFrame::HasHeader(const StompHeader& header) const {
  return headers_.contains(header);
}

const std::string_view& StompFrame::GetHeaderValue(
    const StompHeader& header) const {
  static const std::string_view empty_header_value{};
  if (HasHeader(header)) {
    return headers_.at(header);
  }
  return empty_header_value;
}

const StompFrame::Headers& StompFrame::GetAllHeaders() const {
  return headers_;
}

const std::string_view& StompFrame::GetBody() const {
  return body_;
}

std::string StompFrame::ToString() const {
  std::ostringstream stream;
  stream << GetCommand() << "\n";
  for (const auto& [header, value] : GetAllHeaders()) {
    stream << header << ":" << value << "\n";
  }
  stream << "\n" << GetBody();
  stream.put('\0');

  return stream.str();
}
