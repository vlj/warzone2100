#include "wzapi.h"
#include "multiplay.h"
#include "order.h"
#include "structure.h"
#include "feature.h"
#include "map.h"
#include "fpath.h"
#include "mission.h"
#include "lib/framework/fixedpoint.h"
#include "transporter.h"
#include "scriptextern.h"
#include "display3d.h"
#include "template.h"
#include "hci.h"
#include "lighting.h"
#include "radar.h"
#include "multigifts.h"
#include "projectile.h"
#include "atmos.h"
#include "warcam.h"
#include "frontend.h"
#include "loop.h"
#include "random.h"
#include "component.h"
#include "research.h"
#include "lib/netplay/netplay.h"

BASE_OBJECT *IdToObject(OBJECT_TYPE type, int id, int player);

bool activateStructure(structure_id_player structVal, object_id_player_type objVal)
{
	STRUCTURE *psStruct = IdToStruct(structVal.id, structVal.player);
	BASE_OBJECT *psObj = IdToObject(objVal.type, objVal.id, objVal.player);
	orderStructureObj(structVal.player, psObj);
	return true;
}

//-- \subsection{structureIdle(structure)}
//-- Is given structure idle?
bool structureIdle_(structure_id_player structVal)
{
	STRUCTURE *psStruct = IdToStruct(structVal.id, structVal.player);
	return structureIdle(psStruct);
}

//-- \subsection{removeStruct(structure)}
//-- Immediately remove the given structure from the map. Returns a boolean that is true on success.
//-- No special effects are applied. Deprecated since 3.2.
bool removeStruct_(structure_id_player structVal)
{
	STRUCTURE *psStruct = IdToStruct(structVal.id, structVal.player);
	return removeStruct(psStruct, true);
}

//-- \subsection{removeObject(game object[, special effects?])}
//-- Remove the given game object with special effects. Returns a boolean that is true on success.
//-- A second, optional boolean parameter specifies whether special effects are to be applied. (3.2+ only)
bool removeObject(object_id_player_type qval, bool sfx)
{
	BASE_OBJECT *psObj = IdToObject(qval.type, qval.id, qval.player);
	bool retval = false;
	if (sfx)
	{
		switch (psObj->type)
		{
		case OBJ_STRUCTURE: destroyStruct((STRUCTURE *)psObj, gameTime); break;
		case OBJ_DROID: retval = destroyDroid((DROID *)psObj, gameTime); break;
		case OBJ_FEATURE: retval = destroyFeature((FEATURE *)psObj, gameTime); break;
		default: break;
		}
	}
	else
	{
		switch (psObj->type)
		{
		case OBJ_STRUCTURE: retval = removeStruct((STRUCTURE *)psObj, true); break;
		case OBJ_DROID: retval = removeDroidBase((DROID *)psObj); break;
		case OBJ_FEATURE: retval = removeFeature((FEATURE *)psObj); break;
		default: break;
		}
	}
	return retval;
}

float distBetweenTwoPoints(float x1, float y1, float x2, float y2)
{
	return iHypot(x1 - x2, y1 - y2);
}

//-- \subsection{droidCanReach(droid, x, y)}
//-- Return whether or not the given droid could possibly drive to the given position. Does
//-- not take player built blockades into account.
bool droidCanReach(DROID* psDroid, int x, int y)
{
	const PROPULSION_STATS *psPropStats = asPropulsionStats + psDroid->asBits[COMP_PROPULSION];
	return fpathCheck(psDroid->pos, Vector3i(world_coord(x), world_coord(y), 0), psPropStats->propulsionType);
}

//-- \subsection{propulsionCanReach(propulsion, x1, y1, x2, y2)}
//-- Return true if a droid with a given propulsion is able to travel from (x1, y1) to (x2, y2).
//-- Does not take player built blockades into account. (3.2+ only)
bool propulsionCanReach(const char* propulsionValueName, int x1, int y1, int x2, int y2)
{
	int propulsion = getCompFromName(COMP_PROPULSION, propulsionValueName);
	const PROPULSION_STATS *psPropStats = asPropulsionStats + propulsion;
	return fpathCheck(Vector3i(world_coord(x1), world_coord(y1), 0), Vector3i(world_coord(x2), world_coord(y2), 0), psPropStats->propulsionType);
}

//-- \subsection{terrainType(x, y)}
//-- Returns tile type of a given map tile, such as TER_WATER for water tiles or TER_CLIFFFACE for cliffs.
//-- Tile types regulate which units may pass through this tile. (3.2+ only)
unsigned char terrainType_(int x, int y)
{
	return terrainType(mapTile(x, y));
}

