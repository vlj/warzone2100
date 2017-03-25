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
/** @file
 *  Interface to the common stats module
 */
#ifndef __INCLUDED_SRC_STATS_H__
#define __INCLUDED_SRC_STATS_H__

#include "lib/gamelib/wzconfig.h"

#include <utility>

#include "objectdef.h"

/**************************************************************************************
 *
 * Function prototypes and data storage for the stats
 */

/* The stores for the different stats */
extern BODY_STATS			*asBodyStats;
extern BRAIN_STATS			*asBrainStats;
extern PROPULSION_STATS		*asPropulsionStats;
extern SENSOR_STATS			*asSensorStats;
extern ECM_STATS			*asECMStats;
extern REPAIR_STATS			*asRepairStats;
extern WEAPON_STATS			*asWeaponStats;
extern CONSTRUCT_STATS		*asConstructStats;
extern PROPULSION_TYPES		*asPropulsionTypes;

//used to hold the modifiers cross refd by weapon effect and propulsion type
extern WEAPON_MODIFIER		asWeaponModifier[WE_NUMEFFECTS][PROPULSION_TYPE_NUM];
extern WEAPON_MODIFIER		asWeaponModifierBody[WE_NUMEFFECTS][SIZE_NUM];

/* The number of different stats stored */
extern uint32_t		numBodyStats;
extern uint32_t		numBrainStats;
extern uint32_t		numPropulsionStats;
extern uint32_t		numSensorStats;
extern uint32_t		numECMStats;
extern uint32_t		numRepairStats;
extern uint32_t		numWeaponStats;
extern uint32_t		numConstructStats;
extern uint32_t		numTerrainTypes;

/* What number the ref numbers start at for each type of stat */
#define REF_BODY_START			0x010000
#define REF_BRAIN_START			0x020000
#define REF_PROPULSION_START	0x040000
#define REF_SENSOR_START		0x050000
#define REF_ECM_START			0x060000
#define REF_REPAIR_START		0x080000
#define REF_WEAPON_START		0x0a0000
#define REF_RESEARCH_START		0x0b0000
#define REF_TEMPLATE_START		0x0c0000
#define REF_STRUCTURE_START		0x0d0000
#define REF_FUNCTION_START		0x0e0000
#define REF_CONSTRUCT_START		0x0f0000
#define REF_FEATURE_START		0x100000

/* The maximum number of refs for a type of stat */
#define REF_RANGE				0x010000


//stores for each players component states - see below
extern uint8_t		*apCompLists[MAX_PLAYERS][COMP_NUMCOMPONENTS];

//store for each players Structure states
extern uint8_t		*apStructTypeLists[MAX_PLAYERS];

//Values to fill apCompLists and apStructTypeLists. Not a bitfield, values are in case that helps with savegame compatibility.
enum ItemAvailability
{
	AVAILABLE = 1,    // This item can be used to design droids.
	UNAVAILABLE = 2,  // The player does not know about this item.
	FOUND = 4,        // This item has been found, but is unresearched.
	REDUNDANT = 10,   // The player no longer needs this item.
};

/*******************************************************************************
*		Allocate stats functions
*******************************************************************************/
/* Allocate Weapon stats */
extern bool statsAllocWeapons(uint32_t numEntries);

/*Allocate Armour stats*/
//extern bool statsAllocArmour(uint32_t numEntries);

/*Allocate Body stats*/
extern bool statsAllocBody(uint32_t numEntries);

/*Allocate Brain stats*/
extern bool statsAllocBrain(uint32_t numEntries);

/*Allocate Propulsion stats*/
extern bool statsAllocPropulsion(uint32_t numEntries);

/*Allocate Sensor stats*/
extern bool statsAllocSensor(uint32_t numEntries);

/*Allocate Ecm Stats*/
extern bool statsAllocECM(uint32_t numEntries);

/*Allocate Repair Stats*/
extern bool statsAllocRepair(uint32_t numEntries);

/*Allocate Construct Stats*/
extern bool statsAllocConstruct(uint32_t numEntries);

/*******************************************************************************
*		Load stats functions
*******************************************************************************/
void loadStats(WzConfig &json, BASE_STATS *psStats, int index);

/* Return the number of newlines in a file buffer */
extern uint32_t numCR(const char *pFileBuffer, uint32_t fileSize);

/*Load the weapon stats from the file exported from Access*/
extern bool loadWeaponStats(const char *pFileName);

/*Load the body stats from the file exported from Access*/
extern bool loadBodyStats(const char *pFileName);

/*Load the brain stats from the file exported from Access*/
extern bool loadBrainStats(const char *pFileName);

/*Load the propulsion stats from the file exported from Access*/
extern bool loadPropulsionStats(const char *pFileName);

/*Load the sensor stats from the file exported from Access*/
extern bool loadSensorStats(const char *pFileName);

/*Load the ecm stats from the file exported from Access*/
extern bool loadECMStats(const char *fileName);

/*Load the repair stats from the file exported from Access*/
extern bool loadRepairStats(const char *pFileName);

/*Load the construct stats from the file exported from Access*/
extern bool loadConstructStats(const char *pFileName);

/*Load the Propulsion Types from the file exported from Access*/
extern bool loadPropulsionTypes(const char *pFileName);

/*Load the propulsion sounds from the file exported from Access*/
extern bool loadPropulsionSounds(const char *pFileName);

