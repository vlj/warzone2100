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
 *  Renderer setup and state control routines for 3D rendering.
 */
#include <QString>
#include "lib/gamelib/frame.h"
#include "gl/gl_impl.h"
#include <physfs.h>

#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/piestate.h"
#include "lib/ivis_opengl/piedef.h"
#include "lib/ivis_opengl/tex.h"
#include "lib/ivis_opengl/piepalette.h"
#include "screen.h"
#include "pieclip.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>

#ifndef GLEW_VERSION_4_3
#define GLEW_VERSION_4_3 false
#endif
#ifndef GLEW_KHR_debug
#define GLEW_KHR_debug false
#endif

/*
 *	Global Variables
 */

static GLfloat shaderStretch = 0;
unsigned int pieStateCount = 0; // Used in pie_GetResetCounts
static RENDER_STATE rendStates;
static GLint ecmState = 0;
static GLfloat timeState = 0.0f;

void rendStatesRendModeHack()
{
	rendStates.rendMode = REND_ALPHA;
}

/*
 *	Source
 */

void pie_SetDefaultStates(void)//Sets all states
{
	PIELIGHT black;

	//fog off
	rendStates.fogEnabled = false;// enable fog before renderer
	rendStates.fog = false;//to force reset to false
	pie_SetFogStatus(false);
	black.rgba = 0;
	black.byte.a = 255;
	pie_SetFogColour(black);

	//depth Buffer on
	pie_SetDepthBufferStatus(DEPTH_CMP_LEQ_WRT_ON);

	rendStates.rendMode = REND_ALPHA;	// to force reset to REND_OPAQUE
	pie_SetRendMode(REND_OPAQUE);
}

//***************************************************************************
//
// pie_EnableFog(bool val)
//
// Global enable/disable fog to allow fog to be turned of ingame
//
//***************************************************************************
void pie_EnableFog(bool val)
{
	if (rendStates.fogEnabled != val)
	{
		debug(LOG_FOG, "pie_EnableFog: Setting fog to %s", val ? "ON" : "OFF");
		rendStates.fogEnabled = val;
		if (val)
		{
			pie_SetFogColour(WZCOL_FOG);
		}
		else
		{
			pie_SetFogColour(WZCOL_BLACK); // clear background to black
		}
	}
}

bool pie_GetFogEnabled(void)
{
	return rendStates.fogEnabled;
}

//***************************************************************************
//
// pie_SetFogStatus(bool val)
//
// Toggle fog on and off for rendering objects inside or outside the 3D world
//
//***************************************************************************
bool pie_GetFogStatus(void)
{
	return rendStates.fog;
}

void pie_SetFogColour(PIELIGHT colour)
{
	rendStates.fogColour = colour;
}

PIELIGHT pie_GetFogColour(void)
{
	return rendStates.fogColour;
}

gfx_api::program& get_shader(const SHADER_MODE& shader)
{
	static std::vector<std::unique_ptr<gfx_api::program>> shaderProgram = gfx_api::context::get_context().build_programs();
	return *shaderProgram[shader];
}

void pie_DeactivateShader()
{
	//pie_internal::currentShaderMode = SHADER_NONE;
	glUseProgram(0);
}

void pie_SetShaderTime(uint32_t shaderTime)
{
	uint32_t base = shaderTime % 1000;
	if (base > 500)
	{
		base = 1000 - base;	// cycle
	}
	timeState = (GLfloat)base / 1000.0f;
}

void pie_SetShaderEcmEffect(bool value)
{
	ecmState = (int)value;
}

void pie_SetShaderStretchDepth(float stretch)
{
	shaderStretch = stretch;
}

float pie_GetShaderStretchDepth()
{
	return shaderStretch;
}

void pie_SetDepthBufferStatus(DEPTH_MODE depthMode)
{
	switch (depthMode)
	{
	case DEPTH_CMP_LEQ_WRT_ON:
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);
		glDepthMask(GL_TRUE);
		break;

	case DEPTH_CMP_ALWAYS_WRT_ON:
		glDisable(GL_DEPTH_TEST);
		glDepthMask(GL_TRUE);
		break;

	case DEPTH_CMP_ALWAYS_WRT_OFF:
		glDisable(GL_DEPTH_TEST);
		glDepthMask(GL_FALSE);
		break;
	}
}

/// Set the depth (z) offset
/// Negative values are closer to the screen
void pie_SetDepthOffset(float offset)
{
	if (offset == 0.0f)
	{
		glDisable(GL_POLYGON_OFFSET_FILL);
	}
	else
	{
		glPolygonOffset(offset, offset);
		glEnable(GL_POLYGON_OFFSET_FILL);
	}
}

/// Set the OpenGL fog start and end
void pie_UpdateFogDistance(float begin, float end)
{
	rendStates.fogBegin = begin;
	rendStates.fogEnd = end;
}

//
// pie_SetFogStatus(bool val)
//
// Toggle fog on and off for rendering objects inside or outside the 3D world
//
void pie_SetFogStatus(bool val)
{
	if (rendStates.fogEnabled)
	{
		//fog enabled so toggle if required
		rendStates.fog = val;
	}
	else
	{
		rendStates.fog = false;
	}
}

/** Selects a texture page and binds it for the current texture unit
 *  \param num a number indicating the texture page to bind. If this is a
 *         negative value (doesn't matter what value) it will disable texturing.
 */
void pie_SetTexturePage(int32_t num)
{
	// Only bind textures when they're not bound already
	if (num != rendStates.texPage)
	{
		switch (num)
		{
		case TEXPAGE_NONE:
			glDisable(GL_TEXTURE_2D);
			break;
		case TEXPAGE_EXTERN:
			// GLC will set the texture, we just need to enable texturing
			glEnable(GL_TEXTURE_2D);
			break;
		default:
			if (rendStates.texPage == TEXPAGE_NONE)
			{
				glEnable(GL_TEXTURE_2D);
			}
			glBindTexture(GL_TEXTURE_2D, dynamic_cast<gfx_api::gl_api::gl_texture&>(pie_Texture(num)).object);
		}
		rendStates.texPage = num;
	}
}

void pie_SetRendMode(REND_MODE rendMode)
{
	if (rendMode != rendStates.rendMode)
	{
		rendStates.rendMode = rendMode;
		switch (rendMode)
		{
		case REND_OPAQUE:
			glDisable(GL_BLEND);
			break;

		case REND_ALPHA:
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			break;

		case REND_ADDITIVE:
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE);
			break;

		case REND_MULTIPLICATIVE:
			glEnable(GL_BLEND);
			glBlendFunc(GL_ZERO, GL_SRC_COLOR);
			break;

		case REND_PREMULTIPLIED:
			glEnable(GL_BLEND);
			glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
			break;

		case REND_TEXT:
			glEnable(GL_BLEND);
			glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_COLOR);
			break;

		default:
			ASSERT(false, "Bad render state");
			break;
		}
	}
	return;
}

RENDER_STATE getCurrentRenderState()
{
	return rendStates;
}

bool _glerrors(const char *function, const char *file, int line)
{
	bool ret = false;
	GLenum err = glGetError();
	while (err != GL_NO_ERROR)
	{
		ret = true;
		debug(LOG_ERROR, "OpenGL error in function %s at %s:%u: %s\n", function, file, line, gluErrorString(err));
		err = glGetError();
	}
	return ret;
}
