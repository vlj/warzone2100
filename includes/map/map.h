/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2015  Warzone 2100 Project

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

#pragma once
#include <stdint.h>
#include <string>
#include <map/terrain_type.h>
#include <vector>

struct MapData
{
	struct Gateway
	{
		uint8_t x1, y1, x2, y2;
	};

	/* Information stored with each tile */
	struct MapTile
	{
		uint32_t height;	  // The height at the top left of the tile
		TerrainType_ texture; // Which graphics texture is on this tile
	};

	uint32_t height;
	uint32_t width;
	uint32_t mapVersion;

	// private members - don't touch! :-)
	std::vector<Gateway> mGateways;
	std::vector<MapTile> mMapTiles;
};

/* Load the map data */
std::unique_ptr<MapData> mapLoad(const std::string &mapFile);
