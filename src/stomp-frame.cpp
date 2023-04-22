#include <network-monitor/stomp-frame.hpp>

using namespace NetworkMonitor;

std::ostream& operator<<(std::ostream& os, const StompCommand& command)
{
    //
}

std::string ToString(const StompCommand& command)
{
    //
}

std::ostream& operator<<(std::ostream& os, const StompFrame& command)
{
    //
}

std::string ToString(const StompFrame& frame)
{
    //
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
}

StompFrame& StompFrame::operator=(StompFrame&& other)
{
    //
}