bool orderDroid_(DROID* psDroid, int _order)
{
	DROID_ORDER order = (DROID_ORDER)_order;
	if (order == DORDER_REARM)
	{
		if (STRUCTURE *psStruct = findNearestReArmPad(psDroid, psDroid->psBaseStruct, false))
		{
			orderDroidObj(psDroid, order, psStruct, ModeQueue);
		}
		else
		{
			orderDroid(psDroid, DORDER_RTB, ModeQueue);
		}
	}
	else
	{
		orderDroid(psDroid, order, ModeQueue);
	}
	return true;
}

//-- \subsection{orderDroidObj(droid, order, object)}
//-- Give a droid an order to do something to something.
bool orderDroidObj_(DROID* psDroid, int order_, object_id_player_type objVal)
{
	BASE_OBJECT *psObj = IdToObject(objVal.type, objVal.id, objVal.player);
	DROID_ORDER order = (DROID_ORDER)order_;
	orderDroidObj(psDroid, order, psObj, ModeQueue);
	return true;
}

bool orderDroidBuild5(DROID* psDroid, int _order, const char* statName, int x, int y, float _direction)
{
	int index = getStructStatFromName(statName);
	DROID_ORDER order = (DROID_ORDER)_order;
	STRUCTURE_STATS	*psStats = &asStructureStats[index];
	uint16_t direction = DEG(_direction);
	orderDroidStatsLocDir(psDroid, order, psStats, world_coord(x) + TILE_UNITS / 2, world_coord(y) + TILE_UNITS / 2, direction, ModeQueue);
	return true;
}

bool orderDroidBuild4(DROID* psDroid, int _order, const char* statName, int x, int y)
{
	return orderDroidBuild5(psDroid, _order, statName, x, y, 0.f);
}

//-- \subsection{orderDroidLoc(droid, order, x, y)}
//-- Give a droid an order to do something at the given location.
bool orderDroidLoc_(DROID* psDroid, int order_, int x, int y)
{
	DROID_ORDER order = (DROID_ORDER)order_;
	orderDroidLoc(psDroid, order, world_coord(x), world_coord(y), ModeQueue);
	return true;
}

//-- \subsection{setMissionTime(time)} Set mission countdown in seconds.
bool SetMissionTime(int time)
{
	int value = time * GAME_TICKS_PER_SEC;
	mission.startTime = gameTime;
	mission.time = value;
	setMissionCountDown();
	if (mission.time >= 0)
	{
		mission.startTime = gameTime;
		addMissionTimerInterface();
	}
	else
	{
		intRemoveMissionTimer();
		mission.cheatTime = 0;
	}
	return true;
}

//-- \subsection{getMissionTime()} Get time remaining on mission countdown in seconds. (3.2+ only)
int getMissionTime()
{
	return (mission.time - (gameTime - mission.startTime)) / GAME_TICKS_PER_SEC;
}

//-- \subsection{setTransporterExit(x, y, player)}
//-- Set the exit position for the mission transporter. (3.2+ only)
bool setTransporterExit(int x, int y, int player)
{
	missionSetTransporterExit(player, x, y);
	return true;
}

//-- \subsection{startTransporterEntry(x, y, player)}
//-- Set the entry position for the mission transporter, and make it start flying in
//-- reinforcements. If you want the camera to follow it in, use cameraTrack() on it.
//-- The transport needs to be set up with the mission droids, and the first transport
//-- found will be used. (3.2+ only)
bool startTransporterEntry(int x, int y, int player)
{
	missionSetTransporterEntry(player, x, y);
	missionFlyTransportersIn(player, false);
	return true;
}

//-- \subsection{useSafetyTransport(flag)}
//-- Change if the mission transporter will fetch droids in non offworld missions
//-- setReinforcementTime() is be used to hide it before coming back after the set time
//-- which is handled by the campaign library in the victory data section (3.2.4+ only).
bool useSafetyTransport(bool flag)
{
	setDroidsToSafetyFlag(flag);
	return true;
}

//-- \subsection{restoreLimboMissionData()}
//-- Swap mission type and bring back units previously stored at the start
//-- of the mission (see cam3-c mission). (3.2.4+ only).
bool restoreLimboMissionData()
{
	resetLimboMission();
	return true;
}

