#include "file-downloader.hpp"

#include <curl/curl.h>
#include <fstream>

bool network_monitor::download_file(const std::string& file_url,
                                    const std::filesystem::path& destination,
                                    const std::filesystem::path& cacert_file)
{
    auto curl{curl_easy_init()};
    if (curl == nullptr) {
        return false;
    }

    curl_easy_setopt(curl, CURLOPT_URL, file_url.c_str());
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_CAINFO, cacert_file.c_str());
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);

    std::FILE* file_handler{fopen(destination.c_str(), "wb")};
    if (file_handler == nullptr) {
        curl_easy_cleanup(curl);
        return false;
    }
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, file_handler);

    auto result = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    fclose(file_handler);

    return true;
}

nlohmann::json network_monitor::parse_json_file(const std::filesystem::path& source)
{
    std::ifstream ifs(source);
    return nlohmann::json::parse(ifs);
}
