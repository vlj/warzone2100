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

#include <QString>
#include "lib/gamelib/frame.h"
#include "api_object.h"
#include "gl/gl_impl.h"

#include "lib/ivis_opengl/ivisdef.h"
#include "lib/ivis_opengl/piestate.h"
#include "lib/ivis_opengl/tex.h"
#include "lib/ivis_opengl/piepalette.h"
#include "lib/ivis_opengl/png_util.h"

#include <QtCore/QList>
#include "screen.h"

/* global used to indicate preferred internal OpenGL format */
bool wz_texture_compression = false;

//*************************************************************************

struct iTexPage
{
	char name[iV_TEXNAME_MAX];
	std::unique_ptr<gfx_api::texture> id;
};

std::vector<iTexPage> _TEX_PAGE;

//*************************************************************************

gfx_api::texture& pie_Texture(int page)
{
	return *_TEX_PAGE[page].id;
}

int pie_NumberOfPages()
{
	return _TEX_PAGE.size();
}

// Add a new texture page to the list
int pie_ReserveTexture(const char *name, const gfx_api::format& f, size_t width, size_t height, size_t levels)
{
	iTexPage tex;
	sstrcpy(tex.name, name);
	tex.id = gfx_api::context::get_context().create_texture(f, width, height, levels);
	_TEX_PAGE.emplace_back(std::move(tex));
	return _TEX_PAGE.size() - 1;
}

auto get_internal_format_type(gfx_api::format f)
{
	switch (f)
	{
	case gfx_api::format::rgba: return GL_RGBA8;
	case gfx_api::format::rgb: return GL_RGB8;
	}
	throw;
}


int pie_AddTexPage(iV_Image *s, const char *filename, bool gameTexture, int page)
{
	ASSERT(s && filename, "Bad input parameter");

	if (page < 0)
	{
		iTexPage tex;
		page = _TEX_PAGE.size();
		sstrcpy(tex.name, filename);
		_TEX_PAGE.emplace_back(std::move(tex));
	}
	else // replace
	{
		sstrcpy(_TEX_PAGE[page].name, filename);

	}
	debug(LOG_TEXTURE, "%s page=%d", filename, page);

/*	if (GLEW_VERSION_4_3 || GLEW_KHR_debug)
	{
		glObjectLabel(GL_TEXTURE, pie_Texture(page), -1, filename);
	}*/

	if (gameTexture) // this is a game texture, use texture compression
	{
		_TEX_PAGE[page].id = gfx_api::context::get_context().create_texture(gfx_api::format::rgba, s->width, s->height, 1);
		const auto& as_gl_texture = dynamic_cast<gfx_api::gl_api::gl_texture&>(*_TEX_PAGE[page].id);
		glBindTexture(GL_TEXTURE_2D, as_gl_texture.object);
		gluBuild2DMipmaps(GL_TEXTURE_2D, wz_texture_compression ? GL_RGBA : GL_RGBA, s->width, s->height, get_internal_format_type(iV_getPixelFormat(s)), GL_UNSIGNED_BYTE, s->bmp);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	}
	else	// this is an interface texture, do not use compression
	{
		_TEX_PAGE[page].id = gfx_api::context::get_context().create_texture(gfx_api::format::rgb, s->width, s->height, 1);
		_TEX_PAGE[page].id->upload_texture(gfx_api::format::rgba, s->width, s->height, s->bmp);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	}
	// it is uploaded, we do not need it anymore
	free(s->bmp);
	s->bmp = NULL;

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// Use anisotropic filtering, if available, but only max 4.0 to reduce processor burden
	if (GLEW_EXT_texture_filter_anisotropic)
	{
		GLfloat max;
		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &max);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, MIN(4.0f, max));
	}
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	/* Send back the texpage number so we can store it in the IMD */
	return page;
}

/*!
 * Turns filename into a pagename if possible
 * \param[in,out] filename Filename to pagify
 */
void pie_MakeTexPageName(char *filename)
{
	char *c = strstr(filename, iV_TEXNAME_TCSUFFIX);
	if (c)
	{
		*(c + 7) = '\0';
		return;
	}
	c = strchr(filename + 5, '-');
	if (c)
	{
		*c = '\0';
	}
}