//-- \subsection{setReinforcementTime(time)} Set time for reinforcements to arrive. If time is
//-- negative, the reinforcement GUI is removed and the timer stopped. Time is in seconds.
//-- If time equals to the magic LZ_COMPROMISED_TIME constant, reinforcement GUI ticker
//-- is set to "--:--" and reinforcements are suppressed until this function is called
//-- again with a regular time value.
bool setReinforcementTime(int time)
{
	int value = time * GAME_TICKS_PER_SEC;
	mission.ETA = value;
	if (missionCanReEnforce())
	{
		addTransporterTimerInterface();
	}
	if (value < 0)
	{
		DROID *psDroid;

		intRemoveTransporterTimer();
		/* Only remove the launch if haven't got a transporter droid since the scripts set the
		* time to -1 at the between stage if there are not going to be reinforcements on the submap  */
		for (psDroid = apsDroidLists[selectedPlayer]; psDroid != nullptr; psDroid = psDroid->psNext)
		{
			if (isTransporter(psDroid))
			{
				break;
			}
		}
		// if not found a transporter, can remove the launch button
		if (psDroid == nullptr)
		{
			intRemoveTransporterLaunch();
		}
	}
	return true;
}

//-- \subsection{centreView(x, y)}
//-- Center the player's camera at the given position.
bool centreView(int x, int y)
{
	setViewPos(x, y, false);
	return true;
}

//-- \subsection{setTutorialMode(bool)} Sets a number of restrictions appropriate for tutorial if set to true.
bool setTutorialMode(bool intutorial)
{
	bInTutorial = intutorial;
	return true;
}

//-- \subsection{setMiniMap(bool)} Turns visible minimap on or off in the GUI.
bool setMiniMap(bool radarpermitted)
{
	radarPermitted = radarpermitted;
	return true;
}

//-- \subsection{setDesign(bool)} Whether to allow player to design stuff.
bool setDesign(bool allowdesign)
{
	DROID_TEMPLATE *psCurr;
	allowDesign = allowdesign;
	// Switch on or off future templates
	// FIXME: This dual data structure for templates is just plain insane.
	for (auto &keyvaluepair : droidTemplates[selectedPlayer])
	{
		bool researched = researchedTemplate(keyvaluepair.second, selectedPlayer);
		keyvaluepair.second->enabled = (researched || allowDesign);
	}
	for (auto &localTemplate : localTemplates)
	{
		psCurr = &localTemplate;
		bool researched = researchedTemplate(psCurr, selectedPlayer);
		psCurr->enabled = (researched || allowDesign);
	}
	return true;
}


//-- \subsection{enableTemplate(template name)} Enable a specific template (even if design is disabled).
bool enableTemplate(const char* templateName)
{
	DROID_TEMPLATE *psCurr;
	bool found = false;
	// FIXME: This dual data structure for templates is just plain insane.
	for (auto &keyvaluepair : droidTemplates[selectedPlayer])
	{
		if (QString(templateName).compare(keyvaluepair.second->id) == 0)
		{
			keyvaluepair.second->enabled = true;
			found = true;
			break;
		}
	}
	if (!found)
	{
		debug(LOG_ERROR, "Template %s was not found!", templateName);
		return false;
	}
	for (auto &localTemplate : localTemplates)
	{
		psCurr = &localTemplate;
		if (QString(templateName).compare(psCurr->id) == 0)
		{
			psCurr->enabled = true;
			break;
		}
	}
	return true;
}

//-- \subsection{removeTemplate(template name)} Remove a template.
bool removeTemplate(const char* templateName)
{
	DROID_TEMPLATE *psCurr;
	bool found = false;
	// FIXME: This dual data structure for templates is just plain insane.
	for (auto &keyvaluepair : droidTemplates[selectedPlayer])
	{
		if (QString(templateName).compare(keyvaluepair.second->id) == 0)
		{
			keyvaluepair.second->enabled = false;
			found = true;
			break;
		}
	}
	if (!found)
	{
		debug(LOG_ERROR, "Template %s was not found!", templateName);
		return false;
	}
	for (std::list<DROID_TEMPLATE>::iterator i = localTemplates.begin(); i != localTemplates.end(); ++i)
	{
		psCurr = &*i;
		if (QString(templateName).compare(psCurr->id) == 0)
		{
			localTemplates.erase(i);
			break;
		}
	}
	return true;
}

bool setDroidExperience(DROID* psDroid, float exp)
{
	psDroid->experience = exp * 65536;
	return true;
}


bool setAssemblyPoint_(structure_id_player structVal, int x, int y)
{
	STRUCTURE *psStruct = IdToStruct(structVal.id, structVal.player);
	setAssemblyPoint(((FACTORY *)psStruct->pFunctionality)->psAssemblyPoint, x, y, structVal.player, true);
	return true;
}

