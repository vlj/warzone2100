#pragma once

#include "lib/framework/opengl.h"
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
	GFX(GFXTYPE type, GLenum drawType = GL_TRIANGLES, int coordsPerVertex = 3);

	/// Destroy GPU held resources
	~GFX();

	/// Load texture data from file, allocate space for it, and put it on the GPU
	void loadTexture(const char *filename, GLenum filter = GL_LINEAR);

	/// Allocate space on the GPU for texture of given parameters. If image is non-NULL,
	/// then that memory buffer is uploaded to the GPU.
	void makeTexture(int width, int height, GLenum filter = GL_LINEAR, GLenum format = GL_RGBA, const GLvoid *image = NULL);

	/// Upload given memory buffer to already allocated texture space on the GPU
	void updateTexture(const GLvoid *image, int width = -1, int height = -1);

	/// Upload vertex and texture buffer data to the GPU
	void buffers(int vertices, const GLvoid *vertBuf, const GLvoid *texBuf);

	/// Draw everything
	void draw();

private:
	GFXTYPE mType;
	GLenum mFormat;
	int mWidth;
	int mHeight;
	GLenum mdrawType;
	int mCoordsPerVertex;
	GLuint mBuffers[VBO_COUNT];
	GLuint mTexture;
	int mSize;
};

