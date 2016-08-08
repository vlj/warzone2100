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

#include "lib/framework/frame.h"
#include "lib/framework/opengl.h"
#include <stdlib.h>
#include <string.h>
#include "lib/framework/string_ext.h"
#include "lib/ivis_opengl/ivisdef.h"
#include "lib/ivis_opengl/piestate.h"
#include "lib/ivis_opengl/pieclip.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/piepalette.h"
#include "lib/ivis_opengl/textdraw.h"
#include "lib/ivis_opengl/bitimage.h"
#include "src/multiplay.h"

#define ASCII_SPACE			(32)
#define ASCII_NEWLINE			('@')
#define ASCII_COLOURMODE		('#')

namespace
{
	void (__stdcall *glcContextPtr)(GLint inContext);
	void (__stdcall *glcDeleteContextPtr)(GLint inContext);
	void (__stdcall *glcEnablePtr)(GLCenum inAttrib);
	void (__stdcall *glcFontPtr)(GLint inFont);
	GLint (__stdcall *glcGenFontIDPtr)();
	void (__stdcall *glcRenderStringPtr)(const GLCchar *inString);
	GLfloat* (__stdcall *glcGetMaxCharMetricPtr)(GLCenum inMetric,
		GLfloat *outVec);
	GLfloat* (__stdcall *glcGetStringMetricPtr)(GLCenum inMetric, GLfloat *outVec);
	GLint (__stdcall *glcMeasureStringPtr)(GLboolean inMeasureChars,
		const GLCchar *inString);
	void(__stdcall *glcDeleteFontPtr)(GLint inFont);
	GLfloat* (__stdcall *glcGetCharMetricPtr)(GLint inCode, GLCenum inMetric,
		GLfloat *outVec);
	GLfloat (__stdcall *glcGetfPtr)(GLCenum inAttrib);
	GLint (__stdcall *glcMeasureCountedStringPtr)(GLboolean inMeasureChars,
		GLint inCount,
		const GLCchar *inString);
	void (__stdcall *glcRenderStylePtr)(GLCenum inStyle);
	void (__stdcall *glcStringTypePtr)(GLCenum inStringType);
	GLboolean (__stdcall *glcFontFacePtr)(GLint inFont, const GLCchar *inFace);
	GLint (__stdcall *glcNewFontFromFamilyPtr)(GLint inFont,
		const GLCchar *inFamily);
	GLint(__stdcall *glcGenContextPtr)();

	HINSTANCE dllHandle;

#define LOAD_CALLBACK(name, symbol) name = (decltype(name))GetProcAddress(dllHandle, #symbol)

	void loadDLL()
	{
		dllHandle = LoadLibrary(L"glc32.dll");

		LOAD_CALLBACK(glcContextPtr, _glcContext@4);
		LOAD_CALLBACK(glcDeleteContextPtr, _glcDeleteContext@4);
		LOAD_CALLBACK(glcDeleteFontPtr, _glcDeleteFont@4);
		LOAD_CALLBACK(glcEnablePtr, _glcEnable@4);
		LOAD_CALLBACK(glcFontPtr, _glcFont@4);
		LOAD_CALLBACK(glcFontFacePtr, _glcFontFace@8);
		LOAD_CALLBACK(glcGenFontIDPtr, _glcGenFontID@0);
		LOAD_CALLBACK(glcGetMaxCharMetricPtr, _glcGetMaxCharMetric@8);
		LOAD_CALLBACK(glcGetStringMetricPtr, _glcGetStringMetric@8);
		LOAD_CALLBACK(glcMeasureCountedStringPtr, _glcMeasureCountedString@12);
		LOAD_CALLBACK(glcMeasureStringPtr, _glcMeasureString@8);
		LOAD_CALLBACK(glcNewFontFromFamilyPtr, _glcNewFontFromFamily@8);
		LOAD_CALLBACK(glcRenderStringPtr, _glcRenderString@4);
		LOAD_CALLBACK(glcRenderStylePtr, _glcRenderStyle@4);
		LOAD_CALLBACK(glcStringTypePtr, _glcStringType@4);
		LOAD_CALLBACK(glcGetCharMetricPtr, _glcGetCharMetric@12);
		LOAD_CALLBACK(glcGetfPtr, _glcGetf@4);
		LOAD_CALLBACK(glcGenContextPtr, _glcGenContext@0);
	}
}

