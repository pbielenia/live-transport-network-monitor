#include <curl/curl.h>

#include <cstdio>
#include <fstream>
#include <network-monitor/file-downloader.hpp>

bool network_monitor::DownloadFile(const std::string& file_url,
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

  auto* file_handler = std::fopen(destination.c_str(), "w");
  if (file_handler == nullptr) {
    curl_easy_cleanup(curl);
    return false;
  }
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, file_handler);

  curl_easy_setopt(curl, CURLOPT_URL, file_url.c_str());

  const auto result = curl_easy_perform(curl);
  curl_easy_cleanup(curl);
  std::fclose(file_handler);

  if (result != CURLE_OK) {
    return false;
  }
  return true;
}

nlohmann::json network_monitor::ParseJsonFile(const std::filesystem::path& source) {
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
