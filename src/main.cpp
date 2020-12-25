#include <boost/asio.hpp>
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
    tcp::socket socket(boost::asio::make_strand(io_context));
    boost::system::error_code error_code;

    tcp::resolver resolver(io_context);
    auto endpoint{resolver.resolve("google.com", "80", error_code)};
    if (error_code) {
        log(error_code);
        return -1;
    }

    size_t number_of_threads{4};
    for (size_t index{0}; index < number_of_threads; ++index) {
        socket.async_connect(*endpoint, on_connect);
    }

    std::vector<std::thread> threads;
    threads.reserve(number_of_threads);
    for (size_t index{0}; index < number_of_threads; ++index) {
        threads.emplace_back([&io_context]() { io_context.run(); });
    }

    for (auto& thread : threads) {
        thread.join();
    }
}