extern "C"
{
	void APIENTRY glcContext(GLint inContext)
	{
		glcContextPtr(inContext);
	}

	void APIENTRY glcDeleteContext(GLint inContext)
	{
		glcDeleteContextPtr(inContext);
	}

	void APIENTRY glcEnable(GLCenum inAttrib)
	{
		glcEnablePtr(inAttrib);
	}

	void APIENTRY glcFont(GLint inFont)
	{
		glcFontPtr(inFont);
	}

	GLint APIENTRY glcGenFontID(void)
	{
		return glcGenFontIDPtr();
	}

	void APIENTRY glcRenderString(const GLCchar *inString)
	{
		glcRenderStringPtr(inString);
	}

	GLfloat* APIENTRY glcGetMaxCharMetric(GLCenum inMetric,
		GLfloat *outVec)
	{
		return glcGetMaxCharMetricPtr(inMetric, outVec);
	}

	GLfloat* APIENTRY glcGetStringMetric(GLCenum inMetric, GLfloat *outVec)
	{
		return glcGetStringMetricPtr(inMetric, outVec);
	}

	GLint APIENTRY glcMeasureString(GLboolean inMeasureChars,
		const GLCchar *inString)
	{
		return glcMeasureStringPtr(inMeasureChars, inString);
	}

	void APIENTRY glcDeleteFont(GLint inFont)
	{
		glcDeleteFontPtr(inFont);
	}

	GLfloat* APIENTRY glcGetCharMetric(GLint inCode, GLCenum inMetric,
		GLfloat *outVec)
	{
		return glcGetCharMetricPtr(inCode, inMetric, outVec);
	}

	GLfloat APIENTRY glcGetf(GLCenum inAttrib)
	{
		return glcGetfPtr(inAttrib);
	}

	GLint APIENTRY glcMeasureCountedString(GLboolean inMeasureChars,
		GLint inCount,
		const GLCchar *inString)
	{
		return glcMeasureCountedStringPtr(inMeasureChars, inCount, inString);
	}

	void APIENTRY glcRenderStyle(GLCenum inStyle)
	{
		glcRenderStylePtr(inStyle);
	}

	void APIENTRY glcStringType(GLCenum inStringType)
	{
		glcStringTypePtr(inStringType);
	}

	GLboolean APIENTRY glcFontFace(GLint inFont, const GLCchar *inFace)
	{
		return glcFontFacePtr(inFont, inFace);
	}

	GLint APIENTRY glcNewFontFromFamily(GLint inFont,
		const GLCchar *inFamily)
	{
		return glcNewFontFromFamilyPtr(inFont, inFamily);
	}

	GLint APIENTRY glcGenContext()
	{
		return glcGenContextPtr();
	}
}

iV::font_state iV::default_font_state{};

namespace
{
	void initializeGLC(iV::font_state &st)
	{
		loadDLL();

		if (st._glcContext)
		{
			return;
		}

		st._glcContext = glcGenContext();
		if (!st._glcContext)
		{
			debug(LOG_ERROR, "Failed to initialize");
		}
		else
		{
			debug(LOG_NEVER, "Successfully initialized. _glcContext = %d", (int)st._glcContext);
		}

		glcContext(st._glcContext);

		glcEnable(GLC_AUTO_FONT);		// We *do* want font fall-backs
		glcRenderStyle(GLC_TEXTURE);
		glcStringType(GLC_UTF8_QSO); // Set GLC's string type to UTF-8 FIXME should be UCS4 to avoid conversions

#ifdef WZ_OS_MAC
		{
			char resourcePath[PATH_MAX];
			CFURLRef resourceURL = CFBundleCopyResourcesDirectoryURL(CFBundleGetMainBundle());
			if (CFURLGetFileSystemRepresentation(resourceURL, true, (UInt8 *)resourcePath, PATH_MAX))
			{
				sstrcat(resourcePath, "/Fonts");
				glcAppendCatalog(resourcePath);
			}
			else
			{
				debug(LOG_ERROR, "Could not change to resources directory.");
			}

			if (resourceURL != NULL)
			{
				CFRelease(resourceURL);
			}
		}
#endif

		st._glcFont_Regular = glcGenFontID();
		st._glcFont_Bold = glcGenFontID();

		if (!glcNewFontFromFamily(st._glcFont_Regular, st.font_family.data()))
		{
			debug(LOG_ERROR, "Failed to select font family %s as regular font", st.font_family.data());
		}
		else
		{
			debug(LOG_NEVER, "Successfully selected font family %s as regular font", st.font_family.data());
		}

		if (!glcFontFace(st._glcFont_Regular, st.font_face_regular.data()))
		{
			debug(LOG_WARNING, "Failed to select the \"%s\" font face of font family %s", st.font_face_regular.data(), st.font_family.data());
		}
		else
		{
			debug(LOG_NEVER, "Successfully selected the \"%s\" font face of font family %s", st.font_face_regular.data(), st.font_family.data());
		}

		if (!glcNewFontFromFamily(st._glcFont_Bold, st.font_family.data()))
		{
			debug(LOG_ERROR, "iV_initializeGLC: Failed to select font family %s for the bold font", st.font_family.data());
		}
		else
		{
			debug(LOG_NEVER, "Successfully selected font family %s for the bold font", st.font_family.data());
		}

		if (!glcFontFace(st._glcFont_Bold, st.font_face_bold.data()))
		{
			debug(LOG_WARNING, "Failed to select the \"%s\" font face of font family %s", st.font_face_bold.data(), st.font_family);
		}
		else
		{
			debug(LOG_NEVER, "Successfully selected the \"%s\" font face of font family %s", st.font_face_bold.data(), st.font_family);
		}

		debug(LOG_NEVER, "Finished initializing GLC");
	}

