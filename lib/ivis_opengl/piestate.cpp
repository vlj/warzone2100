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
/** \file
 *  Renderer setup and state control routines for 3D rendering.
 */
#include "lib/framework/frame.h"

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

std::vector<pie_internal::SHADER_PROGRAM> pie_internal::shaderProgram;
static float shaderStretch = 0;
gfx_api::buffer* pie_internal::rectBuffer = nullptr;
static RENDER_STATE rendStates;
static int32_t ecmState = 0;
static float timeState = 0.0f;

void rendStatesRendModeHack()
{
	rendStates.rendMode = REND_ALPHA;
}

/*
 *	Source
 */

void pie_SetDefaultStates()//Sets all states
{
	PIELIGHT black;

	//fog off
	rendStates.fogEnabled = false;// enable fog before renderer
	rendStates.fog = false;//to force reset to false
	pie_SetFogStatus(false);
	black.rgba = 0;
	black.byte.a = 255;
	pie_SetFogColour(black);

	rendStates.rendMode = REND_ALPHA;	// to force reset to REND_OPAQUE
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

bool pie_GetFogEnabled()
{
	return rendStates.fogEnabled;
}

bool pie_GetFogStatus()
{
	return rendStates.fog;
}

void pie_SetFogColour(PIELIGHT colour)
{
	rendStates.fogColour = colour;
}

PIELIGHT pie_GetFogColour()
{
	return rendStates.fogColour;
}

// Read shader into text buffer
static char *readShaderBuf(const char *name)
{
	PHYSFS_file	*fp;
	int	filesize;
	char *buffer;

	fp = PHYSFS_openRead(name);
	debug(LOG_3D, "Reading...[directory: %s] %s", PHYSFS_getRealDir(name), name);
	ASSERT_OR_RETURN(nullptr, fp != nullptr, "Could not open %s", name);
	filesize = PHYSFS_fileLength(fp);
	buffer = (char *)malloc(filesize + 1);
	if (buffer)
	{
		PHYSFS_read(fp, buffer, 1, filesize);
		buffer[filesize] = '\0';
	}
	PHYSFS_close(fp);

	return buffer;
}

// Retrieve shader compilation errors
static void printShaderInfoLog(code_part part, GLuint shader)
{
	GLint infologLen = 0;

	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infologLen);
	if (infologLen > 0)
	{
		GLint charsWritten = 0;
		GLchar *infoLog = (GLchar *)malloc(infologLen);

		glGetShaderInfoLog(shader, infologLen, &charsWritten, infoLog);
		debug(part, "Shader info log: %s", infoLog);
		free(infoLog);
	}
}

// Retrieve shader linkage errors
static void printProgramInfoLog(code_part part, GLuint program)
{
	GLint infologLen = 0;

	glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infologLen);
	if (infologLen > 0)
	{
		GLint charsWritten = 0;
		GLchar *infoLog = (GLchar *)malloc(infologLen);

		glGetProgramInfoLog(program, infologLen, &charsWritten, infoLog);
		debug(part, "Program info log: %s", infoLog);
		free(infoLog);
	}
}

static void getLocs(pie_internal::SHADER_PROGRAM *program)
{
	glUseProgram(program->program);

	// Attributes
	program->locVertex = glGetAttribLocation(program->program, "vertex");
	program->locNormal = glGetAttribLocation(program->program, "vertexNormal");
	program->locTexCoord = glGetAttribLocation(program->program, "vertexTexCoord");
	program->locColor = glGetAttribLocation(program->program, "vertexColor");

	// Uniforms, these never change.
	GLint locTex0 = glGetUniformLocation(program->program, "Texture");
	GLint locTex1 = glGetUniformLocation(program->program, "TextureTcmask");
	GLint locTex2 = glGetUniformLocation(program->program, "TextureNormal");
	GLint locTex3 = glGetUniformLocation(program->program, "TextureSpecular");
	glUniform1i(locTex0, 0);
	glUniform1i(locTex1, 1);
	glUniform1i(locTex2, 2);
	glUniform1i(locTex3, 3);
}

void pie_FreeShaders()
{
	while (pie_internal::shaderProgram.size() > SHADER_MAX)
	{
		glDeleteShader(pie_internal::shaderProgram.back().program);
		pie_internal::shaderProgram.pop_back();
	}
}

