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

BOOST_AUTO_TEST_CASE(json_file_parser)
{
    const auto parsed_json = network_monitor::parse_json_file(TESTS_NETWORK_LAYOUT_PATH);

    BOOST_CHECK(parsed_json.contains("lines"));
    BOOST_CHECK(parsed_json.at("lines").is_array());
    BOOST_CHECK(parsed_json.at("lines").empty() == false);

    BOOST_CHECK(parsed_json.contains("stations"));
    BOOST_CHECK(parsed_json.at("stations").is_array());
    BOOST_CHECK(parsed_json.at("stations").empty() == false);

    BOOST_CHECK(parsed_json.contains("travel_times"));
    BOOST_CHECK(parsed_json.at("travel_times").is_array());
    BOOST_CHECK(parsed_json.at("travel_times").empty() == false);
}

BOOST_AUTO_TEST_SUITE_END();