	float getGLCResolution()
	{
		float resolution = glcGetf(GLC_RESOLUTION);

		// The default resolution as used by OpenGLC is 72 dpi
		if (resolution == 0.f)
		{
			return 72.f;
		}

		return resolution;
	}

	float getGLCPixelSize(float font_size)
	{
		float pixel_size = font_size * getGLCResolution() / 72.f;
		return pixel_size;
	}

	float getGLCPointWidth(const float *boundingbox)
	{
		// boundingbox contains: [ xlb ylb xrb yrb xrt yrt xlt ylt ]
		// l = left; r = right; b = bottom; t = top;
		float rightTopX = boundingbox[4];
		float leftTopX = boundingbox[6];

		float point_width = rightTopX - leftTopX;

		return point_width;
	}

	float getGLCPointHeight(const float *boundingbox)
	{
		// boundingbox contains: [ xlb ylb xrb yrb xrt yrt xlt ylt ]
		// l = left; r = right; b = bottom; t = top;
		float leftBottomY = boundingbox[1];
		float leftTopY = boundingbox[7];

		float point_height = fabsf(leftTopY - leftBottomY);

		return point_height;
	}

	float getGLCPointToPixel(float point_width, float font_size)
	{
		float pixel_width = point_width * getGLCPixelSize(font_size);

		return pixel_width;
	}

	float GetMaxCharBaseY()
	{
		float base_line[4]; // [ xl yl xr yr ]

		if (!glcGetMaxCharMetric(GLC_BASELINE, base_line))
		{
			debug(LOG_ERROR, "Couldn't retrieve the baseline for the character");
			return 0;
		}

		return base_line[1];
	}

	void DrawTextRotatedFv(float x, float y, float rotation, const char *format, va_list ap, iV::font_state &st)
	{
		va_list aq;
		size_t size;
		char *str;

		/* Required because we're using the va_list ap twice otherwise, which
		* results in undefined behaviour. See stdarg(3) for details.
		*/
		va_copy(aq, ap);

		// Allocate a buffer large enough to hold our string on the stack
		size = vsnprintf(NULL, 0, format, ap);
		str = (char *)alloca(size + 1);

		// Print into our newly created string buffer
		vsprintf(str, format, aq);

		va_end(aq);

		// Draw the produced string to the screen at the given position and rotation
		iV::DrawTextRotated(str, x, y, rotation, st);
	}

	enum class wordSplitReason
	{
		space_found,
		newline_found,
		not_enough_space,
		end_of_string,
	};

