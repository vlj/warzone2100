/*
	This file is part of Warzone 2100.
	Copyright (C) 2013-2017  Warzone 2100 Project

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
#include <map/map.h>

#include <cstdint>
#include <array>
#include "lib/framework/physfs_ext.h" // Also includes physfs.h
#include "lib/framework/frame.h"

constexpr auto kMapMaxArea = 256u * 256u;

constexpr auto kVersion9 = 9u;
constexpr auto kVersion10 = 10u;
constexpr auto kVersion39 = 39u;

constexpr auto kCurrentVersionNum = kVersion39;

constexpr auto kElevationScale = 2u;


namespace
{
    auto& mapTile(MapData& map, int x, int y) { return map.mMapTiles[y * map.width + x]; }
}

std::unique_ptr<MapData> mapLoad(const std::string &filename)
{
	const auto &path = filename.c_str();
	PHYSFS_file *fp = PHYSFS_openRead(path);

	if (!fp)
	{
		//debug(LOG_ERROR, "Could not open %s", path);
		return nullptr;
	}

	MapData map;
	char aFileType[4];
	if (WZ_PHYSFS_readBytes(fp, aFileType, 4) != 4 || !PHYSFS_readULE32(fp, &map.mapVersion) ||
			 !PHYSFS_readULE32(fp, &map.width) || !PHYSFS_readULE32(fp, &map.height) || aFileType[0] != 'm' ||
			 aFileType[1] != 'a' || aFileType[2] != 'p')
	{
		//debug(LOG_ERROR, "Bad header in %s", path);
		PHYSFS_close(fp);
		return nullptr;
	}

	if (map.mapVersion <= kVersion9)
	{
		//debug(LOG_ERROR, "%s: Unsupported save format version %u", path, map.mapVersion);
		PHYSFS_close(fp);
		return nullptr;
	}
	if (map.mapVersion > kCurrentVersionNum)
	{
		//debug(LOG_ERROR, "%s: Undefined save format version %u", path, map.mapVersion);
		PHYSFS_close(fp);
		return nullptr;
	}
	else if (map.width * map.height > kMapMaxArea)
	{
		//debug(LOG_ERROR, "Map %s too large : %d %d", path, map.width, map.height);
		PHYSFS_close(fp);
		return nullptr;
	}

	if (map.width <= 1 || map.height <= 1)
	{
		//debug(LOG_ERROR, "Map is too small : %u, %u", map.width, map.height);
		PHYSFS_close(fp);
		return nullptr;
	}

	/* Load in the map data */
	for (int i = 0; i < map.width * map.height; i++)
	{
		uint16_t texture;
		uint8_t height;

		if (!PHYSFS_readULE16(fp, &texture) || !PHYSFS_readULE8(fp, &height))
		{
			//debug(LOG_ERROR, "%s: Error during savegame load", filename);
			PHYSFS_close(fp);
			return nullptr;
		}
		MapData::MapTile currentMapTile{};
		currentMapTile.texture = static_cast<TerrainType_>(texture);
		currentMapTile.height = height * kElevationScale;
		map.mMapTiles.emplace_back(currentMapTile);
	}

    uint32_t gwVersion;
	uint32_t numGateways;
	if (!PHYSFS_readULE32(fp, &gwVersion) || !PHYSFS_readULE32(fp, &numGateways) || gwVersion != 1)
	{
		//debug(LOG_ERROR, "Bad gateway in %s", path);
		PHYSFS_close(fp);
		return nullptr;
	}

	for (int i = 0; i < numGateways; i++)
	{
		MapData::Gateway gw = {};
		if (!PHYSFS_readULE8(fp, &gw.x1) || !PHYSFS_readULE8(fp, &gw.y1) || !PHYSFS_readULE8(fp, &gw.x2) ||
			!PHYSFS_readULE8(fp, &gw.y2))
		{
			//debug(LOG_ERROR, "%s: Failed to read gateway info", path);
			PHYSFS_close(fp);
			return nullptr;
		}
		map.mGateways.emplace_back(gw);
	}

	PHYSFS_close(fp);
	return std::make_unique<MapData>(map);
}

