#pragma once

#include <string>
#include <vector>


//*************************************************************************
//
// immitmap image structures
//
//*************************************************************************

struct ImageDef
{
	unsigned int TPageID;   /**< Which associated file to read our info from */
	unsigned int Tu;        /**< First vertex coordinate */
	unsigned int Tv;        /**< Second vertex coordinate */
	unsigned int Width;     /**< Width of image */
	unsigned int Height;    /**< Height of image */
	int XOffset;            /**< X offset into source position */
	int YOffset;            /**< Y offset into source position */

	int textureId;		///< duplicate of below, fix later
	float invTextureSize;
};

struct Image;

struct IMAGEFILE
{
	struct Page
	{
		int id;    /// OpenGL texture ID.
		int size;  /// Size of texture in pixels. (Should be square.)
	};

	Image find(std::string const &name);  // Defined in bitimage.cpp.

	std::vector<Page> pages;          /// Texture pages.
	std::vector<ImageDef> imageDefs;  /// Stored images.
	std::vector<std::pair<std::string, int> > imageNames;  ///< Names of images, sorted by name. Can lookup indices from name.
};

struct Image
{
	Image(IMAGEFILE const *images = NULL, unsigned id = 0) : images(const_cast<IMAGEFILE *>(images)), id(id) {}

	bool isNull() const
	{
		return images == nullptr;
	}
	int width() const
	{
		return images->imageDefs[id].Width;
	}
	int height() const
	{
		return images->imageDefs[id].Height;
	}
	int xOffset() const
	{
		return images->imageDefs[id].XOffset;
	}
	int yOffset() const
	{
		return images->imageDefs[id].YOffset;
	}

	IMAGEFILE *images;
	unsigned id;
};