	std::tuple<std::string, wordSplitReason> accumulateCharsUntilBlankOrTooBig(const char * &curChar, UDWORD &WWidth, unsigned int FStringWidth, UDWORD Width, iV::font_state & st)
	{
		std::string FWord;
		for (; *curChar != 0; ++curChar)
		{
			if (*curChar == ASCII_SPACE)
				return std::make_tuple(FWord, wordSplitReason::space_found);
			if (*curChar == ASCII_NEWLINE || *curChar == '\n')
				return std::make_tuple(FWord, wordSplitReason::newline_found);
			if (*curChar != ASCII_COLOURMODE) // this character won't be drawn so don't deal with its width
			{
				// Update this line's pixel width.
				//WWidth = FStringWidth + iV_GetCountedTextWidth(FWord.c_str(), i + 1);  // This triggers tonnes of valgrind warnings, if the string contains unicode. Adding lots of trailing garbage didn't help... Using iV_GetTextWidth with a null-terminated string, instead.
				WWidth = FStringWidth + GetTextWidth(FWord, st);
				// If this word doesn't fit on the current line then break out
				if (WWidth > Width)
					return std::make_tuple(FWord, wordSplitReason::not_enough_space);
			}
			// If width ok then add this character to the current word.
			FWord.push_back(*curChar);
		}
		return std::make_tuple(FWord, wordSplitReason::end_of_string);
	}

	std::tuple<std::string, wordSplitReason, int> getNextWord(const char * &curChar, UDWORD &WWidth, const unsigned int &FStringWidth, iV::font_state & st, const UDWORD &Width, bool& GotSpace, bool& NewLine)
	{
		const char * start = curChar;
		std::string FWord;
		wordSplitReason reason;
		std::tie(FWord, reason) = accumulateCharsUntilBlankOrTooBig(curChar, WWidth, FStringWidth, Width, st);

		// Check for new line character.
		if (reason == wordSplitReason::newline_found)
		{
			if (!bMultiPlayer)
			{
				NewLine = true;
			}
			++curChar;
			return std::make_tuple(FWord, wordSplitReason::newline_found, curChar - start - 1);
		}

		// Don't forget the space.
		if (reason == wordSplitReason::space_found)
		{
			// Should be a space below, not '-', but need to work around bug in QuesoGLC
			// which was fixed in CVS snapshot as of 2007/10/26, same day as I reported it :) - Per
			WWidth += GetCharWidth('-', st);
			if (WWidth > Width)
				std::make_tuple(FWord, wordSplitReason::not_enough_space, curChar - start);
			FWord.push_back(' ');
			++curChar;
			GotSpace = true;
		}
		return std::make_tuple(FWord, reason, curChar - start);
	}

	std::string extractLine(const char * &curChar, UDWORD &Width, iV::font_state & st)
	{
		bool GotSpace = false;
		bool NewLine = false;
		std::string FString;
		uint32_t WWidth = 0;

		// Parse through the string, adding words until width is achieved.
		while (*curChar != 0 && WWidth < Width && !NewLine)
		{
			const char *startOfWord = curChar;
			const unsigned int FStringWidth = GetTextWidth(FString, st);

			// Get the next word.
			std::string FWord;
			int last_non_newline_character_index;
			wordSplitReason reason;
			std::tie(FWord, reason, last_non_newline_character_index) = getNextWord(curChar, WWidth, FStringWidth, st, Width, GotSpace, NewLine);

			// If we've passed a space on this line and the word goes past the
			// maximum width and this isn't caused by the appended space then
			// rewind to the start of this word and finish this line.
			if (reason == wordSplitReason::not_enough_space
				&& GotSpace
				&& curChar != startOfWord
				&& *curChar != ' ')
			{
				// Skip back to the beginning of this
				// word and draw it on the next line
				curChar = startOfWord;
				break;
			}

			// And add it to the output string.
			FString.append(FWord);
		}

		// Remove trailing spaces, useful when doing center alignment.
		while (!FString.empty() && FString[FString.size() - 1] == ' ')
		{
			FString.erase(FString.size() - 1);  // std::string has no pop_back().
		}

		return FString;
	}

	int getHorizontalPosition(iV::justification Justify, uint32_t x, uint32_t Width, int TWidth)
	{
		switch (Justify)
		{
		case iV::justification::FTEXT_CENTRE:
			return x + (Width - TWidth) / 2;

		case iV::justification::FTEXT_RIGHTJUSTIFY:
			return x + Width - TWidth;

		case iV::justification::FTEXT_LEFTJUSTIFY:
			return x;
		}
	}
} // end namespace anonymous

/***************************************************************************/
/*
 *	Source
 */
/***************************************************************************/

