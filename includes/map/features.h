#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <memory>

struct LND_OBJECT_
{
	uint32_t id;
	uint32_t player;
	int type; // "IMD" LND object type
	char name[128];
	char script[32];
	uint32_t x, y, z;
	uint32_t direction;
};

struct FeatureData
{
	uint32_t featVersion;
	std::vector<LND_OBJECT_> mLndObjects;
};

std::unique_ptr<FeatureData> loadFeatures(const std::string &filename);