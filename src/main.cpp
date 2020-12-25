#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/system/error_code.hpp>
#include <iomanip>
#include <iostream>
#include <thread>
#include <vector>

void log(boost::system::error_code error_code)
{
    std::cerr << "[" << std::hex << std::setw(12) << std::this_thread::get_id() << "] "
              << (error_code ? "Error: " : "OK")
              << (error_code ? error_code.message() : "") << "\n";
}

void on_connect(boost::system::error_code error_code)
{
    log(error_code);
}

int main()
{
    using tcp = boost::asio::ip::tcp;

    std::cerr << "[" << std::hex << std::setw(12) << std::this_thread::get_id()
              << "] main\n";

    boost::asio::io_context io_context;
    boost::system::error_code error_code;

    tcp::resolver resolver(io_context);
    auto endpoint{resolver.resolve("echo.websocket.org", "80", error_code)};
    if (error_code) {
        log(error_code);
        return -1;
    }

    tcp::socket socket(boost::asio::make_strand(io_context));
    socket.connect(*endpoint, error_code);
    if (error_code) {
        log(error_code);
        return -1;
    }

    boost::beast::websocket::stream<boost::beast::tcp_stream> websocket(
        std::move(socket));

    //    boost::beast::get_lowest_layer(websocket).connect(
    //        resolver.resolve("echo.websocket.org", "80", error_code));

    boost::beast::websocket::response_type response;
    websocket.handshake(response, "echo.websocket.org", "/", error_code);
    if (error_code) {
        log(error_code);
        return -1;
    }
    std::cout << "HANDSHAKE RESPONSE:\n" << response << "\n";
    std::cout << "----------------------------------\n";

    websocket.text(true);

    std::string message("Hello, world!");
    boost::asio::const_buffer write_buffer(message.data(), message.size());
    websocket.write(write_buffer, error_code);
    if (error_code) {
        log(error_code);
        return -1;
    }

    boost::beast::flat_buffer read_buffer;
    websocket.read(read_buffer, error_code);
    if (error_code) {
        log(error_code);
        return -1;
    }
    std::cout << "ECHO:\n" << boost::beast::buffers_to_string(read_buffer.data()) << "\n";
    read_buffer.consume(read_buffer.size());
}