static void setComponent(const QString& name, int player, int value)
{
	COMPONENT_STATS *psComp = getCompStatsFromName(name);
	ASSERT_OR_RETURN(, psComp, "Bad component %s", name.toUtf8().constData());
	apCompLists[player][psComp->compType][psComp->index] = value;
}
//-- \subsection{enableComponent(component, player)}
//-- The given component is made available for research for the given player.
bool enableComponent(const char* componentName, int player)
{
	setComponent(componentName, player, FOUND);
	return true;
}

//-- \subsection{makeComponentAvailable(component, player)}
//-- The given component is made available to the given player. This means the player can
//-- actually build designs with it.
bool makeComponentAvailable(const char* componentName, int player)
{
//	SCRIPT_ASSERT_PLAYER(context, player);
	setComponent(componentName, player, AVAILABLE);
	return true;
}

//-- \subsection{allianceExistsBetween(player, player)}
//-- Returns true if an alliance exists between the two players, or they are the same player.
bool allianceExistsBetween(int player1, int player2)
{
//	SCRIPT_ASSERT(context, player1 < MAX_PLAYERS && player1 >= 0, "Invalid player");
//	SCRIPT_ASSERT(context, player2 < MAX_PLAYERS && player2 >= 0, "Invalid player");
	return alliances[player1][player2] == ALLIANCE_FORMED;
}


//-- \subsection{_(string)}
//-- Mark string for translation.
const char* translate(const char* txt)
{
	// The redundant QString cast is a workaround for a Qt5 bug, the QScriptValue(char const *) constructor interprets as Latin1 instead of UTF-8!
	return gettext(txt);
}

//-- \subsection{safeDest(player, x, y)} Returns true if given player is safe from hostile fire at
//-- the given location, to the best of that player's map knowledge.
bool safeDest(int player, int x, int y)
{
//	SCRIPT_ASSERT_PLAYER(context, player);
//	SCRIPT_ASSERT(context, tileOnMap(x, y), "Out of bounds coordinates(%d, %d)", x, y);
	return !(auxTile(x, y, player) & AUXBITS_DANGER);
}

//-- \subsection{setNoGoArea(x1, y1, x2, y2, player)}
//-- Creates an area on the map on which nothing can be built. If player is zero,
//-- then landing lights are placed. If player is -1, then a limbo landing zone
//-- is created and limbo droids placed.
// FIXME: missing a way to call initNoGoAreas(); check if we can call this in
// every level start instead of through scripts
bool _setNoGoArea(int x1, int y1, int x2, int y2, int player)
{
//	SCRIPT_ASSERT(context, x1 >= 0, "Minimum scroll x value %d is less than zero - ", x1);
//	SCRIPT_ASSERT(context, y1 >= 0, "Minimum scroll y value %d is less than zero - ", y1);
//	SCRIPT_ASSERT(context, x2 <= mapWidth, "Maximum scroll x value %d is greater than mapWidth %d", x2, (int)mapWidth);
//	SCRIPT_ASSERT(context, y2 <= mapHeight, "Maximum scroll y value %d is greater than mapHeight %d", y2, (int)mapHeight);
//	SCRIPT_ASSERT(context, player < MAX_PLAYERS && player >= -1, "Bad player value %d", player);

	if (player == -1)
	{
		setNoGoArea(x1, y1, x2, y2, LIMBO_LANDING);
		placeLimboDroids();	// this calls the Droids from the Limbo list onto the map
	}
	else
	{
		setNoGoArea(x1, y1, x2, y2, player);
	}
	return true;
}


//-- \subsection{setScrollLimits(x1, y1, x2, y2)}
//-- Limit the scrollable area of the map to the given rectangle. (3.2+ only)
bool setScrollLimits(int minX, int minY, int maxX, int maxY)
{
//	SCRIPT_ASSERT(context, minX >= 0, "Minimum scroll x value %d is less than zero - ", minX);
//	SCRIPT_ASSERT(context, minY >= 0, "Minimum scroll y value %d is less than zero - ", minY);
//	SCRIPT_ASSERT(context, maxX <= mapWidth, "Maximum scroll x value %d is greater than mapWidth %d", maxX, (int)mapWidth);
//	SCRIPT_ASSERT(context, maxY <= mapHeight, "Maximum scroll y value %d is greater than mapHeight %d", maxY, (int)mapHeight);

	const int prevMinX = scrollMinX;
	const int prevMinY = scrollMinY;
	const int prevMaxX = scrollMaxX;
	const int prevMaxY = scrollMaxY;

	scrollMinX = minX;
	scrollMaxX = maxX;
	scrollMinY = minY;
	scrollMaxY = maxY;

	// When the scroll limits change midgame - need to redo the lighting
	initLighting(prevMinX < scrollMinX ? prevMinX : scrollMinX,
		prevMinY < scrollMinY ? prevMinY : scrollMinY,
		prevMaxX < scrollMaxX ? prevMaxX : scrollMaxX,
		prevMaxY < scrollMaxY ? prevMaxY : scrollMaxY);

	// need to reset radar to take into account of new size
	resizeRadar();
	return true;
}

