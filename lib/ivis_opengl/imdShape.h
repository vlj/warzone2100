#pragma once

#include <vector>

#include "lib/framework/vector.h"
#include "lib/framework/opengl.h"
#include "pietypes.h"

//*************************************************************************
//
// imd structures
//
//*************************************************************************

/// Stores the from and to verticles from an edge
struct EDGE
{
	int from, to;
};

struct ANIMFRAME
{
	Position pos;
	Rotation rot;
	Vector3f scale;
};

struct iIMDPoly
{
	uint32_t flags;
	int32_t zcentre;
	Vector3f normal;
	int pindex[3];
	Vector2f *texCoord;
	Vector2f texAnim;
};

enum ANIMATION_EVENTS
{
	ANIM_EVENT_NONE,
	ANIM_EVENT_ACTIVE,
	ANIM_EVENT_FIRING, // should not be combined with fire-on-move, as this will look weird
	ANIM_EVENT_DYING,
	ANIM_EVENT_COUNT
};


struct iIMDShape
{
	iIMDShape();
	~iIMDShape();

	unsigned int flags;
	int texpage;
	int tcmaskpage;
	int normalpage;
	int specularpage;
	int sradius, radius;
	Vector3i min, max;

	Vector3f ocen;
	unsigned short numFrames;
	unsigned short animInterval;

	unsigned int nconnectors;
	Vector3i *connectors;

	unsigned int nShadowEdges;
	EDGE *shadowEdgeList;

	// The old rendering data
	unsigned int npoints;
	Vector3f *points;
	unsigned int npolys;
	iIMDPoly *polys;

	// The new rendering data
	GLuint buffers[VBO_COUNT];
	SHADER_MODE shaderProgram; // if using specialized shader for this model

							   // object animation (animating a level, rather than its texture)
	std::vector<ANIMFRAME> objanimdata;
	int objanimframes;

	// more object animation, but these are only set for the first level
	int objanimtime; ///< total time to render all animation frames
	int objanimcycles; ///< Number of cycles to render, zero means infinitely many
	iIMDShape *objanimpie[ANIM_EVENT_COUNT];

	iIMDShape *next;  // next pie in multilevel pies (NULL for non multilevel !)
};


