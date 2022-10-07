#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/system/error_code.hpp>

#include <iomanip>
#include <iostream>
#include <thread>

using tcp = boost::asio::ip::tcp;
using WebsocketStream = boost::beast::websocket::stream<boost::beast::tcp_stream>;

void log(boost::system::error_code error_code)
{
    std::cerr << "* Error: " << error_code.message() << std::endl;
}

void OnMessageReceived(const boost::system::error_code &error,
                       boost::beast::flat_buffer &response)
{
    if (error)
    {
        log(error);
        return;
    }
    std::cout << "* Received the message:\n"
              << boost::beast::make_printable(response.data()) << std::endl;
}

void OnMessageSent(const boost::system::error_code &error,
                   WebsocketStream &websocket_stream,
                   boost::beast::flat_buffer &response)
{
    if (error)
    {
        log(error);
        return;
    }
    std::cout << "* An empty message has been sent" << std::endl;

    websocket_stream.async_read(response, [&response](const boost::system::error_code &error, const size_t)
                                { OnMessageReceived(error, response); });
}

void OnHandshakeCompleted(const boost::system::error_code &error,
                          WebsocketStream &websocket_stream,
                          const std::string &message_content,
                          boost::beast::flat_buffer &response)
{
    if (error)
    {
        log(error);
        return;
    }
    std::cout << "* Handshake succeded" << std::endl;

    boost::asio::const_buffer message{message_content.c_str(), message_content.size()};
    websocket_stream.async_write(message, [&websocket_stream, &response](const boost::system::error_code &error, const size_t)
                                 { OnMessageSent(error, websocket_stream, response); });
}

void OnTcpSocketConnected(const boost::system::error_code &error,
                          WebsocketStream &websocket_stream,
                          const std::string &server_address,
                          const std::string &server_port,
                          const std::string &connection_target,
                          const std::string &message_content,
                          boost::beast::flat_buffer &response)
{
    if (error)
    {
        log(error);
        return;
    }
    std::cout << "* Connected" << std::endl;

    const std::string server_host{server_address + ':' + server_port};
    websocket_stream.async_handshake(server_host, connection_target,
                                     [&websocket_stream, &message_content, &response](
                                         const boost::system::error_code &error)
                                     {
                                         OnHandshakeCompleted(error, websocket_stream,
                                                              message_content, response);
                                     });
}

void OnServerAddressResolved(const boost::system::error_code &error,
                             tcp::resolver::results_type results,
                             WebsocketStream &websocket_stream,
                             const std::string &server_address,
                             const std::string &server_port,
                             const std::string &connection_target,
                             const std::string &message_content,
                             boost::beast::flat_buffer &response)
{
    if (error)
    {
        log(error);
        return;
    }
    std::cout << "* Resolved the address" << std::endl;

    websocket_stream.next_layer().async_connect(*results,
                                                [&websocket_stream, &server_address,
                                                 &server_port, &connection_target,
                                                 &message_content, &response](
                                                    const boost::system::error_code &error)
                                                {
                                                    OnTcpSocketConnected(error,
                                                                         websocket_stream,
                                                                         server_address,
                                                                         server_port,
                                                                         connection_target,
                                                                         message_content,
                                                                         response);
                                                });
}

int main()
{

    boost::asio::io_context io_context{};
    boost::system::error_code error_code{};

    const std::string server_address{"ltnm.learncppthroughprojects.com"};
    const std::string server_port{"80"};
    const std::string connection_target{"/echo"};
    const std::string message_content{"Dobry wieczor"};
    boost::beast::flat_buffer response{};

    tcp::resolver resolver{io_context};
    tcp::socket tcp_socket{io_context};
    boost::beast::websocket::stream<boost::beast::tcp_stream> websocket_stream{std::move(tcp_socket)};

    resolver.async_resolve(server_address, server_port,
                           [&websocket_stream, &server_address, &server_port,
                            &connection_target, &message_content, &response](
                               const boost::system::error_code &error,
                               tcp::resolver::results_type results)
                           {
                               OnServerAddressResolved(error, results, websocket_stream,
                                                       server_address, server_port,
                                                       connection_target, message_content,
                                                       response);
                           });
    io_context.run();
}