//-- \subsection{loadLevel(level name)}
//-- Load the level with the given name.
bool loadLevel(const char* level)
{
	sstrcpy(aLevelName, level);

	// Find the level dataset
	LEVEL_DATASET *psNewLevel = levFindDataSet(level);
//	SCRIPT_ASSERT(context, psNewLevel, "Could not find level data for %s", level.toUtf8().constData());

	// Get the mission rolling...
	nextMissionType = psNewLevel->type;
	loopMissionState = LMS_CLEAROBJECTS;
	return true;
}

//-- \subsection{setAlliance(player1, player2, value)}
//-- Set alliance status between two players to either true or false. (3.2+ only)
bool setAlliance(int player1, int player2, bool value)
{
	if (value)
	{
		formAlliance(player1, player2, true, false, true);
	}
	else
	{
		breakAlliance(player1, player2, true, true);
	}
	return true;
}

//-- \subsection{getExperienceModifier(player)}
//-- Get the percentage of experience this player droids are going to gain. (3.2+ only)
int getExperienceModifier(int player)
{
	return getExpGain(player);
}

//-- \subsection{setExperienceModifier(player, percent)}
//-- Set the percentage of experience this player droids are going to gain. (3.2+ only)
bool setExperienceModifier(int player, int percent)
{
	setExpGain(player, percent);
	return true;
}

//-- \subsection{setSunPosition(x, y, z)}
//-- Move the position of the Sun, which in turn moves where shadows are cast. (3.2+ only)
bool setSunPosition(float x, float y, float z)
{
	setTheSun(Vector3f(x, y, z));
	return true;
}

//-- \subsection{setSunIntensity(ambient r, g, b, diffuse r, g, b, specular r, g, b)}
//-- Set the ambient, diffuse and specular colour intensities of the Sun lighting source. (3.2+ only)
bool setSunIntensity(float ambient_r, float ambient_g, float ambient_b, float diffuse_r, float diffuse_g, float diffuse_b, float specular_r, float specular_g, float specular_b)
{
	float ambient[4];
	float diffuse[4];
	float specular[4];
	ambient[0] = ambient_r;
	ambient[1] = ambient_g;
	ambient[2] = ambient_b;
	ambient[3] = 1.0f;
	diffuse[0] = diffuse_r;
	diffuse[1] = diffuse_g;
	diffuse[2] = diffuse_b;
	diffuse[3] = 1.0f;
	specular[0] = specular_r;
	specular[1] = specular_g;
	specular[2] = specular_b;
	specular[3] = 1.0f;
	pie_Lighting0(LIGHT_AMBIENT, ambient);
	pie_Lighting0(LIGHT_DIFFUSE, diffuse);
	pie_Lighting0(LIGHT_SPECULAR, specular);
	return true;
}

//-- \subsection{setWeather(weather type)}
//-- Set the current weather. This should be one of WEATHER_RAIN, WEATHER_SNOW or WEATHER_CLEAR. (3.2+ only)
bool setWeather(int _weather)
{
	WT_CLASS weather = (WT_CLASS)_weather;
//	SCRIPT_ASSERT(context, weather >= 0 && weather <= WT_NONE, "Bad weather type");
	atmosSetWeatherType(weather);
	return true;
}

//-- \subsection{setSky(texture file, wind speed, skybox scale)}
//-- Change the skybox. (3.2+ only)
bool setSky(const char* page, float wind, float scale)
{
	setSkyBox(page, wind, scale);
	return true;
}

//-- \subsection{cameraSlide(x, y)}
//-- Slide the camera over to the given position on the map. (3.2+ only)
bool cameraSlide(float x, float y)
{
	requestRadarTrack(x, y);
	return true;
}

//-- \subsection{cameraZoom(z, speed)}
//-- Slide the camera to the given zoom distance. Normal camera zoom ranges between 500 and 5000. (3.2+ only)
bool cameraZoom(float z, float speed)
{
	setZoom(speed, z);
	return true;
}