namespace iV
{

void font(const std::string& fontName, const std::string& fontFace, const std::string& fontFaceBold, font_state &st)
{
	if (st._glcContext)
	{
		debug(LOG_ERROR, "Cannot change font in running game, yet.");
		return;
	}
	if (!fontName.empty())
	{
		st.font_family = fontName;
	}
	if (!fontFace.empty())
	{
		st.font_face_regular = fontFace;
	}
	if (!fontFaceBold.empty())
	{
		st.font_face_bold = fontFaceBold;
	}
}

void TextInit(font_state &st)
{
	initializeGLC(st);
	SetFont(fonts::font_regular, st);
}

void TextShutdown(font_state &st)
{
	if (st._glcFont_Regular)
	{
		glcDeleteFont(st._glcFont_Regular);
	}

	if (st._glcFont_Bold)
	{
		glcDeleteFont(st._glcFont_Bold);
	}

	glcContext(0);

	if (st._glcContext)
	{
		glcDeleteContext(st._glcContext);
	}
}

void SetFont(fonts FontID, font_state &st)
{
	switch (FontID)
	{
	case fonts::font_scaled:
		SetTextSize(12.f * pie_GetVideoBufferHeight() / 480, st);
		glcFont(st._glcFont_Regular);
		break;

	default:
	case fonts::font_regular:
		SetTextSize(12.f, st);
		glcFont(st._glcFont_Regular);
		break;

	case fonts::font_large:
		SetTextSize(21.f, st);
		glcFont(st._glcFont_Bold);
		break;

	case fonts::font_medium:
		SetTextSize(16.f, st);
		glcFont(st._glcFont_Regular);
		break;

	case fonts::font_small:
		SetTextSize(9.f, st);
		glcFont(st._glcFont_Regular);
		break;
	}
}

unsigned int GetTextWidth(const std::string &string, font_state &st)
{
	float boundingbox[8]{};
	glcMeasureString(GL_FALSE, string.data());
	if (!glcGetStringMetric(GLC_BOUNDS, boundingbox))
	{
		debug(LOG_ERROR, "Couldn't retrieve a bounding box for the string \"%s\"", string);
		return 0;
	}

	float point_width = getGLCPointWidth(boundingbox);
	float pixel_width = getGLCPointToPixel(point_width, st.font_size);
	return static_cast<unsigned int>(pixel_width);
}

unsigned int GetCountedTextWidth(const std::string &string, size_t string_length, font_state &st)
{
	float boundingbox[8]{};
	glcMeasureCountedString(GL_FALSE, string_length, string.data());
	if (!glcGetStringMetric(GLC_BOUNDS, boundingbox))
	{
		debug(LOG_ERROR, "Couldn't retrieve a bounding box for the string \"%s\" of length %u", string, (unsigned int)string_length);
		return 0;
	}

	float point_width = getGLCPointWidth(boundingbox);
	float pixel_width = getGLCPointToPixel(point_width, st.font_size);
	return static_cast<unsigned int>(pixel_width);
}

unsigned int GetTextHeight(const std::string &string, font_state &st)
{
	float boundingbox[8]{};
	glcMeasureString(GL_FALSE, string.data());
	if (!glcGetStringMetric(GLC_BOUNDS, boundingbox))
	{
		debug(LOG_ERROR, "Couldn't retrieve a bounding box for the string \"%s\"", string);
		return 0;
	}

	float point_height = getGLCPointHeight(boundingbox);
	float pixel_height = getGLCPointToPixel(point_height, st.font_size);
	return static_cast<unsigned int>(pixel_height);
}

unsigned int GetCharWidth(uint32_t charCode, font_state &st)
{
	float boundingbox[8]{};
	if (!glcGetCharMetric(charCode, GLC_BOUNDS, boundingbox))
	{
		debug(LOG_ERROR, "Couldn't retrieve a bounding box for the character code %u", charCode);
		return 0;
	}

	float point_width = getGLCPointWidth(boundingbox);
	float pixel_width = getGLCPointToPixel(point_width, st.font_size);
	return static_cast<unsigned int>(pixel_width);
}

int GetTextLineSize(font_state &st)
{
	float boundingbox[8]{};
	if (!glcGetMaxCharMetric(GLC_BOUNDS, boundingbox))
	{
		debug(LOG_ERROR, "Couldn't retrieve a bounding box for the character");
		return 0;
	}

	float point_height = getGLCPointHeight(boundingbox);
	float pixel_height = getGLCPointToPixel(point_height, st.font_size);
	return static_cast<unsigned int>(pixel_height);
}

int GetTextAboveBase(font_state &st)
{
	float boundingbox[8]{};
	if (!glcGetMaxCharMetric(GLC_BOUNDS, boundingbox))
	{
		debug(LOG_ERROR, "Couldn't retrieve a bounding box for the character");
		return 0;
	}

	float point_base_y = GetMaxCharBaseY();
	float point_top_y = boundingbox[7];
	float point_height = point_base_y - point_top_y;
	float pixel_height = getGLCPointToPixel(point_height, st.font_size);
	return static_cast<int>(pixel_height);
}

int GetTextBelowBase(font_state &st)
{
	float boundingbox[8]{};
	if (!glcGetMaxCharMetric(GLC_BOUNDS, boundingbox))
	{
		debug(LOG_ERROR, "Couldn't retrieve a bounding box for the character");
		return 0;
	}

	float point_base_y = GetMaxCharBaseY();
	float point_bottom_y = boundingbox[1];
	float point_height = point_bottom_y - point_base_y;
	float pixel_height = getGLCPointToPixel(point_height, st.font_size);
	return static_cast<int>(pixel_height);
}

void SetTextColour(PIELIGHT colour, font_state &st)
{
	st.font_colour[0] = colour.byte.r / 255.0f;
	st.font_colour[1] = colour.byte.g / 255.0f;
	st.font_colour[2] = colour.byte.b / 255.0f;
	st.font_colour[3] = colour.byte.a / 255.0f;
}

/** Draws formatted text with word wrap, long word splitting, embedded newlines
 *  (uses '@' rather than '\n') and colour toggle mode ('#') which enables or
 *  disables font colouring.
 *
 *  @param String   the string to display.
 *  @param x,y      X and Y coordinates of top left of formatted text.
 *  @param width    the maximum width of the formatted text (beyond which line
 *                  wrapping is used).
 *  @param justify  The alignment style to use, which is one of the following:
 *                  FTEXT_LEFTJUSTIFY, FTEXT_CENTRE or FTEXT_RIGHTJUSTIFY.
 *  @return the Y coordinate for the next text line.
 */
int DrawFormattedText(const std::string &String, UDWORD x, UDWORD y, UDWORD Width, justification Justify, font_state &st)
{
	int verticalPosition = y;
	const char *curChar = String.data();

	while (*curChar != 0)
	{
		std::string FString = extractLine(curChar, Width, st);
		int TWidth = GetTextWidth(FString, st);
		// Do justify.
		int horizontalPosition = getHorizontalPosition(Justify, x, Width, TWidth);

		// draw the text.
		DrawText(FString.c_str(), horizontalPosition, verticalPosition);

		// and move down a line.
		verticalPosition += GetTextLineSize(st);
	}

	return verticalPosition;
}

void DrawTextRotated(const std::string &string, float XPos, float YPos, float rotation, font_state &st)
{
	GLint matrix_mode = 0;
	ASSERT_OR_RETURN(, string.data(), "Couldn't render string!");
	pie_SetTexturePage(TEXPAGE_EXTERN);

	glGetIntegerv(GL_MATRIX_MODE, &matrix_mode);
	glMatrixMode(GL_TEXTURE);
	glPushMatrix();
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();

	if (rotation != 0.f)
	{
		rotation = 360.f - rotation;
	}

	glTranslatef(XPos, YPos, 0.f);
	glRotatef(180.f, 1.f, 0.f, 0.f);
	glRotatef(rotation, 0.f, 0.f, 1.f);
	glScalef(st.font_size, st.font_size, 0.f);

	glColor4fv(st.font_colour.data());

	glFrontFace(GL_CW);
	glcRenderString(string.data());
	glFrontFace(GL_CCW);

	glPopMatrix();
	glMatrixMode(GL_TEXTURE);
	glPopMatrix();
	glMatrixMode(matrix_mode);

	// Reset the current model view matrix
	glLoadIdentity();
}

void DrawTextF(float x, float y, const char *format, ...)
{
	va_list ap;

	va_start(ap, format);
	DrawTextRotatedFv(x, y, 0.f, format, ap, default_font_state);
	va_end(ap);
}

void SetTextSize(float size, font_state &st)
{
	st.font_size = size;
}

} // end namespace iV
