#include "GFX.h"
#include "png_util.h"
#include "tex.h"
#include "piestate.h"
#include "gl/gl_impl.h"

GFX::GFX(GFXTYPE type, gfx_api::drawtype drawType, int coordsPerVertex) : mType(type), mdrawType(drawType), mCoordsPerVertex(coordsPerVertex), mSize(0)
{
}

void GFX::loadTexture(const char *filename, gfx_api::filter filter)
{
	ASSERT(mType == GFX_TEXTURE, "Wrong GFX type");
	const char *extension = strrchr(filename, '.'); // determine the filetype
	iV_Image image;
	if (!extension || strcmp(extension, ".png") != 0)
	{
		debug(LOG_ERROR, "Bad image filename: %s", filename);
		return;
	}
	if (iV_loadImage_PNG(filename, &image))
	{
		makeTexture(image.width, image.height, filter, iV_getPixelFormat(&image), image.bmp);
		iV_unloadImage(&image);
	}
}

static auto
get_filter(const gfx_api::filter& filter)
{
	switch (filter)
	{
	case gfx_api::filter::linear: return GL_LINEAR;
	case gfx_api::filter::nearest: return GL_NEAREST;
	}
	throw;
}

void GFX::makeTexture(int width, int height, gfx_api::filter filter, gfx_api::format format, const void *image)
{
	ASSERT(mType == GFX_TEXTURE, "Wrong GFX type");
	pie_SetTexturePage(TEXPAGE_EXTERN);
	mTexture = gfx_api::context::get_context().create_texture(format, width, height, 1);
	if (image != nullptr)
		mTexture->upload_texture(format, width, height, image);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, get_filter(filter));
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, get_filter(filter));
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	mWidth = width;
	mHeight = height;
	mFormat = format;
}

void GFX::updateTexture(const void *image, int width, int height)
{
	ASSERT(mType == GFX_TEXTURE, "Wrong GFX type");
	if (width == -1)
	{
		width = mWidth;
	}
	if (height == -1)
	{
		height = mHeight;
	}
	pie_SetTexturePage(TEXPAGE_EXTERN);
	mTexture->upload_texture(mFormat, width, height, image);
}

void GFX::buffers(int vertices, const void *vertBuf, const void *auxBuf)
{
	mBuffers[VBO_VERTEX] = gfx_api::context::get_context().create_buffer(vertices * mCoordsPerVertex * sizeof(float));
	mBuffers[VBO_VERTEX]->upload(0, vertices * mCoordsPerVertex * sizeof(GLfloat), vertBuf);
	if (mType == GFX_TEXTURE)
	{
		mBuffers[VBO_TEXCOORD] = gfx_api::context::get_context().create_buffer(vertices * 2 * sizeof(float));
		mBuffers[VBO_TEXCOORD]->upload(0, vertices * 2 * sizeof(float), auxBuf);
	}
	else if (mType == GFX_COLOUR)
	{
		// reusing texture buffer for colours for now
		mBuffers[VBO_TEXCOORD] = gfx_api::context::get_context().create_buffer(vertices * 4 * sizeof(char));
		mBuffers[VBO_TEXCOORD]->upload(0, vertices * 4 * sizeof(char), auxBuf);
	}
	mSize = vertices;
}

static auto
get_gl_draw_type(const gfx_api::drawtype& dt)
{
	switch (dt)
	{
	case gfx_api::drawtype::line_strip: return GL_LINE_STRIP;
	case gfx_api::drawtype::triangles: return GL_TRIANGLES;
	case gfx_api::drawtype::triangle_fan: return GL_TRIANGLE_FAN;
	case gfx_api::drawtype::triangle_strip: return GL_TRIANGLE_STRIP;
	}
	throw;
}

void GFX::draw()
{
	if (mType == GFX_TEXTURE)
	{
		pie_SetTexturePage(TEXPAGE_EXTERN);
		const auto& as_gl_texture = dynamic_cast<gfx_api::gl_api::gl_texture&>(*mTexture);
		glBindTexture(GL_TEXTURE_2D, as_gl_texture.object);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		const auto& as_gl_buffer = dynamic_cast<gfx_api::gl_api::gl_buffer&>(*mBuffers[VBO_TEXCOORD]);
		glBindBuffer(GL_ARRAY_BUFFER, as_gl_buffer.object); glTexCoordPointer(2, GL_FLOAT, 0, NULL);
	}
	else if (mType == GFX_COLOUR)
	{
		pie_SetTexturePage(TEXPAGE_NONE);
		glEnableClientState(GL_COLOR_ARRAY);
		const auto& as_gl_buffer = dynamic_cast<gfx_api::gl_api::gl_buffer&>(*mBuffers[VBO_TEXCOORD]);
		glBindBuffer(GL_ARRAY_BUFFER, as_gl_buffer.object); glColorPointer(4, GL_UNSIGNED_BYTE, 0, NULL);
	}
	glEnableClientState(GL_VERTEX_ARRAY);
	const auto& as_gl_buffer = dynamic_cast<gfx_api::gl_api::gl_buffer&>(*mBuffers[VBO_VERTEX]);
	glBindBuffer(GL_ARRAY_BUFFER, as_gl_buffer.object); glVertexPointer(mCoordsPerVertex, GL_FLOAT, 0, NULL);
	glDrawArrays(get_gl_draw_type(mdrawType), 0, mSize);
	glDisableClientState(GL_VERTEX_ARRAY);
	if (mType == GFX_TEXTURE)
	{
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	}
	else if (mType == GFX_COLOUR)
	{
		glDisableClientState(GL_COLOR_ARRAY);
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

GFX::~GFX()
{
}
