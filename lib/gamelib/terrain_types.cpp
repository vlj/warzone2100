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
#include <map/terrain_types.h>
#include <array>
#include "lib/framework/physfs_ext.h" // Also includes physfs.h
#include "lib/framework/frame.h"

std::unique_ptr<TerrainTypeData> loadTerrainTypes(const std::string &filename)
{
	const auto &file = filename + "/ttypes.ttp";
	const auto *path = file.c_str();

	auto *fp = PHYSFS_openRead(path);
	if (!fp)
	{
		return nullptr;
	}
	char aFileType[4];
	TerrainTypeData map{};
	if (WZ_PHYSFS_readBytes(fp, aFileType, 4) != 4 || aFileType[0] != 't' || aFileType[1] != 't' ||
			 aFileType[2] != 'y' || aFileType[3] != 'p' || !PHYSFS_readULE32(fp, &map.terrainVersion) ||
		!PHYSFS_readULE32(fp, &map.numTerrainTypes))
	{
		//debug(LOG_ERROR, "Bad features header in %s", path);
		PHYSFS_close(fp);
		return nullptr;
	}

	if (map.numTerrainTypes >= kMaxTileTextures)
	{
		// Workaround for fugly map editor bug, since we can't fix the map editor
		map.numTerrainTypes = kMaxTileTextures - 1;
	}

	for (int i = 0; i < map.numTerrainTypes; i++)
	{
		uint16_t pType;
		PHYSFS_readULE16(fp, &pType);

		if (pType > static_cast<uint16_t>(TerrainType_::TER_MAX))
		{
			//debug(LOG_ERROR, "loadTerrainTypeMap: terrain type out of range");
			PHYSFS_close(fp);
			return nullptr;
		}

		map.terrainTypes[i] = (TerrainType_)pType;
	}

	PHYSFS_close(fp);
	return std::make_unique<TerrainTypeData>(map);
}