// Read/compile/link shaders
SHADER_MODE pie_LoadShader(const char *programName, const char *vertexPath, const char *fragmentPath,
	const std::vector<std::string> &uniformNames)
{
	pie_internal::SHADER_PROGRAM program;
	GLint status;
	bool success = true; // Assume overall success
	char *buffer[2];

	program.program = glCreateProgram();
	glBindAttribLocation(program.program, 0, "vertex");
	glBindAttribLocation(program.program, 1, "vertexTexCoord");
	glBindAttribLocation(program.program, 2, "vertexColor");
	glBindAttribLocation(program.program, 3, "vertexNormal");
	ASSERT_OR_RETURN(SHADER_NONE, program.program, "Could not create shader program!");

	*buffer = (char *)"";

	if (vertexPath)
	{
		success = false; // Assume failure before reading shader file

		if ((*(buffer + 1) = readShaderBuf(vertexPath)))
		{
			GLuint shader = glCreateShader(GL_VERTEX_SHADER);

			glShaderSource(shader, 2, (const char **)buffer, nullptr);
			glCompileShader(shader);

			// Check for compilation errors
			glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
			if (!status)
			{
				debug(LOG_ERROR, "Vertex shader compilation has failed [%s]", vertexPath);
				printShaderInfoLog(LOG_ERROR, shader);
			}
			else
			{
				printShaderInfoLog(LOG_3D, shader);
				glAttachShader(program.program, shader);
				success = true;
			}
			if (GLEW_VERSION_4_3 || GLEW_KHR_debug)
			{
				glObjectLabel(GL_SHADER, shader, -1, vertexPath);
			}
			free(*(buffer + 1));
		}
	}

	if (success && fragmentPath)
	{
		success = false; // Assume failure before reading shader file

		if ((*(buffer + 1) = readShaderBuf(fragmentPath)))
		{
			GLuint shader = glCreateShader(GL_FRAGMENT_SHADER);

			glShaderSource(shader, 2, (const char **)buffer, nullptr);
			glCompileShader(shader);

			// Check for compilation errors
			glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
			if (!status)
			{
				debug(LOG_ERROR, "Fragment shader compilation has failed [%s]", fragmentPath);
				printShaderInfoLog(LOG_ERROR, shader);
			}
			else
			{
				printShaderInfoLog(LOG_3D, shader);
				glAttachShader(program.program, shader);
				success = true;
			}
			if (GLEW_VERSION_4_3 || GLEW_KHR_debug)
			{
				glObjectLabel(GL_SHADER, shader, -1, fragmentPath);
			}
			free(*(buffer + 1));
		}
	}

	if (success)
	{
		glLinkProgram(program.program);

		// Check for linkage errors
		glGetProgramiv(program.program, GL_LINK_STATUS, &status);
		if (!status)
		{
			debug(LOG_ERROR, "Shader program linkage has failed [%s, %s]", vertexPath, fragmentPath);
			printProgramInfoLog(LOG_ERROR, program.program);
			success = false;
		}
		else
		{
			printProgramInfoLog(LOG_3D, program.program);
		}
		if (GLEW_VERSION_4_3 || GLEW_KHR_debug)
		{
			glObjectLabel(GL_PROGRAM, program.program, -1, programName);
		}
	}
	GLuint p = program.program;
	std::transform(uniformNames.begin(), uniformNames.end(),
		std::back_inserter(program.locations),
		[p](const std::string name) { return glGetUniformLocation(p, name.data()); });

	getLocs(&program);
	glUseProgram(0);

	pie_internal::shaderProgram.push_back(program);

	return SHADER_MODE(pie_internal::shaderProgram.size() - 1);
}

static float fogBegin;
static float fogEnd;

// Run from screen.c on init. Do not change the order of loading here! First ones are enumerated.
bool pie_LoadShaders()
{
	GLbyte rect[] {
		0, 255, 0, 255,
		0, 0, 0, 255,
		255, 255, 0, 255,
		255, 0, 0, 255
	};
	if (pie_internal::rectBuffer)
		delete pie_internal::rectBuffer;
	pie_internal::rectBuffer = gfx_api::context::get().create_buffer(gfx_api::buffer::usage::vertex_buffer, 16 * sizeof(GLbyte));
	pie_internal::rectBuffer->upload(0, 16 * sizeof(GLbyte), rect);

	return true;
}


void pie_SetShaderTime(uint32_t shaderTime)
{
	uint32_t base = shaderTime % 1000;
	if (base > 500)
	{
		base = 1000 - base;	// cycle
	}
	timeState = (float)base / 1000.0f;
}

float pie_GetShaderTime()
{
	return timeState;
}

void pie_SetShaderEcmEffect(bool value)
{
	ecmState = (int)value;
}

int pie_GetShaderEcmEffect()
{
	return ecmState;
}

void pie_SetShaderStretchDepth(float stretch)
{
	shaderStretch = stretch;
}

float pie_GetShaderStretchDepth()
{
	return shaderStretch;
}

/// Set the OpenGL fog start and end
void pie_UpdateFogDistance(float begin, float end)
{
	rendStates.fogBegin = begin;
	rendStates.fogEnd = end;
}

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

int pie_GetMaxAntialiasing()
{
	GLint maxSamples = 0;
	glGetIntegerv(GL_MAX_SAMPLES, &maxSamples);
	return maxSamples;
}
