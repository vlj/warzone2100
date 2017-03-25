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
/** \file
 *  Definitions for droids.
 */

#ifndef __INCLUDED_DROIDDEF_H__
#define __INCLUDED_DROIDDEF_H__

#include <vector>

#include "stringdef.h"
#include "actiondef.h"
#include "basedef.h"
#include "movedef.h"
#include "orderdef.h"
#include "statsdef.h"
#include "weapondef.h"
#include "orderdef.h"
#include "actiondef.h"

/*!
 * The number of components in the asParts / asBits arrays.
 * Weapons are stored seperately, thus the maximum index into the array
 * is 1 smaller than the number of components.
 */
#define DROID_MAXCOMP (COMP_NUMCOMPONENTS - 1)

/* The maximum number of droid weapons */
#define	DROID_DAMAGE_SCALING	400
// This should really be logarithmic
#define	CALC_DROID_SMOKE_INTERVAL(x) ((((100-x)+10)/10) * DROID_DAMAGE_SCALING)

//defines how many times to perform the iteration on looking for a blank location
#define LOOK_FOR_EMPTY_TILE		20

typedef std::vector<DROID_ORDER_DATA> OrderList;

struct DROID_TEMPLATE : public BASE_STATS
{
	DROID_TEMPLATE();

	/*!
	 * The droid components.
	 *
	 * This array is indexed by COMPONENT_TYPE so the ECM would be accessed
	 * using asParts[COMP_ECM].
	 *
	 * Weapons are stored in asWeaps, _not_ here at index COMP_WEAPON! (Which is the reason we do not have a COMP_NUMCOMPONENTS sized array here.)
	 */
	uint8_t         asParts[DROID_MAXCOMP];
	/* The weapon systems */
	int8_t          numWeaps;                   ///< Number of weapons
	uint8_t         asWeaps[MAX_WEAPONS];       ///< weapon indices
	DROID_TYPE      droidType;                  ///< The type of droid
	uint32_t          multiPlayerID;              ///< multiplayer unique descriptor(cant use id's for templates). Used for save games as well now - AB 29/10/98
	bool            prefab;                     ///< Not player designed, not saved, never delete or change
	bool            stored;                     ///< Stored template
	bool            enabled;                    ///< Has been enabled
};

class DROID_GROUP;
struct STRUCTURE;

struct DROID : public BASE_OBJECT
{
	DROID(uint32_t id, unsigned player);
	~DROID();

	/// UTF-8 name of the droid. This is generated from the droid template
	///  WARNING: This *can* be changed by the game player after creation & can be translated, do NOT rely on this being the same for everyone!
	char            aName[MAX_STR_LENGTH];
	DROID_TYPE      droidType;                      ///< The type of droid
	/** Holds the specifics for the component parts - allows damage
	 *  per part to be calculated. Indexed by COMPONENT_TYPE.
	 *  Weapons need to be dealt with separately.
	 */
	uint8_t         asBits[DROID_MAXCOMP];
	/* The other droid data.  These are all derived from the components
	 * but stored here for easy access
	 */
	uint32_t          weight;
	uint32_t          baseSpeed;                      ///< the base speed dependant on propulsion type
	uint32_t          originalBody;                   ///< the original body points
	uint32_t        experience;
	uint32_t          lastFrustratedTime;             ///< Set when eg being stuck; used for eg firing indiscriminately at map features to clear the way
	int16_t           resistance;                     ///< used in Electronic Warfare
	// The group the droid belongs to
	DROID_GROUP    *psGroup;
	DROID          *psGrpNext;
	STRUCTURE      *psBaseStruct;                   ///< a structure that this droid might be associated with. For VTOLs this is the rearming pad
	// queued orders
	int32_t          listSize;                       ///< Gives the number of synchronised orders. Orders from listSize to the real end of the list may not affect game state.
	OrderList       asOrderList;                    ///< The range [0; listSize - 1] corresponds to synchronised orders, and the range [listPendingBegin; listPendingEnd - 1] corresponds to the orders that will remain, once all orders are synchronised.
	unsigned        listPendingBegin;               ///< Index of first order which will not be erased by a pending order. After all messages are processed, the orders in the range [listPendingBegin; listPendingEnd - 1] will remain.
	/* Order data */
	DROID_ORDER_DATA order;

#ifdef DEBUG
	// these are to help tracking down dangling pointers
	char            targetFunc[MAX_EVENT_NAME_LEN];
	int             targetLine;
	char            actionTargetFunc[MAX_WEAPONS][MAX_EVENT_NAME_LEN];
	int             actionTargetLine[MAX_WEAPONS];
	char            baseFunc[MAX_EVENT_NAME_LEN];
	int             baseLine;
#endif

	// secondary order data
	uint32_t          secondaryOrder;
	uint32_t        secondaryOrderPending;          ///< What the secondary order will be, after synchronisation.
	int             secondaryOrderPendingCount;     ///< Number of pending secondary order changes.

	/* Action data */
	DROID_ACTION    action;
	Vector2i        actionPos;
	BASE_OBJECT    *psActionTarget[MAX_WEAPONS]; ///< Action target object
	uint32_t          actionStarted;                  ///< Game time action started
	uint32_t          actionPoints;                   ///< number of points done by action since start
	uint32_t          expectedDamageDirect;                 ///< Expected damage to be caused by all currently incoming direct projectiles. This info is shared between all players,
	uint32_t          expectedDamageIndirect;                 ///< Expected damage to be caused by all currently incoming indirect projectiles. This info is shared between all players,
	///< but shouldn't make a difference unless 3 mutual enemies happen to be fighting each other at the same time.
	uint8_t           illumination;
	/* Movement control data */
	MOVE_CONTROL    sMove;
	Spacetime       prevSpacetime;                  ///< Location of droid in previous tick.
	uint8_t         blockedBits;                    ///< Bit set telling which tiles block this type of droid (TODO)
	/* anim data */
	int32_t          iAudioID;
};

#endif // __INCLUDED_DROIDDEF_H__
