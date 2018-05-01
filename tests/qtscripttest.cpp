#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <QtCore/QCoreApplication>
#include "lint.h"
#include <filesystem>
#include <range/v3/all.hpp>
#include <gtest/gtest.h>

namespace fs = std::filesystem;

auto get_js_files = [](const auto& datapath) {
  return fs::recursive_directory_iterator(datapath) |
    ranges::view::transform([](auto&& e) {return e.path(); }) |
    ranges::view::filter([](auto&& p) { return p.extension() == fs::path(".js"); });
};

TEST(jsTest, FoundJS)
{
  int n = 0;
  for (const auto& file : get_js_files(fs::current_path())) n++;
  EXPECT_NE(n, 0) << "JS files not found";
}

TEST(jsTest, loadJS)
{
  int argc = 0;
  char* argv = "";
  QCoreApplication app(argc, &argv);
  for (const auto& file : get_js_files(fs::current_path()))
  {
    testGlobalScript(QString::fromStdString(file.generic_string()));
  }
}
