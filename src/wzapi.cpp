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

