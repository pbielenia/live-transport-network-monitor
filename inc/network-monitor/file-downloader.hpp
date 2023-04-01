#include <filesystem>
#include <nlohmann/json.hpp>
#include <string>

namespace NetworkMonitor {

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

}  // namespace NetworkMonitor
