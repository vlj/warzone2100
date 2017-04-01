#pragma once

#include "api_object.h"
#include "pietypes.h"

enum GFXTYPE
{
	GFX_TEXTURE,
	GFX_COLOUR,
	GFX_COUNT
};

/// Generic graphics using VBOs drawing class
class GFX
{
public:
	/// Initialize class and allocate GPU resources
	GFX(GFXTYPE type, gfx_api::drawtype drawType = gfx_api::drawtype::triangles, int coordsPerVertex = 3);

	/// Destroy GPU held resources
	~GFX();

	/// Load texture data from file, allocate space for it, and put it on the GPU
	void loadTexture(const char *filename, gfx_api::filter filter = gfx_api::filter::linear);

	/// Allocate space on the GPU for texture of given parameters. If image is non-NULL,
	/// then that memory buffer is uploaded to the GPU.
	void makeTexture(int width, int height, gfx_api::filter filter = gfx_api::filter::linear, gfx_api::format format = gfx_api::format::rgba, const void *image = nullptr);

	/// Upload given memory buffer to already allocated texture space on the GPU
	void updateTexture(const void *image, int width = -1, int height = -1);

	/// Upload vertex and texture buffer data to the GPU
	void buffers(int vertices, const void *vertBuf, const void *texBuf);

	/// Draw everything
	void draw();

private:
	GFXTYPE mType;
	gfx_api::format mFormat;
	int mWidth;
	int mHeight;
	gfx_api::drawtype mdrawType;
	int mCoordsPerVertex;
	std::array<std::unique_ptr<gfx_api::buffer>, VBO_COUNT> mBuffers;
	std::unique_ptr<gfx_api::texture> mTexture;
	int mSize;
};

