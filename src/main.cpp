#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/system/error_code.hpp>

#include <iomanip>
#include <iostream>
#include <thread>

void log(boost::system::error_code error_code)
{
    std::cerr << "* Error: " << error_code.message() << std::endl;
}

int main()
{
    using tcp = boost::asio::ip::tcp;

    boost::asio::io_context io_context{};

    boost::system::error_code error_code{};

    // 1. Reolve the address
    const std::string server_address{"ltnm.learncppthroughprojects.com"};
    const std::string server_port{"80"};
    const std::string server_target{"/echo"};
    tcp::resolver resolver{io_context};
    const auto resolver_results{resolver.resolve(server_address, server_port, error_code)};
    if (error_code)
    {
        log(error_code);
        return -1;
    }
    std::cout << "* Resolved the address" << std::endl;

    // 2. Create a TCP connection to the resolved host through a socket object
    tcp::socket tcp_socket{io_context};
    tcp_socket.connect(*resolver_results, error_code);
    if (error_code)
    {
        log(error_code);
        return -2;
    }
    std::cout << "* Connected" << std::endl;

    // 3. Use the socket object to construct a websocket::stream class
    boost::beast::websocket::stream<boost::beast::tcp_stream> websocket_stream{std::move(tcp_socket)};

    // 4. Use the websocket::stream instance to perform a WebSocket handshake
    const std::string server_host{server_address + ':' + server_port};
    websocket_stream.handshake(server_host, server_target, error_code);
    if (error_code)
    {
        log(error_code);
        return -3;
    }
    std::cout << "* Handshake succeded" << std::endl;

    // 5. Send a message to the connected WebSocket server and receive an echo message
    //    back using the websocket::stream::read and write APIs
    const std::string message_content{"Dzien dobry"};
    boost::asio::const_buffer message{message_content.c_str(), message_content.size()};
    websocket_stream.write(message, error_code);
    if (error_code)
    {
        log(error_code);
        return -4;
    }
    std::cout << "* An empty message has been sent" << std::endl;

    boost::beast::flat_buffer response{};
    websocket_stream.read(response, error_code);
    if (error_code)
    {
        log(error_code);
        return -5;
    }
    std::cout << "* Received the message:\n"
              << boost::beast::make_printable(response.data()) << std::endl;
}
