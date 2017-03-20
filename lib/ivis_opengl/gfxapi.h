#pragma once

#include <string>
#include <tuple>
#include "lib/framework/vector.h"

/// gaphics performance measurement points
enum class PERF_POINT
{
	PERF_START_FRAME,
	PERF_EFFECTS,
	PERF_TERRAIN,
	PERF_SKYBOX,
	PERF_MODEL_INIT,
	PERF_PARTICLES,
	PERF_WATER,
	PERF_MODELS,
	PERF_MISC,
	PERF_GUI,
	PERF_COUNT
};

enum class PIPELINE_STATE
{
	terrain,
};

struct OPENGL_DATA
{
	std::string vendor;
	std::string renderer;
	std::string version;
	std::string GLEWversion;
	std::string GLSLversion;
};

struct PER_FRAME_DATA
{
	//PIELIGHT			fogColour;
	float				fogBegin;
	float				fogEnd;
	PIELIGHT			sunColor;
	Vector3f			sunPosition;
};

struct gfxAPI
{
	virtual OPENGL_DATA getInfo() = 0;
	virtual void pie_ScreenFlip(int ClearMode) = 0;

	auto getScreenDimension() const
	{
		return std::make_tuple(screenWidth, screenHeight);
	}

	virtual void wzPerfBegin(PERF_POINT pp, const char *descr) = 0;
	virtual void wzPerfEnd(PERF_POINT pp) = 0;

	virtual void setPipelineState(const PIPELINE_STATE&) = 0;

	virtual void setPerFrameConstants() = 0;

private:
	unsigned screenWidth;
	unsigned screenHeight;
};
