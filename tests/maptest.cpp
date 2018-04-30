#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <gtest/gtest.h>
#include "map/mapload.h"
#include <filesystem>
#include <range/v3/all.hpp>

namespace fs = std::filesystem;

TEST(MapTest, InitPhysFS)
{
  EXPECT_EQ(PHYSFS_init(nullptr), 1);
  std::string datapath = fs::current_path().generic_string();
  EXPECT_EQ(PHYSFS_mount(datapath.c_str(), NULL, 1), 1);
  PHYSFS_deinit();
}

auto get_map_directories = [](const auto& datapath) {
  return fs::recursive_directory_iterator(datapath) |
    ranges::view::transform([](auto&& e) {return e.path(); }) |
    ranges::view::filter([](auto&& p) { return p.extension() == fs::path(".map"); }) |
    ranges::view::transform([](auto&& e) { return e.parent_path(); });
};

TEST(MapTest, MapFounds)
{
  auto map_files = get_map_directories(fs::current_path());
  int n = 0;
  for (const auto& i : map_files) ++n;
  EXPECT_NE(n, 0);
}

TEST(MapTest, LoadMaps)
{
  EXPECT_EQ(PHYSFS_init(nullptr), 1);
  auto datapath = fs::current_path();
  EXPECT_EQ(PHYSFS_mount(datapath.generic_string().c_str(), NULL, 1), 1);

  auto map_files = get_map_directories(datapath);

  for (const auto& file : map_files)
  {
    auto path = fs::relative(file, datapath).generic_string();
    GAMEMAP* map;
    EXPECT_NO_FATAL_FAILURE(map = mapLoad(const_cast<char*>(path.c_str()))) << "Failed to load " << path;
    EXPECT_NE(map, nullptr) << "Map " << path << " is nullptr";
    mapFree(map);
  }
}
