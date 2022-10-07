#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>
#include <iostream>

int main()
{
    boost::system::error_code error_code{};
    if (error_code)
    {
        std::cout << "Error: " << error_code.message() << std::endl;
        return -1;
    }
    else
    {
        std::cout << "Ok" << std::endl;
        return 0;
    }
}