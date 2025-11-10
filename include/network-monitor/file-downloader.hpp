#pragma once

#include <filesystem>
#include <string>

#include <nlohmann/json.hpp>

namespace network_monitor {

namespace details {

// Used to receive data from the underlying implementation.
size_t WriteFunctionCallback(void* data,
                             size_t size,
                             size_t real_size,
                             void* userdata);

// Returns true if `source_size` (aka unsigned long) is not greater than
// `std::streamsize` (aka long) maximum value, and so casting it will not result
// in overflow.
bool StreamSizeIsSafe(size_t source_size);

}  // namespace details

/*! \brief Download a file from a remote HTTPS URL.
 *
 *  \param destination   The full path and filename of the output file. The path
 *                       to the file must exist.
 *  \param ca_cert_file  The path to a cacert.pem file to perform certificate
 *                       verification in an HTTPS connection.
 */
bool DownloadFile(const std::string& file_url,
                  const std::filesystem::path& destination,
                  const std::filesystem::path& ca_cert_file = {});

/*! \brief Parse a local file into a JSON object.
 *
 *  \param source The path to the JSON file to load and parse.
 */
nlohmann::json ParseJsonFile(const std::filesystem::path& source);

}  // namespace network_monitor