//-- \subsection{addSpotter(x, y, player, range, type, expiry)}
//-- Add an invisible viewer at a given position for given player that shows map in given range. \emph{type}
//-- is zero for vision reveal, or one for radar reveal. The difference is that a radar reveal can be obstructed
//-- by ECM jammers. \emph{expiry}, if non-zero, is the game time at which the spotter shall automatically be
//-- removed. The function returns a unique ID that can be used to remove the spotter with \emph{removeSpotter}. (3.2+ only)
unsigned int _addSpotter(int x, int y, int player, int range, bool radar, unsigned int expiry)
{
	return addSpotter(x, y, player, range, radar, expiry);
}

//-- \subsection{removeSpotter(id)}
//-- Remove a spotter given its unique ID. (3.2+ only)
bool _removeSpotter(unsigned int id)
{
	removeSpotter(id);
	return true;
}

//-- \subsection{syncRandom(limit)}
//-- Generate a synchronized random number in range 0...(limit - 1) that will be the same if this function is
//-- run on all network peers in the same game frame. If it is called on just one peer (such as would be
//-- the case for AIs, for instance), then game sync will break. (3.2+ only)
int syncRandom(unsigned int limit)
{
	return gameRand(limit);
}

//-- \subsection{replaceTexture(old_filename, new_filename)}
//-- Replace one texture with another. This can be used to for example give buildings on a specific tileset different
//-- looks, or to add variety to the looks of droids in campaign missions. (3.2+ only)
bool _replaceTexture(const char* oldfile, const char* newfile)
{
	_replaceTexture(oldfile, newfile);
	return true;
}

//-- \subsection{fireWeaponAtLoc(x, y, weapon_name)}
//-- Fires a weapon at the given coordinates (3.2.4+ only).
bool fireWeaponAtLoc(int xLocation, int yLocation, const char *weaponName)
{
	int weapon = getCompFromName(COMP_WEAPON, weaponName);
	//	SCRIPT_ASSERT(context, weapon > 0, "No such weapon: %s", weaponValue.toString().toUtf8().constData());

	Vector3i target;
	target.x = xLocation;
	target.y = yLocation;
	target.z = map_Height(xLocation, yLocation);

	WEAPON sWeapon;
	sWeapon.nStat = weapon;
	// send the projectile using the selectedPlayer so that it can always be seen
	proj_SendProjectile(&sWeapon, nullptr, selectedPlayer, target, nullptr, true, 0);
	return true;
}

//-- \subsection{changePlayerColour(player, colour)}
//-- Change a player's colour slot. The current player colour can be read from the playerData array. There are as many
//-- colour slots as the maximum number of players. (3.2.3+ only)
bool changePlayerColour(int player, int colour)
{
	setPlayerColour(player, colour);
	return true;
}

