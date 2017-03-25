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
/***************************************************************************/
/*
 * pieBlitFunc.c
 *
 */
/***************************************************************************/
#include "lib/framework/opengl.h"

#include "pieblitfunc.h"
#include "pietypes.h"
#include "piestate.h"
#include "bitimage.h"
#include "GFX.h"
#include "screen.h"


/***************************************************************************/
/*
 *	Local Variables
 */
/***************************************************************************/

static GFX *radarGfx = NULL;

struct PIERECT  ///< Screen rectangle.
{
	float x, y, w, h;
};

/***************************************************************************/
/*
 *	Static function forward declarations
 */
/***************************************************************************/

static bool assertValidImage(IMAGEFILE *imageFile, unsigned id);
static Vector2i makePieImage(IMAGEFILE *imageFile, unsigned id, PIERECT *dest = NULL, int x = 0, int y = 0);

/***************************************************************************/
/*
 *	Source
 */
/***************************************************************************/

void iV_Line(int x0, int y0, int x1, int y1, PIELIGHT colour)
{
	pie_SetTexturePage(TEXPAGE_NONE);

	glColor4ubv(colour.vector);
	glBegin(GL_LINES);
	glVertex2i(x0, y0);
	glVertex2i(x1, y1);
	glEnd();
}

/**
 *	Assumes render mode set up externally, draws filled rectangle.
 */
static void pie_DrawRect(float x0, float y0, float x1, float y1, PIELIGHT colour)
{
	glColor4ubv(colour.vector);
	glBegin(GL_TRIANGLE_STRIP);
	glVertex2f(x0, y0);
	glVertex2f(x1, y0);
	glVertex2f(x0, y1);
	glVertex2f(x1, y1);
	glEnd();
}

void iV_ShadowBox(int x0, int y0, int x1, int y1, int pad, PIELIGHT first, PIELIGHT second, PIELIGHT fill)
{
	pie_SetRendMode(REND_OPAQUE);
	pie_SetTexturePage(TEXPAGE_NONE);
	pie_DrawRect(x0 + pad, y0 + pad, x1 - pad, y1 - pad, fill);
	iV_Box2(x0, y0, x1, y1, first, second);
}

/***************************************************************************/

void iV_Box2(int x0, int y0, int x1, int y1, PIELIGHT first, PIELIGHT second)
{
	pie_SetTexturePage(TEXPAGE_NONE);

	glColor4ubv(first.vector);
	glBegin(GL_LINES);
	glVertex2i(x0, y1);
	glVertex2i(x0, y0);
	glVertex2i(x0, y0);
	glVertex2i(x1, y0);
	glEnd();
	glColor4ubv(second.vector);
	glBegin(GL_LINES);
	glVertex2i(x1, y0);
	glVertex2i(x1, y1);
	glVertex2i(x0, y1);
	glVertex2i(x1, y1);
	glEnd();
}

/***************************************************************************/

void pie_BoxFill(int x0, int y0, int x1, int y1, PIELIGHT colour, REND_MODE rendermode)
{
	pie_SetRendMode(rendermode);
	pie_SetTexturePage(TEXPAGE_NONE);
	pie_DrawRect(x0, y0, x1, y1, colour);
}

/***************************************************************************/

void iV_TransBoxFill(float x0, float y0, float x1, float y1)
{
	pie_UniTransBoxFill(x0, y0, x1, y1, WZCOL_TRANSPARENT_BOX);
}

/***************************************************************************/

void pie_UniTransBoxFill(float x0, float y0, float x1, float y1, PIELIGHT light)
{
	pie_SetTexturePage(TEXPAGE_NONE);
	pie_SetRendMode(REND_ALPHA);
	pie_DrawRect(x0, y0, x1, y1, light);
}

/***************************************************************************/

static bool assertValidImage(IMAGEFILE *imageFile, unsigned id)
{
	ASSERT_OR_RETURN(false, id < imageFile->imageDefs.size(), "Out of range 1: %u/%d", id, (int)imageFile->imageDefs.size());
	ASSERT_OR_RETURN(false, imageFile->imageDefs[id].TPageID < imageFile->pages.size(), "Out of range 2: %u", imageFile->imageDefs[id].TPageID);
	return true;
}

