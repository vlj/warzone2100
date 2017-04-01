#include "imdShape.h"


iIMDShape::iIMDShape()
{
	flags = 0;
	nconnectors = 0; // Default number of connectors must be 0
	npoints = 0;
	npolys = 0;
	points = NULL;
	polys = NULL;
	connectors = NULL;
	next = NULL;
	shadowEdgeList = NULL;
	nShadowEdges = 0;
	texpage = iV_TEX_INVALID;
	tcmaskpage = iV_TEX_INVALID;
	normalpage = iV_TEX_INVALID;
	specularpage = iV_TEX_INVALID;
	numFrames = 0;
	shaderProgram = SHADER_NONE;
	objanimtime = 0;
	objanimcycles = 0;
}

iIMDShape::~iIMDShape()
{
	if (points)
	{
		free(points);
	}
	if (connectors)
	{
		free(connectors);
	}
	if (polys)
	{
		for (unsigned i = 0; i < npolys; i++)
		{
			if (polys[i].texCoord)
			{
				free(polys[i].texCoord);
			}
		}
		free(polys);
	}
	if (shadowEdgeList)
	{
		free(shadowEdgeList);
		shadowEdgeList = NULL;
	}
	// shader deleted later, if any
	iIMDShape *d = next;
	delete d;
}


