#include "utils.hpp"

#include <sys/mman.h>

#include <filesystem>
#include <fstream>

#include "json.hpp"
#include "gtest/gtest.h"
#include "gmock/gmock.h"

namespace perma {

using namespace testing;
namespace fs = std::filesystem;

constexpr auto FILE_SIZE = 8388608u;  // 8 MiB

class UtilsTest : public ::testing::Test {
 protected:
  void SetUp() override {
    tmp_file_name_create = std::tmpnam(nullptr);
    std::ofstream temp_stream_create{tmp_file_name_create};
    temp_stream_create.close();

    tmp_file_name_read = std::tmpnam(nullptr);
    std::ofstream temp_stream_read{tmp_file_name_read};
    temp_stream_read.close();
    fs::resize_file(tmp_file_name_read, FILE_SIZE);

    internal::setPMEM_MAP_FLAGS(MAP_SHARED);
  }

  std::filesystem::path tmp_file_name_create;
  std::filesystem::path tmp_file_name_read;
};

/**
 * Verifies whether the first 100'000 zipfian generated values are in between the given boundaries.
 */
TEST_F(UtilsTest, ZipfBound) {
  for (uint32_t i = 0; i < 100'000; i++) {
    const uint64_t value = zipf(0.99, 1000);
    EXPECT_GE(value, 0);
    EXPECT_LT(value, 1000);
  }
}

/**
 * Verifies whether the memory mapped file is the same size as the file.
 */
TEST_F(UtilsTest, ReadMapFileCorrectSizePmem) { EXPECT_NO_THROW(map_file(tmp_file_name_read, false, FILE_SIZE)); }

/**
 * Verifies whether the memory mapped file can be mapped without a backing file.
 */
TEST_F(UtilsTest, ReadMapFileCorrectSizeDRAM) { EXPECT_NO_THROW(map_file(tmp_file_name_read, true, FILE_SIZE)); }

/**
 * Verifies whether the created memory mapped file is of the correct size.
 */
TEST_F(UtilsTest, CreateMapFileSize) {
  ASSERT_NO_THROW(create_file(tmp_file_name_create, false, FILE_SIZE));
  size_t file_size = fs::file_size(tmp_file_name_create);
  EXPECT_EQ(file_size, FILE_SIZE);
}

TEST_F(UtilsTest, CreateResultFileFromConfigFile) {
  const std::filesystem::path config_path = fs::temp_directory_path() / "test.yaml";
  std::ofstream config_file(config_path);

  const fs::path result_dir = fs::temp_directory_path();
  const fs::path result_file = create_result_file(result_dir, config_path);
  ASSERT_TRUE(fs::is_regular_file(result_file));
  EXPECT_THAT(result_file.filename(), StartsWith("test-results-"));
  EXPECT_EQ(result_file.extension().string(), ".json");

  nlohmann::json content;
  std::ifstream results(result_file);
  results >> content;
  EXPECT_TRUE(content.is_array());

  fs::remove(result_file);
  fs::remove(config_path);
}

TEST_F(UtilsTest, CreateResultFileFromConfigDir) {
  const std::filesystem::path config_path = fs::temp_directory_path() / "test-configs";
  fs::create_directories(config_path);

  const fs::path result_dir = fs::temp_directory_path();
  const fs::path result_file = create_result_file(result_dir, config_path);
  ASSERT_TRUE(fs::is_regular_file(result_file));
  EXPECT_THAT(result_file.filename(), StartsWith("test-configs-results-"));
  EXPECT_EQ(result_file.extension().string(), ".json");

  nlohmann::json content;
  std::ifstream results(result_file);
  results >> content;
  EXPECT_TRUE(content.is_array());

  fs::remove(result_file);
  fs::remove(config_path);
}

TEST_F(UtilsTest, AddToResultFile) {
  const std::filesystem::path config_path = fs::temp_directory_path() / "test.yaml";
  std::ofstream config_file(config_path);

  const fs::path result_dir = fs::temp_directory_path();
  const fs::path result_file = create_result_file(result_dir, config_path);
  ASSERT_TRUE(fs::is_regular_file(result_file));

  nlohmann::json result1;
  result1["test"] = true;
  write_benchmark_results(result_file, result1);

  std::ifstream results1(result_file);
  nlohmann::json content1;
  results1 >> content1;
  EXPECT_TRUE(content1.is_array());
  ASSERT_EQ(content1.size(), 1);
  EXPECT_EQ(content1[0].at("test"), true);
  results1.close();

  nlohmann::json result2;
  result2["foo"] = "bar";
  write_benchmark_results(result_file, result2);

  std::ifstream results2(result_file);
  nlohmann::json content2;
  results2 >> content2;
  EXPECT_TRUE(content2.is_array());
  ASSERT_EQ(content2.size(), 2);
  EXPECT_EQ(content2[0].at("test"), true);
  ASSERT_EQ(content2[1].at("foo"), "bar");
  results2.close();

  fs::remove(result_file);
  fs::remove(config_path);
}

}  // namespace perma