/*!
 * Turns page filename into a pagename + tc mask if possible
 * \param[in,out] filename Filename to pagify
 */
void pie_MakeTexPageTCMaskName(char *filename)
{
	if (strncmp(filename, "page-", 5) == 0)
	{
		int i;
		for (i = 5; i < iV_TEXNAME_MAX - 1 && isdigit(filename[i]); i++) {}
		filename[i] = '\0';
		strcat(filename, iV_TEXNAME_TCSUFFIX);
	}
}

/** Retrieve the texture number for a given texture resource.
 *
 *  @note We keep textures in a separate data structure _TEX_PAGE apart from the
 *        normal resource system.
 *
 *  @param filename The filename of the texture page to search for.
 *  @param compression If we need to load it, should we use texture compression?
 *
 *  @return a non-negative index number for the texture, negative if no texture
 *          with the given filename could be found
 */
int iV_GetTexture(const char *filename, bool compression)
{
	iV_Image sSprite;
	char path[PATH_MAX];

	/* Have we already loaded this one then? */
	sstrcpy(path, filename);
	pie_MakeTexPageName(path);
	for (int i = 0; i < _TEX_PAGE.size(); i++)
	{
		if (strncmp(path, _TEX_PAGE[i].name, iV_TEXNAME_MAX) == 0)
		{
			return i;
		}
	}

	// Try to load it
	sstrcpy(path, "texpages/");
	sstrcat(path, filename);
	if (!iV_loadImage_PNG(path, &sSprite))
	{
		debug(LOG_ERROR, "Failed to load %s", path);
		return -1;
	}
	sstrcpy(path, filename);
	pie_MakeTexPageName(path);
	return pie_AddTexPage(&sSprite, path, compression);
}

bool replaceTexture(const QString &oldfile, const QString &newfile)
{
	char tmpname[iV_TEXNAME_MAX];

	// Load new one to replace it
	iV_Image image;
	if (!iV_loadImage_PNG(QString("texpages/" + newfile).toUtf8().constData(), &image))
	{
		debug(LOG_ERROR, "Failed to load image: %s", newfile.toUtf8().constData());
		return false;
	}
	sstrcpy(tmpname, oldfile.toUtf8().constData());
	pie_MakeTexPageName(tmpname);
	// Have we already loaded this one?
	for (int i = 0; i < _TEX_PAGE.size(); i++)
	{
		if (strcmp(tmpname, _TEX_PAGE[i].name) == 0)
		{
			//GL_DEBUG("Replacing texture");
			//debug(LOG_TEXTURE, "Replacing texture %s with %s from index %d (tex id %u)", _TEX_PAGE[i].name, newfile.toUtf8().constData(), i, _TEX_PAGE[i].id);
			sstrcpy(tmpname, newfile.toUtf8().constData());
			pie_MakeTexPageName(tmpname);
			pie_AddTexPage(&image, tmpname, true, i);
			iV_unloadImage(&image);
			glErrors();
			return true;
		}
	}
	iV_unloadImage(&image);
	debug(LOG_ERROR, "Nothing to replace!");
	return false;
}

void pie_TexShutDown(void)
{
	// TODO, lazy deletions for faster loading of next level
	debug(LOG_TEXTURE, "Cleaning out %u textures", _TEX_PAGE.size());
	int _TEX_INDEX = _TEX_PAGE.size() - 1;
	_TEX_PAGE.clear();
}

void pie_TexInit(void)
{
	debug(LOG_TEXTURE, "pie_TexInit successful");
}

void iV_unloadImage(iV_Image *image)
{
	if (image)
	{
		if (image->bmp)
		{
			free(image->bmp);
			image->bmp = NULL;
		}
	}
	else
	{
		debug(LOG_ERROR, "Tried to free invalid image!");
	}
}

gfx_api::format iV_getPixelFormat(const iV_Image *image)
{
	switch (image->depth)
	{
	case 3:
		return gfx_api::format::rgb;
	case 4:
		return gfx_api::format::rgba;
	default:
		debug(LOG_ERROR, "iV_getPixelFormat: Unsupported image depth: %u", image->depth);
		//return GL_INVALID_ENUM;
	}
	throw;
}
