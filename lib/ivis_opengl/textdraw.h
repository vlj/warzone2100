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

#ifndef _INCLUDED_TEXTDRAW_
#define _INCLUDED_TEXTDRAW_

#include "ivisdef.h"
#include "piepalette.h"

#ifdef WZ_OS_MAC
# include <CoreFoundation/CoreFoundation.h>
# include <CoreFoundation/CFURL.h>
# include <QuesoGLC/glc.h>
#else
#ifdef WZ_OS_WIN
#define GLCAPI extern
#endif
# include <GL/glc.h>
#endif
#include <array>

namespace iV
{
enum class fonts
{
	font_regular,
	font_large,
	font_medium,
	font_small,
	font_scaled,
	font_count
};

// Valid values for "Justify" argument of iV_DrawFormattedText().
enum class justification
{
	FTEXT_LEFTJUSTIFY,			// Left justify.
	FTEXT_CENTRE,				// Centre justify.
	FTEXT_RIGHTJUSTIFY,			// Right justify.
};

struct font_state
{
	std::string font_family;
	std::string font_face_regular;
	std::string font_face_bold;

	float font_size = 12.f;
	// Contains the font color in the following order: red, green, blue, alpha
	std::array<float,4> font_colour = { 1.f, 1.f, 1.f, 1.f };

	GLint _glcContext = 0;
	GLint _glcFont_Regular = 0;
	GLint _glcFont_Bold = 0;
};

extern font_state default_font_state;

void TextInit(font_state& = default_font_state);
void TextShutdown(font_state& = default_font_state);
void font(const std::string &fontName, const std::string &fontFace, const std::string &fontFaceBold, font_state& = default_font_state);
void SetFont(fonts FontID, font_state& = default_font_state);
int GetTextAboveBase(font_state& = default_font_state);
int GetTextBelowBase(font_state& = default_font_state);
int GetTextLineSize(font_state& = default_font_state);
unsigned int GetTextWidth(const std::string &string, font_state& = default_font_state);
unsigned int GetCharWidth(uint32_t charCode, font_state& = default_font_state);
unsigned int GetTextHeight(const std::string &string, font_state& = default_font_state);
void SetTextColour(PIELIGHT colour, font_state& = default_font_state);
int DrawFormattedText(const std::string &String, UDWORD x, UDWORD y, UDWORD Width, justification Justify, font_state& = default_font_state);
void SetTextSize(float size, font_state& = default_font_state);
void DrawTextRotated(const std::string &string, float x, float y, float rotation, font_state& = default_font_state);

/** Draws text with a printf syntax to the screen.
 */
inline void DrawText(const std::string &string, float x, float y)
{
	DrawTextRotated(string, x, y, 0.f, default_font_state);
}

void DrawTextF(float x, float y, const char *format, ...) WZ_DECL_FORMAT(printf, 3, 4);

} // end namespace iV

#endif // _INCLUDED_TEXTDRAW_
