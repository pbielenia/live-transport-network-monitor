#include <boost/asio.hpp>
#include <boost/test/unit_test.hpp>
#include <filesystem>
#include <fstream>
#include <network-monitor/file-downloader.hpp>
#include <string>

BOOST_AUTO_TEST_SUITE(network_monitor);

BOOST_AUTO_TEST_CASE(file_downloader)
{
    const std::string file_url{
        "https://ltnm.learncppthroughprojects.com/network-layout.json"};
    const auto destination{std::filesystem::temp_directory_path()
                           / "network-layout.json"};
    bool downloaded{
        network_monitor::download_file(file_url, destination, TESTS_CACERT_PEM)};
    BOOST_CHECK(downloaded);
    BOOST_CHECK(std::filesystem::exists(destination));

    const std::string expected_string{"\"stations\": ["};
    std::ifstream file{destination};
    std::string line{};
    bool found_expected_string{false};
    while (std::getline(file, line)) {
        if (line.find(expected_string) != std::string::npos) {
            found_expected_string = true;
            break;
        }
    }
    BOOST_CHECK(found_expected_string);

    std::filesystem::remove(destination);
}

BOOST_AUTO_TEST_SUITE_END();
