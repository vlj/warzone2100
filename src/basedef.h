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
 *  Definitions for the base object type.
 */

#ifndef __INCLUDED_BASEDEF_H__
#define __INCLUDED_BASEDEF_H__

#include "displaydef.h"
#include "statsdef.h"
#include "lib/gamelib/base_object.h"

//the died flag for a droid is set to this when it gets added to the non-current list
#define NOT_CURRENT_LIST 1

/*
 Coordinate system used for objects in Warzone 2100:
  x - "right"
  y - "forward"
  z - "up"

  For explanation of yaw/pitch/roll look for "flight dynamics" in your encyclopedia.
*/


#define BASEFLAG_TARGETED  0x01 ///< Whether object is targeted by a selectedPlayer droid sensor (quite the hack)
#define BASEFLAG_DIRTY     0x02 ///< Whether certain recalculations are needed for object on frame update

/// Space-time coordinate, including orientation.
struct Spacetime
{
	Spacetime() : time(0), pos(0, 0, 0), rot(0, 0, 0) {}
	Spacetime(Position pos_, Rotation rot_, uint32_t time_) : time(time_), pos(pos_), rot(rot_) {}

	uint32_t  time;        ///< Game time

	Position  pos;         ///< Position of the object
	Rotation  rot;         ///< Rotation of the object
};

static inline Spacetime getSpacetime(SIMPLE_OBJECT const *psObj)
{
	return Spacetime(psObj->pos, psObj->rot, psObj->time);
}
static inline void setSpacetime(SIMPLE_OBJECT *psObj, Spacetime const &st)
{
	psObj->pos = st.pos;
	psObj->rot = st.rot;
	psObj->time = st.time;
}

static inline bool isDead(const BASE_OBJECT *psObj)
{
	// See objmem.c for comments on the NOT_CURRENT_LIST hack
	return (psObj->died > NOT_CURRENT_LIST);
}

static inline int objPosDiffSq(Position pos1, Position pos2)
{
	const Vector2i diff = (pos1 - pos2).xy;
	return diff * diff;
}

static inline int objPosDiffSq(SIMPLE_OBJECT const *pos1, SIMPLE_OBJECT const *pos2)
{
	return objPosDiffSq(pos1->pos, pos2->pos);
}

// True iff object is a droid, structure or feature (not a projectile). Will incorrectly return true if passed a nonsense object of type OBJ_TARGET or OBJ_NUM_TYPES.
static inline bool isBaseObject(SIMPLE_OBJECT const *psObject)
{
	return psObject != NULL && psObject->type != OBJ_PROJECTILE;
}
// Returns BASE_OBJECT * if base_object or NULL if not.
static inline BASE_OBJECT *castBaseObject(SIMPLE_OBJECT *psObject)
{
	return isBaseObject(psObject) ? (BASE_OBJECT *)psObject : (BASE_OBJECT *)NULL;
}
// Returns BASE_OBJECT const * if base_object or NULL if not.
static inline BASE_OBJECT const *castBaseObject(SIMPLE_OBJECT const *psObject)
{
	return isBaseObject(psObject) ? (BASE_OBJECT const *)psObject : (BASE_OBJECT const *)NULL;
}

// Must be #included __AFTER__ the definition of BASE_OBJECT
#include "baseobject.h"

#endif // __INCLUDED_BASEDEF_H__
