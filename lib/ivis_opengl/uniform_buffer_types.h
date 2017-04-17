#pragma once

#include "lib/framework/vector.h"

namespace ivis
{
	struct TerrainDepthUniforms
	{
		glm::mat4 ModelViewProjectionMatrix;
		glm::vec4 paramsXLight;
		glm::vec4 paramsYLight;
	};

	struct TerrainLayers
	{
		glm::mat4 ModelViewProjection;
		glm::vec4 paramsX;
		glm::vec4 paramsY;
		glm::vec4 paramsXLight;
		glm::vec4 paramsYLight;
		glm::mat4 textureMatrix;
		bool fogEnabled;
		float fogBegin;
		float fogEnd;
		glm::vec4 fogColor;
	};

	struct TerrainDecals
	{
		glm::mat4 ModelViewProjection;
		glm::vec4 paramsXLight;
		glm::vec4 paramsYLight;
		glm::mat4 textureMatrix;
		bool fogEnabled;
		float fogBegin;
		float fogEnd;
		glm::vec4 fogColor;
	};
}