static void iv_DrawImageImpl(Vector2i Position, Vector2i size, Vector2f TextureUV, Vector2f TextureSize, PIELIGHT colour)
{
	glColor4ubv(colour.vector);
	glBegin(GL_TRIANGLE_STRIP);
	glTexCoord2f(TextureUV.x, TextureUV.y);
	glVertex2f(Position.x, Position.y);

	glTexCoord2f(TextureUV.x + TextureSize.x, TextureUV.y);
	glVertex2f(Position.x + size.x, Position.y);

	glTexCoord2f(TextureUV.x, TextureUV.y + TextureSize.y);
	glVertex2f(Position.x, Position.y + size.y);

	glTexCoord2f(TextureUV.x + TextureSize.x, TextureUV.y + TextureSize.y);
	glVertex2f(Position.x + size.x, Position.y + size.y);
	glEnd();

}

void iV_DrawImage(GLuint TextureID, Vector2i Position, Vector2i size, REND_MODE mode, PIELIGHT colour)
{
	pie_SetRendMode(mode);
	pie_SetTexturePage(TEXPAGE_EXTERN);
	glBindTexture(GL_TEXTURE_2D, TextureID);
	iv_DrawImageImpl(Position, size, Vector2f(0.f, 0.f), Vector2f(1.f, 1.f), colour);
}

static void pie_DrawImage(IMAGEFILE *imageFile, int id, Vector2i size, const PIERECT *dest, PIELIGHT colour = WZCOL_WHITE)
{
	ImageDef const &image2 = imageFile->imageDefs[id];
	GLuint texPage = imageFile->pages[image2.TPageID].id;
	GLfloat invTextureSize = 1.f / imageFile->pages[image2.TPageID].size;
	int tu = image2.Tu;
	int tv = image2.Tv;

	pie_SetTexturePage(texPage);
	iv_DrawImageImpl(Vector2i(dest->x, dest->y), Vector2i(dest->w, dest->h),
		Vector2f(tu * invTextureSize, tv * invTextureSize),
		Vector2f(size.x * invTextureSize, size.y * invTextureSize),
		colour);
}

static Vector2i makePieImage(IMAGEFILE *imageFile, unsigned id, PIERECT *dest, int x, int y)
{
	ImageDef const &image = imageFile->imageDefs[id];
	Vector2i pieImage;
	pieImage.x = image.Width;
	pieImage.y = image.Height;
	if (dest != NULL)
	{
		dest->x = x + image.XOffset;
		dest->y = y + image.YOffset;
		dest->w = image.Width;
		dest->h = image.Height;
	}
	return pieImage;
}

void iV_DrawImage2(const ImageDef &image, float x, float y, float width, float height)
{
	const GLfloat invTextureSize = image.invTextureSize;
	const int tu = image.Tu;
	const int tv = image.Tv;
	const int w = width > 0 ? width : image.Width;
	const int h = height > 0 ? height : image.Height;
	x += image.XOffset;
	y += image.YOffset;
	pie_SetTexturePage(image.textureId);
	pie_SetRendMode(REND_ALPHA);
	iv_DrawImageImpl(Vector2i(x, y), Vector2i(w, h),
		Vector2f(tu * invTextureSize, tv * invTextureSize),
		Vector2f(image.Width * invTextureSize, image.Height * invTextureSize),
		WZCOL_WHITE);
}

void iV_DrawImage(IMAGEFILE *ImageFile, uint16_t ID, int x, int y)
{
	if (!assertValidImage(ImageFile, ID))
	{
		return;
	}

	PIERECT dest;
	Vector2i pieImage = makePieImage(ImageFile, ID, &dest, x, y);

	pie_SetRendMode(REND_ALPHA);

	pie_DrawImage(ImageFile, ID, pieImage, &dest);
}

