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
 *  structures required for research stats
 */

#ifndef __INCLUDED_SRC_RESEARCH_H__
#define __INCLUDED_SRC_RESEARCH_H__

#include "objectdef.h"

#define NO_RESEARCH_ICON 0
//max 'research complete' console message length
#define MAX_RESEARCH_MSG_SIZE 200


//used for loading in the research stats into the appropriate list
enum
{
	REQ_LIST,
	RED_LIST,
	RES_LIST
};


enum
{
	RID_ROCKET,
	RID_CANNON,
	RID_HOVERCRAFT,
	RID_ECM,
	RID_PLASCRETE,
	RID_TRACKS,
	RID_DROIDTECH,
	RID_WEAPONTECH,
	RID_COMPUTERTECH,
	RID_POWERTECH,
	RID_SYSTEMTECH,
	RID_STRUCTURETECH,
	RID_CYBORGTECH,
	RID_DEFENCE,
	RID_QUESTIONMARK,
	RID_GRPACC,
	RID_GRPUPG,
	RID_GRPREP,
	RID_GRPROF,
	RID_GRPDAM,
	RID_MAXRID
};


/* The store for the research stats */
extern std::vector<RESEARCH> asResearch;

//List of pointers to arrays of PLAYER_RESEARCH[numResearch] for each player
extern std::vector<PLAYER_RESEARCH> asPlayerResList[MAX_PLAYERS];

//used for Callbacks to say which topic was last researched
extern RESEARCH				*psCBLastResearch;
extern STRUCTURE			*psCBLastResStructure;
extern int32_t				CBResFacilityOwner;

/* Default level of sensor, repair and ECM */
extern uint32_t	aDefaultSensor[MAX_PLAYERS];
extern uint32_t	aDefaultECM[MAX_PLAYERS];
extern uint32_t	aDefaultRepair[MAX_PLAYERS];

//extern bool loadResearch(void);
extern bool loadResearch(QString filename);

/*function to check what can be researched for a particular player at any one
  instant. Returns the number to research*/
//extern uint8_t fillResearchList(uint8_t *plist, uint32_t playerID, uint16_t topic,
//							   uint16_t limit);
//needs to be uint16_t sized for Patches
extern uint16_t fillResearchList(uint16_t *plist, uint32_t playerID, uint16_t topic,
                              uint16_t limit);

/* process the results of a completed research topic */
extern void researchResult(uint32_t researchIndex, uint8_t player, bool bDisplay, STRUCTURE *psResearchFacility, bool bTrigger);

//this just inits all the research arrays
extern bool ResearchShutDown(void);
//this free the memory used for the research
extern void ResearchRelease(void);

/* For a given view data get the research this is related to */
extern RESEARCH *getResearch(const char *pName);

/* sets the status of the topic to cancelled and stores the current research
   points accquired */
extern void cancelResearch(STRUCTURE *psBuilding, QUEUE_MODE mode);

/* For a given view data get the research this is related to */
struct VIEWDATA;
RESEARCH *getResearchForMsg(VIEWDATA *pViewData);

/* Sets the 'possible' flag for a player's research so the topic will appear in
the research list next time the Research Facilty is selected */
extern bool enableResearch(RESEARCH *psResearch, uint32_t player);

/*find the last research topic of importance that the losing player did and
'give' the results to the reward player*/
extern void researchReward(uint8_t losingPlayer, uint8_t rewardPlayer);

/*check to see if any research has been completed that enables self repair*/
extern bool selfRepairEnabled(uint8_t player);

extern int32_t	mapRIDToIcon(uint32_t rid);
extern int32_t	mapIconToRID(uint32_t iconID);

/*puts research facility on hold*/
extern void holdResearch(STRUCTURE *psBuilding, QUEUE_MODE mode);
/*release a research facility from hold*/
extern void releaseResearch(STRUCTURE *psBuilding, QUEUE_MODE mode);

/*checks the stat to see if its of type wall or defence*/
extern bool wallDefenceStruct(STRUCTURE_STATS *psStats);

extern void enableSelfRepair(uint8_t player);

void CancelAllResearch(uint32_t pl);

extern bool researchInitVars(void);

bool researchAvailable(int inc, int playerID, QUEUE_MODE mode);

struct AllyResearch
{
	unsigned player;
	int completion;
	int powerNeeded;
	int timeToResearch;
	bool active;
};
std::vector<AllyResearch> const &listAllyResearch(unsigned ref);

#endif // __INCLUDED_SRC_RESEARCH_H__
