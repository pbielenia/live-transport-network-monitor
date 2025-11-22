#include "network-monitor/file-downloader.hpp"

#include <cstddef>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <ios>
#include <limits>
#include <string>

#include <curl/curl.h>
#include <nlohmann/json.hpp>

namespace network_monitor {

bool details::StreamSizeIsSafe(size_t stream_size) {
  constexpr auto streamsize_max =
      static_cast<size_t>(std::numeric_limits<std::streamsize>::max());
  return (stream_size <= streamsize_max);
}

size_t details::WriteFunctionCallback(void* data,
                                      size_t size,
                                      size_t real_size,
                                      void* userdata) {
  if (size == 0) {
    return 0;
  }

  const auto total_size = size * real_size;
  // TODO: split into multiple writes if is not safe
  if (!StreamSizeIsSafe(total_size)) {
    return 0;
  }

  auto* ofstream = static_cast<std::ofstream*>(userdata);
  ofstream->write(static_cast<char*>(data),
                  static_cast<std::streamsize>(total_size));

  return total_size;
}

bool DownloadFile(const std::string& file_url,
                  const std::filesystem::path& destination,
                  const std::filesystem::path& ca_cert_file) {
  curl_global_init(CURL_GLOBAL_DEFAULT);

  auto* curl = curl_easy_init();
  if (curl == nullptr) {
    return false;
  }

  if (!ca_cert_file.empty()) {
    curl_easy_setopt(curl, CURLOPT_CAINFO, ca_cert_file.c_str());
  }

  std::ofstream destination_stream(destination);
  if (!destination_stream.is_open()) {
    curl_easy_cleanup(curl);
    return false;
  }

  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &destination_stream);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
                   &details::WriteFunctionCallback);
  curl_easy_setopt(curl, CURLOPT_URL, file_url.c_str());

  const auto result = curl_easy_perform(curl);

  curl_easy_cleanup(curl);
  destination_stream.close();

  return result == CURLE_OK;
}

nlohmann::json ParseJsonFile(const std::filesystem::path& source) {
  auto parsed = nlohmann::json::object();
  if (!std::filesystem::exists(source)) {
    return parsed;
  }
  try {
    std::ifstream file{source};
    file >> parsed;
  } catch (...) {
  }
  return parsed;
}

}  // namespace network_monitor
