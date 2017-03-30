#pragma once

#include "lib/framework/types.h"
#include "lib/framework/vector.h"
#include "weapondef.h"
#include "tile.h"

enum OBJECT_TYPE
{
	OBJ_DROID,      ///< Droids
	OBJ_STRUCTURE,  ///< All Buildings
	OBJ_FEATURE,    ///< Things like roads, trees, bridges, fires
	OBJ_PROJECTILE, ///< Comes out of guns, stupid :-)
	OBJ_TARGET,     ///< for the camera tracking
	OBJ_NUM_TYPES,  ///< number of object types - MUST BE LAST
};

struct SIMPLE_OBJECT
{
	SIMPLE_OBJECT(OBJECT_TYPE type, uint32_t id, unsigned player);
	virtual ~SIMPLE_OBJECT();

	const OBJECT_TYPE type;                         ///< The type of object
	uint32_t          id;                             ///< ID number of the object
	Position        pos;                            ///< Position of the object
	Rotation        rot;                            ///< Object's yaw +ve rotation around up-axis
	uint8_t           player;                         ///< Which player the object belongs to
	uint32_t          born;                           ///< Time the game object was born
	uint32_t          died;                           ///< When an object was destroyed, if 0 still alive
	uint32_t        time;                           ///< Game time of given space-time position.
};

/// NEXTOBJ is an ugly hack to avoid having to fix all occurences of psNext and psNextFunc. The use of the original NEXTOBJ(pointerType) hack wasn't valid C, so in that sense, it's an improvement.
/// NEXTOBJ is a BASE_OBJECT *, which can automatically be cast to DROID *, STRUCTURE * and FEATURE *...

struct BASE_OBJECT;

struct NEXTOBJ
{
	NEXTOBJ(BASE_OBJECT *ptr_ = NULL) : ptr(ptr_) {}
	NEXTOBJ &operator =(BASE_OBJECT *ptr_)
	{
		ptr = ptr_;
		return *this;
	}
	template<class T>
	operator T *() const
	{
		return static_cast<T *>(ptr);
	}

	BASE_OBJECT *ptr;
};

#define MAX_WEAPONS 3

struct BASE_OBJECT : public SIMPLE_OBJECT
{
	BASE_OBJECT(OBJECT_TYPE type, uint32_t id, unsigned player);
	~BASE_OBJECT();

	SCREEN_DISP_DATA    sDisplay;                   ///< screen coordinate details
	uint8_t               group;                      ///< Which group selection is the droid currently in?
	uint8_t               selected;                   ///< Whether the object is selected (might want this elsewhere)
	uint8_t               cluster;                    ///< Which cluster the object is a member of
	uint8_t               visible[MAX_PLAYERS];       ///< Whether object is visible to specific player
	uint8_t               seenThisTick[MAX_PLAYERS];  ///< Whether object has been seen this tick by the specific player.
	uint16_t               numWatchedTiles;            ///< Number of watched tiles, zero for features
	uint32_t              lastEmission;               ///< When did it last puff out smoke?
	WEAPON_SUBCLASS     lastHitWeapon;              ///< The weapon that last hit it
	uint32_t              timeLastHit;                ///< The time the structure was last attacked
	uint32_t              body;                       ///< Hit points with lame name
	uint32_t              periodicalDamageStart;                  ///< When the object entered the fire
	uint32_t              periodicalDamage;                 ///< How much damage has been done since the object entered the fire
	uint16_t            flags;                      ///< Various flags
	TILEPOS             *watchedTiles;              ///< Variable size array of watched tiles, NULL for features

	uint32_t              timeAnimationStarted;       ///< Animation start time, zero for do not animate
	uint8_t               animationEvent;             ///< If animation start time > 0, this points to which animation to run

	unsigned            numWeaps;
	WEAPON              asWeaps[MAX_WEAPONS];

	NEXTOBJ             psNext;                     ///< Pointer to the next object in the object list
	NEXTOBJ             psNextFunc;                 ///< Pointer to the next object in the function list
};


