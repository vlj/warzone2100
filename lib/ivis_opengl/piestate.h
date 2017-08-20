/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2017  Warzone 2100 Project

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
/***************************************************************************/
/*
 * pieState.h
 *
 * render State controlr all pumpkin image library functions.
 *
 */
/***************************************************************************/

#ifndef _piestate_h
#define _piestate_h

/***************************************************************************/

#include <string>
#include <vector>

#include "lib/framework/frame.h"
#include "lib/framework/vector.h"
#include "lib/framework/opengl.h"
#include "lib/ivis_opengl/gfx_api.h"
#include <glm/gtc/type_ptr.hpp>
#include "piedef.h"

struct iIMDShape;

/***************************************************************************/
/*
 *	Global Definitions
 */
/***************************************************************************/

struct RENDER_STATE
{
	bool				fogEnabled;
	bool				fog;
	PIELIGHT			fogColour;
	float				fogBegin;
	float				fogEnd;
	SDWORD				texPage;
	REND_MODE			rendMode;
};

void rendStatesRendModeHack();  // Sets rendStates.rendMode = REND_ALPHA; (Added during merge, since the renderStates is now static.)

/***************************************************************************/
/*
 *	Global ProtoTypes
 */
/***************************************************************************/
void pie_SetDefaultStates();//Sets all states
//fog available
void pie_EnableFog(bool val);
bool pie_GetFogEnabled();
//fog currently on
void pie_SetFogStatus(bool val);
bool pie_GetFogStatus();
void pie_SetFogColour(PIELIGHT colour);
PIELIGHT pie_GetFogColour() WZ_DECL_PURE;
void pie_UpdateFogDistance(float begin, float end);
//render states
RENDER_STATE getCurrentRenderState();

int pie_GetMaxAntialiasing();

bool pie_LoadShaders();
void pie_FreeShaders();
SHADER_MODE pie_LoadShader(const char *programName, const char *vertexPath, const char *fragmentPath,
	const std::vector<std::string> &);

namespace pie_internal
{
	struct SHADER_PROGRAM
	{
		GLuint program;

		// Uniforms
		std::vector<GLint> locations;

		// Attributes
		GLint locVertex;
		GLint locNormal;
		GLint locTexCoord;
		GLint locColor;
	};

	extern std::vector<SHADER_PROGRAM> shaderProgram;
	extern gfx_api::buffer* rectBuffer;
}

void pie_SetShaderStretchDepth(float stretch);
float pie_GetShaderStretchDepth();
void pie_SetShaderTime(uint32_t shaderTime);
float pie_GetShaderTime();
void pie_SetShaderEcmEffect(bool value);
int pie_GetShaderEcmEffect();

/* Errors control routine */
#ifdef DEBUG
#define glErrors() \
	_glerrors(__FUNCTION__, __FILE__, __LINE__)
#else
#define glErrors()
#endif

bool _glerrors(const char *, const char *, int);

#endif // _pieState_h
