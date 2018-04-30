/*
	This file is part of Warzone 2100.
	Copyright (C) 2007-2017  Warzone 2100 Project

	Warzone 2100 is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100; if not, write to the Free Software
	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
*/

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <errno.h>
#include <stdint.h>
#include <stdbool.h>
#include <gtest/gtest.h>
#include <array>
#include "../3rdparty/glm/glm.hpp"
#include <filesystem>
#include <range/v3/all.hpp>
#include <fstream>

namespace fs = std::filesystem;

constexpr auto iV_IMD_TEX = 0x00000200;
constexpr auto iV_IMD_XNOCUL = 0x00002000;
constexpr auto iV_IMD_TEXANIM = 0x00004000;
constexpr auto iV_IMD_TCMASK = 0x00010000;
constexpr auto MAX_POLYGON_SIZE = 3u; // the game can't handle more

struct WZ_FACE {
	std::array<int, MAX_POLYGON_SIZE> index;
	std::array<glm::ivec2, MAX_POLYGON_SIZE> texCoord;
	int vertices;
	int frames, rate, width, height; // animation data
	bool cull;
};

struct WZ_POSITION {
	int x, y, z, reindex;
	bool dupe;
};

static void check_pie(const fs::path& input)
{
  int num, x, y, z, levels, level;
  std::ifstream fp(input);
  std::string s;
  std::string keyword;

  EXPECT_TRUE(fp >> keyword >> x);
  EXPECT_EQ(keyword, "PIE") << "Bad PIE file";
  EXPECT_FALSE(x != 2 && x != 3) << "Unknown PIE version";

  EXPECT_TRUE(fp >> keyword >> std::hex >> x);
  EXPECT_EQ(keyword, "TYPE") << "Bad TYPE directive";
  EXPECT_FALSE(!(x & iV_IMD_TEX)) << "Missing texture flag";

  EXPECT_TRUE(fp >> keyword >> std::hex >> z >> s >> x >> y);
  EXPECT_EQ(keyword, "TEXTURE") << "Bad TEXTURE directive";

  // Either event or level
  EXPECT_TRUE(fp >> keyword);
  while (keyword == "EVENT")
  {
    EXPECT_TRUE(fp >> x >> s);
    EXPECT_FALSE(x < 1 || x > 3) << "Bad type for EVENT directive";
    fp >> keyword;
  }

  EXPECT_EQ(keyword, "LEVELS") << "Bad LEVELS directive";
  EXPECT_TRUE(fp >> std::dec >> levels);

  EXPECT_TRUE(fp >> keyword);
	for (int level = 0; level < levels; level++)
	{
		int j, points, faces;

    EXPECT_EQ(keyword, "LEVEL") << "Bad LEVEL directive  in " << input.string() << " for level " << level + 1;
    EXPECT_TRUE(fp >> std::dec >> x);
    EXPECT_EQ(x, level + 1) << "LEVEL directive  was " << x << " should be " << level + 1;

    EXPECT_TRUE(fp >> keyword >> std::dec >> points);
    EXPECT_EQ(keyword, "POINTS") << "Bad POINTS directive";

		for (int j = 0; j < points; j++)
		{
			double a, b, c;
      EXPECT_TRUE(fp >> a >> b >> c);
    }
    EXPECT_TRUE(fp >> keyword >> faces);
    EXPECT_EQ(keyword, "POLYGONS") << "Bad POLYGONS directive in " << input.string() << " in level " << x;

		for (int j = 0; j < faces; ++j)
		{
			int k;
			unsigned int flags;

      EXPECT_TRUE(fp >> std::hex >> flags);

      EXPECT_FALSE(!(flags & iV_IMD_TEX)) << "in " << input.string() << " Bad polygon flag entry level" << level << " number " << j << "- no texture flag!";
      EXPECT_FALSE(flags & iV_IMD_XNOCUL) << "in" << input.string() << "Bad polygon flag entry level " << level << " number " << j << " - face culling not supported anymore!";
      EXPECT_TRUE(fp >> std::dec >> k);
      EXPECT_EQ(k, 3) << "Bad POLYGONS vertices entry level " << level << " number " << j << "-- non-triangle polygon found";

			// Read in vertex indices and texture coordinates
			for (int k = 0; k < 3; k++)
			{
				int l;
        EXPECT_TRUE(fp >> l);
			}
			if (flags & iV_IMD_TEXANIM)
			{
				int frames, rate;
        float width, height;
        EXPECT_TRUE(fp >> std::dec >> frames >> rate >> width >> height);
        EXPECT_GT(frames, 1) << "Level " << level << " Polygon " << j << "has a single animation frame";
			}
			for (int k = 0; k < 3; k++)
			{
				double t, u;
        EXPECT_TRUE(fp >> t >> u);
			}
		}

    fp >> keyword;
    if (keyword == "CONNECTORS")
		{
      EXPECT_TRUE(fp >> std::dec >> x);
      EXPECT_GE(x, 0) << "Bad CONNECTORS directive in level " << level;
			for (int j = 0; j < x; ++j)
			{
				int a, b, c;

        EXPECT_TRUE(fp >> a >> b >> c);
			}
      fp >> keyword;
		}
		if (keyword == "ANIMOBJECT")
		{
      int unused0, unused1;
      EXPECT_TRUE(fp >> unused0 >> unused1 >> x);
      EXPECT_GE(x, 0) << "Bad ANIMOBJECT directive in level " << level;
			for (int j = 0; j < x; ++j)
			{
				int frame;
				float xpos, ypos, zpos, xrot, yrot, zrot, xscale, yscale, zscale;

        EXPECT_TRUE(fp >> std::dec >> frame >> xpos >> ypos >> zpos >> xrot >> yrot >> zrot >> xscale >> yscale >> zscale);
			}
      fp >> keyword;
		}
	}
}

auto get_model_files = [](const auto& datapath) {
  return fs::recursive_directory_iterator(datapath) |
    ranges::view::transform([](auto&& e) {return e.path(); }) |
    ranges::view::filter([](auto&& p) { return p.extension() == fs::path(".pie"); });
};

TEST(modelTest, LoadModels)
{
  for (const auto& model_file : get_model_files(fs::current_path()))
  {
    EXPECT_NO_THROW(check_pie(model_file));
  }
}