void iV_DrawImageTc(Image image, Image imageTc, int x, int y, PIELIGHT colour)
{
	if (!assertValidImage(image.images, image.id) || !assertValidImage(imageTc.images, imageTc.id))
	{
		return;
	}

	PIERECT dest;
	Vector2i pieImage   = makePieImage(image.images, image.id, &dest, x, y);
	Vector2i pieImageTc = makePieImage(imageTc.images, imageTc.id);

	pie_SetRendMode(REND_ALPHA);

	pie_DrawImage(image.images, image.id, pieImage, &dest);
	pie_DrawImage(imageTc.images, imageTc.id, pieImageTc, &dest, colour);
}

// Repeat a texture
void iV_DrawImageRepeatX(IMAGEFILE *ImageFile, uint16_t ID, int x, int y, int Width)
{
	assertValidImage(ImageFile, ID);
	const ImageDef *Image = &ImageFile->imageDefs[ID];

	pie_SetRendMode(REND_OPAQUE);

	PIERECT dest;
	Vector2i pieImage = makePieImage(ImageFile, ID, &dest, x, y);

	unsigned hRemainder = Width % Image->Width;

	for (unsigned hRep = 0; hRep < Width / Image->Width; hRep++)
	{
		pie_DrawImage(ImageFile, ID, pieImage, &dest);
		dest.x += Image->Width;
	}

	// draw remainder
	if (hRemainder > 0)
	{
		pieImage.x = hRemainder;
		dest.w = hRemainder;
		pie_DrawImage(ImageFile, ID, pieImage, &dest);
	}
}

void iV_DrawImageRepeatY(IMAGEFILE *ImageFile, uint16_t ID, int x, int y, int Height)
{
	assertValidImage(ImageFile, ID);
	const ImageDef *Image = &ImageFile->imageDefs[ID];

	pie_SetRendMode(REND_OPAQUE);

	PIERECT dest;
	Vector2i pieImage = makePieImage(ImageFile, ID, &dest, x, y);

	unsigned vRemainder = Height % Image->Height;

	for (unsigned vRep = 0; vRep < Height / Image->Height; vRep++)
	{
		pie_DrawImage(ImageFile, ID, pieImage, &dest);
		dest.y += Image->Height;
	}

	// draw remainder
	if (vRemainder > 0)
	{
		pieImage.y = vRemainder;
		dest.h = vRemainder;
		pie_DrawImage(ImageFile, ID, pieImage, &dest);
	}
}

bool pie_InitRadar(void)
{
	radarGfx = new GFX(GFX_TEXTURE, GL_TRIANGLE_STRIP, 2);
	return true;
}

bool pie_ShutdownRadar(void)
{
	delete radarGfx;
	radarGfx = NULL;
	return true;
}

void pie_SetRadar(GLfloat x, GLfloat y, GLfloat width, GLfloat height, int twidth, int theight, bool filter)
{
	radarGfx->makeTexture(twidth, theight, filter ? GL_LINEAR : GL_NEAREST);
	GLfloat texcoords[] = { 0.0f, 0.0f,  1.0f, 0.0f,  0.0f, 1.0f,  1.0f, 1.0f };
	GLfloat vertices[] = { x, y,  x + width, y,  x, y + height,  x + width, y + height };
	radarGfx->buffers(4, vertices, texcoords);
}

/** Store radar texture with given width and height. */
void pie_DownLoadRadar(uint32_t *buffer)
{
	radarGfx->updateTexture(buffer);
}

/** Display radar texture using the given height and width, depending on zoom level. */
void pie_RenderRadar()
{
	pie_SetRendMode(REND_ALPHA);
	glColor4ubv(WZCOL_WHITE.vector); // hack
	radarGfx->draw();
}

/// Load and display a random backdrop picture.
void pie_LoadBackDrop(SCREENTYPE screenType)
{
	switch (screenType)
	{
	case SCREEN_RANDOMBDROP:
		screen_SetRandomBackdrop("texpages/bdrops/", "backdrop");
		break;
	case SCREEN_MISSIONEND:
		screen_SetRandomBackdrop("texpages/bdrops/", "missionend");
		break;
	case SCREEN_CREDITS:
		screen_SetRandomBackdrop("texpages/bdrops/", "credits");
		break;
	}
}