/*Load the Terrain Table from the file exported from Access*/
extern bool loadTerrainTable(const char *pFileName);

/*Load the Weapon Effect Modifiers from the file exported from Access*/
extern bool loadWeaponModifiers(const char *pFileName);

/*******************************************************************************
*		Generic stats functions
*******************************************************************************/

/*calls the STATS_DEALLOC macro for each set of stats*/
extern bool statsShutDown(void);

extern uint32_t getSpeedFactor(uint32_t terrainType, uint32_t propulsionType);

/// Get the component index for a component based on the name, verifying with type.
/// It is currently identical to getCompFromID, but may not be in the future.
int getCompFromName(COMPONENT_TYPE compType, const QString &name);

/// This function only allows you to use the old, deprecated ID name.
int getCompFromID(COMPONENT_TYPE compType, const QString &name);

/// Get the component pointer for a component based on the name
COMPONENT_STATS *getCompStatsFromName(const QString &name);

/*returns the weapon sub class based on the string name passed in */
extern bool getWeaponSubClass(const char *subClass, WEAPON_SUBCLASS *wclass);
const char *getWeaponSubClass(WEAPON_SUBCLASS wclass);
/*sets the store to the body size based on the name passed in - returns false
if doesn't compare with any*/
extern bool getBodySize(const char *pSize, BODY_SIZE *pStore);

/**
 * Determines the propulsion type indicated by the @c typeName string passed
 * in.
 *
 * @param typeName  name of the propulsion type to determine the enumerated
 *                  constant for.
 * @param[out] type Will contain an enumerated constant representing the given
 *                  propulsion type, if successful (as indicated by the return
 *                  value).
 *
 * @return true if successful, false otherwise. If successful, @c *type will
 *         contain a valid propulsion type enumerator, otherwise its value will
 *         be left unchanged.
 */
extern bool getPropulsionType(const char *typeName, PROPULSION_TYPE *type);

/**
 * Determines the weapon effect indicated by the @c weaponEffect string passed
 * in.
 *
 * @param weaponEffect name of the weapon effect to determine the enumerated
 *                     constant for.
 * @param[out] effect  Will contain an enumerated constant representing the
 *                     given weapon effect, if successful (as indicated by the
 *                     return value).
 *
 * @return true if successful, false otherwise. If successful, @c *effect will
 *         contain a valid weapon effect enumerator, otherwise its value will
 *         be left unchanged.
 */
extern const StringToEnumMap<WEAPON_EFFECT> map_WEAPON_EFFECT;

WZ_DECL_PURE int weaponROF(const WEAPON_STATS *psStat, int player);
WZ_DECL_PURE int weaponFirePause(const WEAPON_STATS *psStats, int player);
WZ_DECL_PURE int weaponReloadTime(const WEAPON_STATS *psStats, int player);
WZ_DECL_PURE int weaponShortHit(const WEAPON_STATS *psStats, int player);
WZ_DECL_PURE int weaponLongHit(const WEAPON_STATS *psStats, int player);
WZ_DECL_PURE int weaponDamage(const WEAPON_STATS *psStats, int player);
WZ_DECL_PURE int weaponRadDamage(const WEAPON_STATS *psStats, int player);
WZ_DECL_PURE int weaponPeriodicalDamage(const WEAPON_STATS *psStats, int player);
WZ_DECL_PURE int sensorRange(const SENSOR_STATS *psStats, int player);
WZ_DECL_PURE int ecmRange(const ECM_STATS *psStats, int player);
WZ_DECL_PURE int repairPoints(const REPAIR_STATS *psStats, int player);
WZ_DECL_PURE int constructorPoints(const CONSTRUCT_STATS *psStats, int player);
WZ_DECL_PURE int bodyPower(const BODY_STATS *psStats, int player);
WZ_DECL_PURE int bodyArmour(const BODY_STATS *psStats, int player, WEAPON_CLASS weaponClass);

extern void adjustMaxDesignStats(void);

//Access functions for the max values to be used in the Design Screen
extern uint32_t getMaxComponentWeight(void);
extern uint32_t getMaxBodyArmour(void);
extern uint32_t getMaxBodyPower(void);
extern uint32_t getMaxBodyPoints(void);
extern uint32_t getMaxSensorRange(void);
extern uint32_t getMaxECMRange(void);
extern uint32_t getMaxConstPoints(void);
extern uint32_t getMaxRepairPoints(void);
extern uint32_t getMaxWeaponRange(void);
extern uint32_t getMaxWeaponDamage(void);
extern uint32_t getMaxWeaponROF(void);
extern uint32_t getMaxPropulsionSpeed(void);

WZ_DECL_PURE bool objHasWeapon(const BASE_OBJECT *psObj);

extern void statsInitVars(void);

bool getWeaponEffect(const char *weaponEffect, WEAPON_EFFECT *effect);
bool getWeaponClass(QString weaponClassStr, WEAPON_CLASS *weaponClass);

/* Wrappers */

/** If object is an active radar (has sensor turret), then return a pointer to its sensor stats. If not, return NULL. */
WZ_DECL_PURE SENSOR_STATS *objActiveRadar(const BASE_OBJECT *psObj);

/** Returns whether object has a radar detector sensor. */
WZ_DECL_PURE bool objRadarDetector(const BASE_OBJECT *psObj);

#endif // __INCLUDED_SRC_STATS_H__
