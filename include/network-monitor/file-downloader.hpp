#pragma once

#include <filesystem>
#include <string>
#include <nlohmann/json.hpp>

namespace network_monitor {

bool download_file(const std::string& file_url,
                   const std::filesystem::path& destination,
                   const std::filesystem::path& cacert_file = {});

nlohmann::json parse_json_file(const std::filesystem::path& source);

}