//-- \subsection{pursueResearch(lab, research)}
//-- Start researching the first available technology on the way to the given technology.
//-- First parameter is the structure to research in, which must be a research lab. The
//-- second parameter is the technology to pursue, as a text string as defined in "research.json".
//-- The second parameter may also be an array of such strings. The first technology that has
//-- not yet been researched in that list will be pursued.
bool pursueResearch(structure_id_player structVal, string_list list)
{
	STRUCTURE *psStruct = IdToStruct(structVal.id, structVal.player);
//	SCRIPT_ASSERT(context, psStruct, "No such structure id %d belonging to player %d", id, player);
	RESEARCH *psResearch = nullptr;  // Dummy initialisation.
	int k;
	if (list.count > 1)
	{
		for (k = 0; k < list.count; ++k)
		{
			psResearch = getResearch(list.strings[k]);
			//			SCRIPT_ASSERT(context, psResearch, "No such research: %s", resName.toUtf8().constData());
			PLAYER_RESEARCH *plrRes = &asPlayerResList[structVal.player][psResearch->index];
			if (!IsResearchStartedPending(plrRes) && !IsResearchCompleted(plrRes))
			{
				break; // use this one
			}
		}
		if (k == list.count)
		{
			debug(LOG_SCRIPT, "Exhausted research list -- doing nothing");
			return false;
		}
	}
	else
	{
		psResearch = getResearch(list.strings[0]);
//		SCRIPT_ASSERT(context, psResearch, "No such research: %s", resName.toUtf8().constData());
		PLAYER_RESEARCH *plrRes = &asPlayerResList[structVal.player][psResearch->index];
		if (IsResearchStartedPending(plrRes) || IsResearchCompleted(plrRes))
		{
			debug(LOG_SCRIPT, "%s has already been researched!", list.strings[0]);
			return false;
		}
	}
//	SCRIPT_ASSERT(context, psStruct->pStructureType->type == REF_RESEARCH, "Not a research lab: %s", objInfo(psStruct));
	RESEARCH_FACILITY *psResLab = (RESEARCH_FACILITY *)psStruct->pFunctionality;
//	SCRIPT_ASSERT(context, psResLab->psSubject == nullptr, "Research lab not ready");
	// Go down the requirements list for the desired tech
	QList<RESEARCH *> reslist;
	RESEARCH *cur = psResearch;
	int iterations = 0;  // Only used to assert we're not stuck in the loop.
	while (cur)
	{
		if (researchAvailable(cur->index, structVal.player, ModeQueue))
		{
			bool started = false;
			for (int i = 0; i < game.maxPlayers; i++)
			{
				if (i == structVal.player || (aiCheckAlliances(structVal.player, i) && alliancesSharedResearch(game.alliance)))
				{
					int bits = asPlayerResList[i][cur->index].ResearchStatus;
					started = started || (bits & STARTED_RESEARCH) || (bits & STARTED_RESEARCH_PENDING)
						|| (bits & RESBITS_PENDING_ONLY) || (bits & RESEARCHED);
				}
			}
			if (!started) // found relevant item on the path?
			{
				sendResearchStatus(psStruct, cur->index, structVal.player, true);
#if defined (DEBUG)
				char sTemp[128];
				sprintf(sTemp, "player:%d starts topic from script: %s", player, getID(cur));
				NETlogEntry(sTemp, SYNC_FLAG, 0);
#endif
				debug(LOG_SCRIPT, "Started research in %d's %s(%d) of %s", player,
					objInfo(psStruct), psStruct->id, getName(cur));
				return true;
			}
		}
		RESEARCH *prev = cur;
		cur = nullptr;
		if (!prev->pPRList.empty())
		{
			cur = &asResearch[prev->pPRList[0]]; // get first pre-req
		}
		for (int i = 1; i < prev->pPRList.size(); i++)
		{
			// push any other pre-reqs on the stack
			reslist += &asResearch[prev->pPRList[i]];
		}
		if (!cur && !reslist.empty())
		{
			cur = reslist.takeFirst(); // retrieve options from the stack
		}
		ASSERT_OR_RETURN(false, ++iterations < asResearch.size() * 100 || !cur, "Possible cyclic dependencies in prerequisites, possibly of research \"%s\".", getName(cur));
	}
	debug(LOG_SCRIPT, "No research topic found for %s(%d)", objInfo(psStruct), psStruct->id);
	return false; // none found
}

//-- \subsection{addFeature(name, x, y)}
//-- Create and place a feature at the given x, y position. Will cause a desync in multiplayer.
//-- Returns the created game object on success, null otherwise. (3.2+ only)
FEATURE* _addFeature(const char* featName, int x, int y)
{
	int feature = getFeatureStatFromName(featName);
	FEATURE_STATS *psStats = &asFeatureStats[feature];
	for (FEATURE *psFeat = apsFeatureLists[0]; psFeat; psFeat = psFeat->psNext)
	{
//		SCRIPT_ASSERT(context, map_coord(psFeat->pos.x) != x || map_coord(psFeat->pos.y) != y,
//			"Building feature on tile already occupied");
	}
	return buildFeature(psStats, world_coord(x), world_coord(y), false);
}

//-- \subsection{addDroidToTransporter(transporter, droid)}
//-- Load a droid, which is currently located on the campaign off-world mission list,
//-- into a transporter, which is also currently on the campaign off-world mission list.
//-- (3.2+ only)
bool addDroidToTransporter(droid_id_player transporter, droid_id_player droid)
{
	DROID *psTransporter = IdToMissionDroid(transporter.id, transporter.player);
//	SCRIPT_ASSERT(context, psTransporter, "No such transporter id %d belonging to player %d", transporterId, transporterPlayer);
//	SCRIPT_ASSERT(context, isTransporter(psTransporter), "Droid id %d belonging to player %d is not a transporter", transporterId, transporterPlayer);
	DROID *psDroid = IdToMissionDroid(droid.id, droid.player);
//	SCRIPT_ASSERT(context, psDroid, "No such droid id %d belonging to player %d", droidId, droidPlayer);
//	SCRIPT_ASSERT(context, checkTransporterSpace(psTransporter, psDroid), "Not enough room in transporter %d for droid %d", transporterId, droidId);
	bool removeSuccessful = droidRemove(psDroid, mission.apsDroidLists);
//	SCRIPT_ASSERT(context, removeSuccessful, "Could not remove droid id %d from mission list", droidId);
	psTransporter->psGroup->add(psDroid);
	return true;
}

