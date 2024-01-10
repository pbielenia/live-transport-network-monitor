#pragma once

#include <string>
#include <string_view>
#include <unordered_map>

namespace NetworkMonitor {

/*! \brief Available STOMP commands, from the STOMP protocol v1.2.
 */
enum class StompCommand {
    Abort,
    Ack,
    Begin,
    Commit,
    Connect,
    Connected,
    Disconnect,
    Error,
    Invalid,
    Message,
    NAck,
    Receipt,
    Send,
    Stomp,
    Subscribe,
    Unsubscribe,
};

/*! \brief Print operator for the `StompCommand` class.
 */
std::ostream& operator<<(std::ostream& os, const StompCommand& command);

/*! \brief Convert `StompCommand` to string.
 */
std::string ToString(const StompCommand& command);

/*! \brief Available STOMP headers, from the STOMP protocol v1.2.
 */
enum class StompHeader {
    AcceptVersion,
    Ack,
    ContentLength,
    ContentType,
    Destination,
    HeartBeat,
    Host,
    Id,
    Invalid,
    Login,
    Message,
    MessageId,
    Passcode,
    Receipt,
    ReceiptId,
    Session,
    Server,
    Subscription,
    Transaction,
    Version,
};

/*! \brief Print operator for the `StompHeader` class.
 */
std::ostream& operator<<(std::ostream& os, const StompHeader& header);

/*! \brief Convert `StompHeader` to string.
 */
std::string ToString(const StompHeader& header);

/*! \brief Error codes for the STOMP protocol
 */
enum class StompError {
    Ok = 0,
    UndefinedError,
    InvalidCommand,
    InvalidHeader,
    InvalidHeaderValue,
    NoHeaderValue,
    EmptyHeaderValue,
    NoNewlineCharacters,
    MissingLastHeaderNewline,
    MissingBodyNewline,
    MissingClosingNullCharacter,
    JunkAfterBody,
    ContentLengthsDontMatch,
    MissingRequiredHeader,
    NoData,
    MissingCommand,
    NoHeaderName
};

/*! \brief Print operator for the `StompError` class.
 */
std::ostream& operator<<(std::ostream& os, const StompError& error);

/*! \brief Convert `StompError` to string.
 */
std::string ToString(const StompError& error);

/* \brief STOMP frame representation, supporting STOMP v1.2.
 */
class StompFrame {
  public:
    using Headers = std::unordered_map<StompHeader, std::string_view>;

    /*! \brief Default constructor. Corresponds to an empty, invalid STOMP frame.
     */
    StompFrame();

    /*! \brief Construct the STOMP frame from a string. The string is copied.
     *
     *  The result of the operation is stored in the error code.
     *
     *  If any `error_code` other than `StompError:Ok` is set, none of the frame lookup
     *  methods should be used as they may store invalid values.
     */
    StompFrame(StompError& error_code, const std::string& content);

    /*! \brief Construct the STOMP frame from a string. The string is moved into
     *         the object.
     *
     *  The result of the operation is stored in the error code.
     */
    StompFrame(StompError& error_code, std::string&& content);

    /*! \brief Copy constructor.
     */
    StompFrame(const StompFrame& other);

    /*! \brief Move constructor.
     */
    StompFrame(StompFrame&& other);

    /*! \brief Copy assignment operator.
     */
    StompFrame& operator=(const StompFrame& other);

    /*! \brief Move assignment operator.
     */
    StompFrame& operator=(StompFrame&& other);

    /*! \brief Get the STOMP command.
     */
    StompCommand GetCommand() const;

    /*! \brief Check if the frame has a specified header.
     */
    const bool HasHeader(const StompHeader& header) const;

    /*! \brief Get the value for the specified header.
     *
     *  \returns An empty std::string_view if the header is not in the frame.
     */
    const std::string_view& GetHeaderValue(const StompHeader& header) const;

    /*! \brief Get the values of all contained headers.
     */
    const Headers& GetAllHeaders() const;

    /*! \brief Get the frame body.
     */
    const std::string_view& GetBody() const;

    /*! \brief Get a text representation of the frame.
     */
    std::string ToString() const;

   private:
    StompError ParseFrame();
    StompError ValidateFrame();

    std::string plain_content_{};
    StompCommand command_{StompCommand::Invalid};
    Headers headers_{};
    std::string_view body_{};
};

}  // namespace NetworkMonitor
