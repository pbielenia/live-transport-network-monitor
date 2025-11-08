#include "network-monitor/file-downloader.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

#include <boost/test/unit_test.hpp>

using network_monitor::DownloadFile;
using network_monitor::ParseJsonFile;

BOOST_AUTO_TEST_SUITE(network_monitor);

BOOST_AUTO_TEST_CASE(file_downloader) {
  const std::string file_url{
      "https://ltnm.learncppthroughprojects.com/network-layout.json"};
  const auto destination{std::filesystem::temp_directory_path() /
                         "network-layout.json"};

  // Download the file.
  bool downloaded{DownloadFile(file_url, destination, TESTS_CACERT_PEM)};
  BOOST_CHECK(downloaded);
  BOOST_CHECK(std::filesystem::exists(destination));

  // Check the content of the file.
  // We cannot check the whole file content as it changes over time, but we
  // can at least check some expected file properties.
  {
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
  }

  // Clean up.
  std::filesystem::remove(destination);
}

// TODO: group details test cases
// TODO: group StreamSizeIsSafeReturn test cases

BOOST_AUTO_TEST_CASE(StreamSizeIsSafeReturnTrueOnSafeInput) {
  constexpr auto streamsize_max =
      static_cast<size_t>(std::numeric_limits<std::streamsize>::max());
  BOOST_TEST(details::StreamSizeIsSafe(streamsize_max / 2) == true);
}

BOOST_AUTO_TEST_CASE(StreamSizeIsSafeReturnTrueOnMaxLimitInput) {
  constexpr auto streamsize_max =
      static_cast<size_t>(std::numeric_limits<std::streamsize>::max());
  BOOST_TEST(details::StreamSizeIsSafe(streamsize_max) == true);
}

BOOST_AUTO_TEST_CASE(StreamSizeIsSafeReturnTrueOnOverflowInput) {
  constexpr auto streamsize_max =
      static_cast<size_t>(std::numeric_limits<std::streamsize>::max());
  constexpr auto streamsize_overflow = streamsize_max + 10;
  BOOST_TEST(details::StreamSizeIsSafe(streamsize_overflow) == false);
}

BOOST_AUTO_TEST_CASE(WriteFunctionCallback) {
  const std::string test_input{"test input"};
  constexpr size_t kInputSize = 1;
  std::ostringstream stream;

  const auto result = details::WriteFunctionCallback(
      (void*)(test_input.data()), kInputSize, test_input.size(), &stream);

  // TODO: check if `test_input` was not modified
  BOOST_CHECK(test_input == stream.str());
  BOOST_CHECK(result == test_input.size());
}

BOOST_AUTO_TEST_CASE(parse_json_file) {
  const auto parsed_json = ParseJsonFile(TESTS_NETWORK_LAYOUT_JSON);

  const std::string lines_keyword{"lines"};
  const std::string stations_keyword{"stations"};
  const std::string travel_times_keyword{"travel_times"};

  BOOST_CHECK(parsed_json.is_object());

  BOOST_CHECK(parsed_json.contains(lines_keyword));
  BOOST_CHECK(parsed_json.at(lines_keyword).is_array());
  BOOST_CHECK(!parsed_json.at(lines_keyword).empty());

  BOOST_CHECK(parsed_json.contains(stations_keyword));
  BOOST_CHECK(parsed_json.at(stations_keyword).is_array());
  BOOST_CHECK(!parsed_json.at(stations_keyword).empty());

  BOOST_CHECK(parsed_json.contains(travel_times_keyword));
  BOOST_CHECK(parsed_json.at(travel_times_keyword).is_array());
  BOOST_CHECK(!parsed_json.at(travel_times_keyword).empty());
}

BOOST_AUTO_TEST_SUITE_END();
