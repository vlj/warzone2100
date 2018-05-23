#include <map/features.h>
#include "lib/framework/physfs_ext.h" // Also includes physfs.h
#include "lib/framework/frame.h"

constexpr auto kVersion14 = 14u;
constexpr auto kVersion19 = 19u;


std::unique_ptr<FeatureData> loadFeatures(const std::string& filename)
{
	const auto *path = filename.c_str();
	auto fp = PHYSFS_openRead(path);
	if (!fp)
	{
		//debug(LOG_ERROR, "Feature file %s not found", path);
		return nullptr;
	}

	char aFileType[4];
	uint32_t numFeatures;
	FeatureData result;
	if (WZ_PHYSFS_readBytes(fp, aFileType, 4) != 4 || aFileType[0] != 'f' || aFileType[1] != 'e' ||
		aFileType[2] != 'a' || aFileType[3] != 't' || !PHYSFS_readULE32(fp, &result.featVersion) ||
		!PHYSFS_readULE32(fp, &numFeatures))
	{
		//debug(LOG_ERROR, "Bad features header in %s", path);
		PHYSFS_close(fp);
		return nullptr;
	}
	for (int i = 0; i < numFeatures; i++)
	{
		LND_OBJECT_ psObj{};
		int nameLength = 60;
		uint32_t dummy;
		uint8_t visibility[8];

		if (result.featVersion <= kVersion19)
		{
			nameLength = 40;
		}
		if (WZ_PHYSFS_readBytes(fp, psObj.name, nameLength) != nameLength || !PHYSFS_readULE32(fp, &psObj.id) ||
			!PHYSFS_readULE32(fp, &psObj.x) || !PHYSFS_readULE32(fp, &psObj.y) || !PHYSFS_readULE32(fp, &psObj.z) ||
			!PHYSFS_readULE32(fp, &psObj.direction) || !PHYSFS_readULE32(fp, &psObj.player) ||
			!PHYSFS_readULE32(fp, &dummy)	 // BOOL inFire
			|| !PHYSFS_readULE32(fp, &dummy)																// burnStart
			|| !PHYSFS_readULE32(fp, &dummy))																// burnDamage
		{
			//debug(LOG_ERROR, "Failed to read feature from %s", path);
			PHYSFS_close(fp);
			return nullptr;
		}
		psObj.player = 0; // work around invalid feature owner
		if (result.featVersion >= kVersion14 && WZ_PHYSFS_readBytes(fp, &visibility, 8) != 8)
		{
			//debug(LOG_ERROR, "Failed to read feature visibility from %s", path);
			PHYSFS_close(fp);
			return nullptr;
		}
		result.mLndObjects.emplace_back(psObj);
	}
	PHYSFS_close(fp);
	return std::make_unique<FeatureData>(result);
}