extern bool structDoubleCheck(BASE_STATS *psStat, UDWORD xx, UDWORD yy, SDWORD maxBlockingTiles);

//-- \subsection{pickStructLocation(droid, structure type, x, y)}
//-- Pick a location for constructing a certain type of building near some given position.
//-- Returns an object containing "type" POSITION, and "x" and "y" values, if successful.
optional_position pickStructLocation(droid_id_player droidVal, const char* statName, int startX, int startY, int maxBlockingTiles)
{
	DROID *psDroid = IdToDroid(droidVal.id, droidVal.player);
	int index = getStructStatFromName(statName);
	//SCRIPT_ASSERT(context, index >= 0, "%s not found", statName.toUtf8().constData());
	STRUCTURE_STATS	*psStat = &asStructureStats[index];
	int numIterations = 30;
	bool found = false;
	int incX, incY, x, y;

//	SCRIPT_ASSERT(context, psDroid, "No such droid id %d belonging to player %d", id, player);
//	SCRIPT_ASSERT(context, psStat, "No such stat found: %s", statName.toUtf8().constData());
//	SCRIPT_ASSERT_PLAYER(context, player);
//	SCRIPT_ASSERT(context, startX >= 0 && startX < mapWidth && startY >= 0 && startY < mapHeight, "Bad position (%d, %d)", startX, startY);

	x = startX;
	y = startY;

	Vector2i offset(psStat->baseWidth * (TILE_UNITS / 2), psStat->baseBreadth * (TILE_UNITS / 2));

	// save a lot of typing... checks whether a position is valid
#define LOC_OK(_x, _y) (tileOnMap(_x, _y) && \
                        (!psDroid || fpathCheck(psDroid->pos, Vector3i(world_coord(_x), world_coord(_y), 0), PROPULSION_TYPE_WHEELED)) \
                        && validLocation(psStat, world_coord(Vector2i(_x, _y)) + offset, 0, droidVal.player, false) && structDoubleCheck(psStat, _x, _y, maxBlockingTiles))

	// first try the original location
	if (LOC_OK(startX, startY))
	{
		found = true;
	}

	// try some locations nearby
	for (incX = 1, incY = 1; incX < numIterations && !found; incX++, incY++)
	{
		y = startY - incY;	// top
		for (x = startX - incX; x < startX + incX; x++)
		{
			if (LOC_OK(x, y))
			{
				found = true;
				goto endstructloc;
			}
		}
		x = startX + incX;	// right
		for (y = startY - incY; y < startY + incY; y++)
		{
			if (LOC_OK(x, y))
			{
				found = true;
				goto endstructloc;
			}
		}
		y = startY + incY;	// bottom
		for (x = startX + incX; x > startX - incX; x--)
		{
			if (LOC_OK(x, y))
			{
				found = true;
				goto endstructloc;
			}
		}
		x = startX - incX;	// left
		for (y = startY + incY; y > startY - incY; y--)
		{
			if (LOC_OK(x, y))
			{
				found = true;
				goto endstructloc;
			}
		}
	}

endstructloc:
	if (found)
	{
		return { true, x + map_coord(offset.x) , y + map_coord(offset.y) };
	}
	else
	{
		debug(LOG_SCRIPT, "Did not find valid positioning for %s", getName(psStat));
	}
	return { false };
}

//-- \subsection{donatePower(amount, to)}
//-- Donate power to another player. Returns true. (3.2+ only)
bool donatePower(int amount, int to, me from)
{
	giftPower(from.player, to, amount, true);
	return true;
}

//-- \subsection{addStructure(structure type, player, x, y)}
//-- Create a structure on the given position. Returns the structure on success, null otherwise.
STRUCTURE* _addStructure(const char* building, int player, int x, int y)
{
	int index = getStructStatFromName(building);
//	SCRIPT_ASSERT(context, index >= 0, "%s not found", building.toUtf8().constData());
//	SCRIPT_ASSERT_PLAYER(context, player);
	STRUCTURE_STATS *psStat = &asStructureStats[index];
	STRUCTURE *psStruct = buildStructure(psStat, x, y, player, false);
	if (psStruct)
	{
		psStruct->status = SS_BUILT;
		buildingComplete(psStruct);
		return psStruct;
	}
	return nullptr;
}

