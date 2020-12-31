#pragma once

#include <filesystem>
#include <string>

namespace network_monitor {

bool download_file(const std::string& file_url,
                   const std::filesystem::path& destination,
                   const std::filesystem::path& cacert_file = {});

}
