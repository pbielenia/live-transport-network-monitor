#include "formatters.hpp"

namespace network_monitor {

std::ostream& operator<<(std::ostream& os, const StompFrame::Headers& headers) {
  if (headers.empty()) {
    os << "[]";
    return os;
  }

  os << "[ ";
  bool first = true;
  for (const auto& [header, value] : headers) {
    if (!first) {
      os << ", ";
    }
    first = false;
    os << R"({ ")" << ToString(header) << R"(": ")" << value << R"(" })";
  }
  os << " ]";

  return os;
}

}  // namespace network_monitor
