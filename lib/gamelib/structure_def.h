#pragma once
#include "base_object.h"

/* Defines for indexing an appropriate IMD object given a buildings purpose. */
enum STRUCTURE_TYPE
{
	REF_HQ,
	REF_FACTORY,
	REF_FACTORY_MODULE,//draw as factory 2
	REF_POWER_GEN,
	REF_POWER_MODULE,
	REF_RESOURCE_EXTRACTOR,
	REF_DEFENSE,
	REF_WALL,
	REF_WALLCORNER,				//corner wall - no gun
	REF_GENERIC,
	REF_RESEARCH,
	REF_RESEARCH_MODULE,
	REF_REPAIR_FACILITY,
	REF_COMMAND_CONTROL,		//control centre for command droids
	REF_BRIDGE,			//NOT USED, but removing it would change savegames
	REF_DEMOLISH,			//the demolish structure type - should only be one stat with this type
	REF_CYBORG_FACTORY,
	REF_VTOL_FACTORY,
	REF_LAB,
	REF_REARM_PAD,
	REF_MISSILE_SILO,
	REF_SAT_UPLINK,         //added for updates - AB 8/6/99
	REF_GATE,
	NUM_DIFF_BUILDINGS,		//need to keep a count of how many types for IMD loading
};

//this structure is used whenever an instance of a building is required in game
struct STRUCTURE : public BASE_OBJECT
{
	STRUCTURE(uint32_t id, unsigned player);
	~STRUCTURE();

	STRUCTURE_STATS     *pStructureType;            /* pointer to the structure stats for this type of building */
	STRUCT_STATES       status;                     /* defines whether the structure is being built, doing nothing or performing a function */
	uint32_t            currentBuildPts;            /* the build points currently assigned to this structure */
	int                 resistance;                 /* current resistance points, 0 = cannot be attacked electrically */
	uint32_t              lastResistance;             /* time the resistance was last increased*/
	FUNCTIONALITY       *pFunctionality;            /* pointer to structure that contains fields necessary for functionality */
	int                 buildRate;                  ///< Rate that this structure is being built, calculated each tick. Only meaningful if status == SS_BEING_BUILT. If construction hasn't started and build rate is 0, remove the structure.
	int                 lastBuildRate;              ///< Needed if wanting the buildRate between buildRate being reset to 0 each tick and the trucks calculating it.
	BASE_OBJECT *psTarget[MAX_WEAPONS];
#ifdef DEBUG
	// these are to help tracking down dangling pointers
	char targetFunc[MAX_WEAPONS][MAX_EVENT_NAME_LEN];
	int targetLine[MAX_WEAPONS];
#endif

	uint32_t expectedDamage;           ///< Expected damage to be caused by all currently incoming projectiles. This info is shared between all players,
									   ///< but shouldn't make a difference unless 3 mutual enemies happen to be fighting each other at the same time.
	uint32_t prevTime;               ///< Time of structure's previous tick.
	float foundationDepth;           ///< Depth of structure's foundation
	uint8_t capacity;                ///< Number of module upgrades
	STRUCT_ANIM_STATES	state;
	uint32_t lastStateTime;
	iIMDShape *prebuiltImd;
};

