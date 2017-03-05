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

/*!
 * \file imdload.c
 *
 * Load IMD (.pie) files
 */

#include <numeric>
#include <sstream>
#include <optional>
#include <array>
#include <memory>
#include <map>

# include "lib/framework/file.h"
#include "ivisdef.h" // for imd structures
#include "imd.h" // for imd structures
 //#include "tex.h" // texture page loading
#include "piematrix.h"

// Scale animation numbers from int to float
#define INT_SCALE       1000

using MODELMAP = std::map<std::string, iIMDShape *>;
static MODELMAP models;

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
/*	texpage = iV_TEX_INVALID;
	tcmaskpage = iV_TEX_INVALID;
	normalpage = iV_TEX_INVALID;
	specularpage = iV_TEX_INVALID;*/
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
	glDeleteBuffers(VBO_COUNT, buffers);
	// shader deleted later, if any
	iIMDShape *d = next;
	delete d;
}

void modelShutdown()
{
	for (const auto& pair : models)
	{
		delete pair.second;
	}
	models.clear();
}


iIMDShape *iV_ProcessIMD(const std::string& filename, const char **ppFileData, const char *FileDataEnd);

static bool tryLoad(const std::string &path, const std::string &filename)
{
	if (PHYSFS_exists((path + filename).c_str()))
	{
		char *pFileData = NULL, *fileEnd;
		UDWORD size = 0;
		if (!loadFile((path + filename).c_str(), &pFileData, &size))
		{
			debug(LOG_ERROR, "Failed to load model file: %s", (path + filename).c_str());
			return false;
		}
		fileEnd = pFileData + size;
		iIMDShape *s = iV_ProcessIMD(filename, (const char **)&pFileData, fileEnd);
		if (s)
		{
			models[filename] = s;
			//models.insert(filename, s);
		}
		return true;
	}
	return false;
}

std::string modelName(iIMDShape *model)
{
	const auto& It = std::find_if(models.begin(), models.end(), [&](const auto& pair) { return pair.second == model; });
	if (It != models.end())
		return It->first;
	ASSERT(false, "An IMD pointer could not be backtraced to a filename!");
	return std::string();
}

iIMDShape *modelGet(const std::string &filename)
{
	const auto& It = models.find(filename);
	const auto& name = filename;
	if (It != models.end())
		return It->second;
	else if (tryLoad("structs/", name) || tryLoad("misc/", name) || tryLoad("effects/", name)
	         || tryLoad("components/prop/", name) || tryLoad("components/weapons/", name)
	         || tryLoad("components/bodies/", name) || tryLoad("features/", name)
	         || tryLoad("misc/micnum/", name) || tryLoad("misc/minum/", name) || tryLoad("misc/mivnum/", name) || tryLoad("misc/researchimds/", name))
	{
		return models[filename];
	}
	debug(LOG_ERROR, "Could not find: %s", name.c_str());
	return NULL;
}

static auto
get_texture_coordinates(std::istringstream & stream, const int& pieVersion, const int& nFrames, const int& framesPerLine, const Vector2f texAnim)
{
    std::unique_ptr<Vector2f[]> tc{new Vector2f[nFrames * 3]};
    for (unsigned j = 0; j < 3; j++)
    {
        float VertexU, VertexV;
        stream >> VertexU >> VertexV;

        if (pieVersion != PIE_FLOAT_VER)
        {
            VertexU /= OLD_TEXTURE_SIZE_FIX;
            VertexV /= OLD_TEXTURE_SIZE_FIX;
        }

        for (int frame = 0; frame < nFrames; frame++)
        {
            const float uFrame = (frame % framesPerLine) * texAnim.x;
            const float vFrame = (frame / framesPerLine) * texAnim.y;
            Vector2f &c = tc[frame * 3 + j];

            c.x = VertexU + uFrame;
            c.y = VertexV + vFrame;
        }
    }
    return tc;
}

static auto
get_potentially_animated_texcoordinate_info(std::istringstream& stream, const int& flags, const std::string& filename, const int& pieVersion)
{
    if (!(flags & iV_IMD_TEXANIM))
    {
        return std::make_tuple(1, 1, 1, Vector2f{ 0, 0 });
    }
    int nFrames, framesPerLine, pbRate;
    float tWidth, tHeight;
    stream >> nFrames >> pbRate >> tWidth >> tHeight;

    ASSERT(tWidth > 0.0001f, "%s: texture width = %f", filename.c_str(), tWidth);
    ASSERT(tHeight > 0.f, "%s: texture height = %f (width=%f)", filename.c_str(), tHeight, tWidth);
    ASSERT(nFrames > 1, "%s: animation frames = %d", filename.c_str(), nFrames);
    ASSERT(pbRate > 0, "%s: animation interval = %d ms", filename.c_str(), pbRate);

    if (pieVersion != PIE_FLOAT_VER)
    {
        tWidth /= OLD_TEXTURE_SIZE_FIX;
        tHeight /= OLD_TEXTURE_SIZE_FIX;
    }
    framesPerLine = 1 / tWidth;
    return std::make_tuple(nFrames, framesPerLine, pbRate, Vector2f{ tWidth, tHeight });
}

/*!
 * Load shape level polygons
 * \param ppFileData Pointer to the data (usualy read from a file)
 * \param s Pointer to shape level
 * \return false on error (memory allocation failure/bad file format), true otherwise
 * \pre ppFileData loaded
 * \pre s allocated
 * \pre s->npolys set
 * \post s->polys allocated (iFSDPoly * s->npolys)
 * \post s->pindex allocated for each poly
 */
static bool _imd_load_polys(const std::string &filename, std::istringstream& stream, iIMDShape *s, int pieVersion)
{
	unsigned int i;
	iIMDPoly *poly;

	s->numFrames = 0;
	s->animInterval = 0;

	s->polys = (iIMDPoly *)malloc(sizeof(iIMDPoly) * s->npolys);
	if (s->polys == NULL)
	{
		debug(LOG_ERROR, "(_load_polys) Out of memory (polys)");
		return false;
	}

	for (i = 0, poly = s->polys; i < s->npolys; i++, poly++)
	{
		unsigned int flags, npnts;

		stream >> std::hex >> flags >> std::dec >> npnts;

		poly->flags = flags;
		ASSERT_OR_RETURN(false, npnts == 3, "Invalid polygon size (%d)", npnts);
		stream >> poly->pindex[0] >> poly->pindex[1] >> poly->pindex[2];

		// calc poly normal
		poly->normal = pie_SurfaceNormal3fv(
                s->points[poly->pindex[0]],
                s->points[poly->pindex[1]],
                s->points[poly->pindex[2]]);

		// texture coord routine
		if (poly->flags & iV_IMD_TEX)
		{
			int nFrames, framesPerLine, pbRate;
            Vector2f texAnim;
            std::tie(nFrames, framesPerLine, pbRate, texAnim) = get_potentially_animated_texcoordinate_info(stream, flags, filename, pieVersion);

            /* Must have same number of frames and same playback rate for all polygons */
            if (nFrames > 1)
            {
                if (s->numFrames == 0)
                {
                    s->numFrames = nFrames;
                    s->animInterval = pbRate;
                }
                ASSERT(s->numFrames == nFrames,
                    "%s: varying number of frames within one PIE level: %d != %d",
                    filename.c_str(), nFrames, s->numFrames);
                ASSERT(s->animInterval == pbRate,
                    "%s: varying animation intervals within one PIE level: %d != %d",
                    filename.c_str(), pbRate, s->animInterval);
            }

            const auto& tc = get_texture_coordinates(stream, pieVersion, nFrames, framesPerLine, texAnim);
            poly->texAnim = texAnim;
			poly->texCoord = (Vector2f *)malloc(sizeof(*poly->texCoord) * nFrames * 3);
            memcpy(poly->texCoord, tc.get(), sizeof(Vector2f) * nFrames * 3);
		}
		else
		{
			ASSERT_OR_RETURN(false, !(poly->flags & iV_IMD_TEXANIM), "Polygons with texture animation must have textures!");
			poly->texCoord = NULL;
		}
	}
	return true;
}

static auto get_maximally_separated_pair(const Vector3f *p, const int& size)
{
    // Get extrema points in x, y, z direction
    const auto& vxmin_vymin_vzmin_vxmax_vymax_vzmax =
        std::accumulate(p, p + size,
            std::make_tuple(
                Vector3f{ FP12_MULTIPLIER, 0, 0 }, Vector3f{ 0, FP12_MULTIPLIER, 0 }, Vector3f{ 0, 0, FP12_MULTIPLIER },
                Vector3f{ -FP12_MULTIPLIER, 0, 0 }, Vector3f{ 0, -FP12_MULTIPLIER, 0 }, Vector3f{ 0, 0, -FP12_MULTIPLIER }
            ),
            [](auto&& current_vmins_vmaxs, const auto& current_vector)
            {
                auto&& vxmin = std::get<0>(current_vmins_vmaxs);
                auto&& vymin = std::get<1>(current_vmins_vmaxs);
                auto&& vzmin = std::get<2>(current_vmins_vmaxs);
                auto&& vxmax = std::get<3>(current_vmins_vmaxs);
                auto&& vymax = std::get<4>(current_vmins_vmaxs);
                auto&& vzmax = std::get<5>(current_vmins_vmaxs);

                if (current_vector.x < vxmin.x) vxmin = current_vector;
                if (current_vector.x > vxmax.x) vxmax = current_vector;
                if (current_vector.y < vymin.y) vymin = current_vector;
                if (current_vector.y > vymax.y) vymax = current_vector;
                if (current_vector.z < vzmin.z) vzmin = current_vector;
                if (current_vector.z > vzmax.z) vzmax = current_vector;
                return std::make_tuple(
                    vxmin, vymin, vzmin,
                    vxmax, vymax, vzmax
                );
            }
        );

    const auto& vxmin = std::get<0>(vxmin_vymin_vzmin_vxmax_vymax_vzmax);
    const auto& vymin = std::get<1>(vxmin_vymin_vzmin_vxmax_vymax_vzmax);
    const auto& vzmin = std::get<2>(vxmin_vymin_vzmin_vxmax_vymax_vzmax);
    const auto& vxmax = std::get<3>(vxmin_vymin_vzmin_vxmax_vymax_vzmax);
    const auto& vymax = std::get<4>(vxmin_vymin_vzmin_vxmax_vymax_vzmax);
    const auto& vzmax = std::get<5>(vxmin_vymin_vzmin_vxmax_vymax_vzmax);

    // set xspan = distance between 2 points xmin & xmax (squared)
    const auto& xspan = pow(double{ glm::distance(vxmin, vxmax) }, 2);
    // same for yspan
    const auto& yspan = pow(double{ glm::distance(vymin, vzmax) }, 2);
    // and ofcourse zspan
    const auto& zspan = pow(double{ glm::distance(vzmin, vzmax) }, 2);

    Vector3f dia1, dia2;
    dia1 = vxmin;
    dia2 = vxmax;
    // assume xspan biggest
    double maxspan = xspan;

    if (yspan > maxspan)
    {
        maxspan = yspan;
        dia1 = vymin;
        dia2 = vymax;
    }

    if (zspan > maxspan)
    {
        dia1 = vzmin;
        dia2 = vzmax;
    }
    return std::make_tuple(dia1, dia2);
}

static auto get_tight_bounding_sphere(const Vector3f *p, const int& size)
{
	// set points dia1 & dia2 to maximally seperated pair
    const auto& maximally_separated_pair = get_maximally_separated_pair(p, size);
	const auto& dia1 = std::get<0>(maximally_separated_pair);
	const auto& dia2 = std::get<1>(maximally_separated_pair);

	// dia1, dia2 diameter of initial sphere
	const auto& initial_center = (dia1 + dia2) / 2.f;
	// calc initial radius
	const auto& initial_radius = glm::distance(dia2, initial_center);

	const auto& cen_radius = std::accumulate(
		p, p + size,
		std::make_tuple(initial_center, initial_radius),
		[](const auto& current_cen_radius, const auto& current_vector)
		{
			const auto& cen = std::get<0>(current_cen_radius);
			const auto& radius = std::get<1>(current_cen_radius);
			//	 rad_sq, rad, old_to_p_sq, old_to_p, old_to_new;
			const auto& old_to_p = glm::distance(current_vector, cen);

			if (old_to_p <= radius)
				return current_cen_radius;

			// this point outside current sphere
			// radius of new sphere
			const auto& new_radius = (radius + old_to_p) / 2.f;
			const auto& old_to_new = old_to_p - new_radius;
			// centre of new sphere
			const auto& new_cen = (new_radius * cen + old_to_new * current_vector) / old_to_p;
			return std::make_tuple(new_cen, new_radius);
		}
	);
    return std::get<0>(cen_radius);
}

void _imd_calc_bounds(iIMDShape *s, Vector3f *p, int size)
{
	// set up bounding data for minimum number of vertices
	std::tie (s->min, s->max) =
		std::accumulate(p, p + size,
			std::make_tuple(
				Vector3i{ FP12_MULTIPLIER, FP12_MULTIPLIER, FP12_MULTIPLIER },
				Vector3i{ -FP12_MULTIPLIER, -FP12_MULTIPLIER, -FP12_MULTIPLIER }
			),
			[](const auto& current_min_max, const auto& current_vector)
			{
				return std::make_tuple(
					glm::min(std::get<0>(current_min_max), Vector3i{ current_vector }),
					glm::max(std::get<1>(current_min_max), Vector3i{ current_vector })
				);
			}
		);

	// no need to scale an IMD shape (only FSD)
	const auto& xmax = std::max(s->max.x, -s->min.x);
	const auto& ymax = std::max(s->max.y, -s->min.y);
	const auto& zmax = std::max(s->max.z, -s->min.z);

	s->radius = std::max(xmax, std::max(ymax, zmax));
	s->sradius = sqrtf(xmax * xmax + ymax * ymax + zmax * zmax);
    s->ocen = get_tight_bounding_sphere(p, size);
}

template<typename T>
auto load_vector3(std::istringstream& stream, const unsigned& vectors_count)
{
	std::unique_ptr<T[]> vectors{ new T[vectors_count] };
	std::for_each(vectors.get(), vectors.get() + vectors_count, [&stream](auto& vector)
	{
		stream >> vector.x >> vector.y >> vector.z;
	});
	return vectors;
}

// performance hack
/*static QVector<Vector3f> vertices;
static QVector<Vector3f> normals;
static QVector<Vector2f> texcoords;
static QVector<GLfloat> tangents;
static QVector<uint16_t> indices; // size is npolys * 3 * numFrames*/
static int vertexCount;

/*static inline int addVertex(iIMDShape *s, int i, const iIMDPoly *p, int frameidx)
{
	// if texture animation flag is present, fetch animation coordinates for this polygon
	// otherwise just show the first set of texel coordinates
	int frame = (p->flags & iV_IMD_TEXANIM) ? frameidx : 0;

	// See if we already have this defined, if so, return reference to it.
	for (int j = 0; j < vertexCount; j++)
	{
		if (texcoords[j] == p->texCoord[frame * 3 + i]
		    && vertices[j] == s->points[p->pindex[i]]
		    && normals[j] == p->normal)
		{
			return j;
		}
	}
	// We don't have it, add it.
	normals.append(p->normal);
	texcoords.append(p->texCoord[frame * 3 + i]);
	vertices.append(s->points[p->pindex[i]]);
	vertexCount++;
	return vertexCount - 1;
}*/

static auto get_directive(std::istringstream &stream)
{
	std::string directive;
	int argument;
	stream >> directive >> argument;
	return std::make_tuple(directive, argument);
}

static auto get_checked_directive_s_argument(std::istringstream &stream, const std::string &filename, const std::string& expected_directive)
{
	const auto &directive_and_arg = get_directive(stream);
	const auto& directive = std::get<0>(directive_and_arg);
	if (directive != expected_directive)
	{
		debug(LOG_ERROR, "%s: Expecting '%s' directive, got: %s", filename.c_str(), expected_directive.c_str(), directive.c_str());
		throw std::exception();
	}
	return std::get<1>(directive_and_arg);
}

static auto
get_object_anim_frame(std::istringstream & stream, const int &i, const std::string & filename)
{
    int frame;
    Vector3i pos, rot;
    Vector3f scale;

    stream >> frame >> pos.x >> pos.y >> pos.z
        >> rot.x >> rot.y >> rot.z
        >> scale.x >> scale.y >> scale.z;

    ASSERT(frame == i, "%s: Invalid frame enumeration object animation %d: %d", filename.c_str(), i, frame);
    return ANIMFRAME{
        Position{pos / INT_SCALE},
        Rotation(
            -(rot.z * DEG_1 / INT_SCALE),
            -(rot.x * DEG_1 / INT_SCALE),
            -(rot.y * DEG_1 / INT_SCALE)
        ),
        scale
    };
}

void issue_gfx_copy(iIMDShape * s)
{
	glGenBuffers(VBO_COUNT, s->buffers);
	vertexCount = 0;
	for (int k = 0; k < MAX(1, s->numFrames); k++)
	{
		// Go through all polygons for each frame
		for (unsigned i = 0; i < s->npolys; i++)
		{
			const iIMDPoly *pPolys = &s->polys[i];

			// Do we already have the vertex data for this polygon?
//			indices.append(addVertex(s, 0, pPolys, k));
//			indices.append(addVertex(s, 1, pPolys, k));
	//		indices.append(addVertex(s, 2, pPolys, k));
		}
	}

/*	glBindBuffer(GL_ARRAY_BUFFER, s->buffers[VBO_VERTEX]);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vector3f), vertices.constData(), GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, s->buffers[VBO_NORMAL]);
	glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(Vector3f), normals.constData(), GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s->buffers[VBO_INDEX]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint16_t), indices.constData(), GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, s->buffers[VBO_TEXCOORD]);
	glBufferData(GL_ARRAY_BUFFER, texcoords.size() * sizeof(Vector2f), texcoords.constData(), GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0); // unbind

	indices.resize(0);
	vertices.resize(0);
	texcoords.resize(0);
	normals.resize(0);*/
}

/*!
 * Load shape levels recursively
 * \param ppFileData Pointer to the data (usualy read from a file)
 * \param FileDataEnd ???
 * \param nlevels Number of levels to load
 * \return pointer to iFSDShape structure (or NULL on error)
 * \pre ppFileData loaded
 * \post s allocated
 */
static auto _imd_load_level(const std::string &filename, std::istringstream& stream, const int& pieVersion)
{
    std::string directive;
    stream >> directive;

    iIMDShape *s = new iIMDShape;

	// Optionally load and ignore deprecated MATERIALS directive
	if (directive == "MATERIALS")
	{
		float dummy;
        for (unsigned i = 0; i < 11; ++i)
            stream >> dummy;
		debug(LOG_WARNING, "MATERIALS directive no longer supported!");
	}
	else if (directive == "SHADERS")
	{
        std::string vertex;
        std::string fragment;

        stream >> vertex >> fragment;

		std::vector<std::string> uniform_names { "colour", "teamcolour", "stretch", "tcmask", "fogEnabled", "normalmap",
		                                         "specularmap", "ecmEffect", "alphaTest", "graphicsCycle", "ModelViewProjectionMatrix" };
		//s->shaderProgram = pie_LoadShader(filename.c_str(), vertex.c_str(), fragment.c_str(), uniform_names);
	}

	stream >> s->npoints;

	// load points

	//ASSERT_OR_RETURN(NULL, directive == "POINTS", "Expecting 'POINTS' directive, got: %s", directive);

	//load the points then pass through a second time to setup bounding datavalues
	const auto&& points = load_vector3<Vector3f>(stream, s->npoints);
	_imd_calc_bounds(s, points.get(), s->npoints);

	s->points = static_cast<Vector3f*>(malloc(sizeof(Vector3f) * s->npoints));
	memcpy(s->points, points.get(), sizeof(Vector3f) * s->npoints);

	s->npolys = get_checked_directive_s_argument(stream, filename, "POLYGONS");
	_imd_load_polys(filename, stream, s, pieVersion);

	// optional stuff : levels, object animations, connectors
	s->objanimframes = 0;
	int n = 0;
	while (stream >> directive >> n) // Scans in the line ... if we don't get 2 parameters then quit
	{
		if (directive == "LEVEL") // check for next level
		{
			debug(LOG_3D, "imd[_load_level] = npoints %d, npolys %d", s->npoints, s->npolys);
			return std::make_tuple(s, n);
//			s->next = _imd_load_level(filename, stream, nlevels - 1, pieVersion, level + 1);
		}
		else if (directive == "CONNECTORS")
		{
			//load connector stuff
			s->nconnectors = n;
			
			const auto& connectors = load_vector3<Vector3i>(stream, s->nconnectors);
			s->connectors = (Vector3i *)malloc(sizeof(Vector3i) * s->nconnectors);
			memcpy(s->connectors, connectors.get(), sizeof(Vector3i) * s->nconnectors);
		}
		else if (directive == "ANIMOBJECT")
		{
			s->objanimtime = n;
            stream >> s->objanimcycles >> s->objanimframes;
            for (unsigned i = 0; i < s->objanimframes; ++i)
            {
                s->objanimdata.push_back(get_object_anim_frame(stream, i, filename));
            }
		}
		else
		{
			throw std::exception("(_load_level) unexpected directive");
		}
	}
	// We reached end of file
	return std::make_tuple(s, -1);
}

static auto get_pie_version(const std::string& filename, std::istringstream &stream)
{
    std::string buffer;
    int32_t imd_version;
    stream >> buffer;
    stream >> imd_version;
    if (buffer != PIE_NAME)
    {
        debug(LOG_ERROR, "%s: Not an IMD file (%s %d)", filename.c_str(), buffer, imd_version);
        return NULL;
    }

    //Now supporting version PIE_VER and PIE_FLOAT_VER files
    if (imd_version != PIE_VER && imd_version != PIE_FLOAT_VER)
    {
        debug(LOG_ERROR, "%s: Version %d not supported", filename.c_str(), imd_version);
        return NULL;
    }

    return imd_version;
}

static auto get_flags(std::istringstream &stream)
{
    std::string buffer;
    uint32_t imd_flags;
    stream >> buffer >> imd_flags;
    return imd_flags;
}

struct textureData
{
    std::optional<std::string> texture_path;
    std::optional<std::string> normalmap_path;
    std::optional<std::string> specularmap_path;
};

static auto process_textures(std::istringstream &stream, const std::string& filename)
{
    auto keyword = get_directive(stream);
    std::optional<std::string> opt_texture_path;
    std::optional<std::string> opt_normalmap_path;
    std::optional<std::string> opt_specularmap_path;

    if (std::get<0>(keyword) == "TEXTURE")
    {
        std::string texture_path;
        /* the first parameter for textures is always ignored; which is why we ignore
        * nlevels read in above */
        int pwidth, pheight;
        stream >> texture_path >> pwidth >> pheight;
        if (texture_path.substr(texture_path.length() - 4, 3) == "png")
        {
            debug(LOG_ERROR, "%s: Only png textures supported", filename.c_str());
            //return NULL;
        }
        keyword = get_directive(stream);
        opt_texture_path = texture_path;
    }

    if (std::get<0>(keyword) == "NORMALMAP")
    {
        std::string normalmap_path;
        /* the first parameter for textures is always ignored; which is why we ignore
        * nlevels read in above */
        stream >> normalmap_path;

        if (normalmap_path.substr(normalmap_path.size() - 4, 3) == "png")
        {
            debug(LOG_ERROR, "%s: Only png normal maps supported", filename.c_str());
            //return NULL;
        }
        keyword = get_directive(stream);
        opt_normalmap_path = normalmap_path;
    }

    if (std::get<0>(keyword) == "SPECULARMAP")
    {
        std::string specularmap_path;
        stream >> specularmap_path;

        /* the first parameter for textures is always ignored; which is why we ignore nlevels read in above */
        // Run up to the dot or till the buffer is filled. Leave room for the extension.
        if (specularmap_path.substr(specularmap_path.size() - 4, 3) == "png")
        {
            debug(LOG_ERROR, "%s: only png specular maps supported", filename.c_str());
            //return NULL;
        }
        keyword = get_directive(stream);
        opt_specularmap_path = specularmap_path;
    }
    return std::make_tuple(keyword, textureData{opt_texture_path, opt_normalmap_path, opt_specularmap_path});
}

/*!
 * Load ppFileData into a shape
 * \param ppFileData Data from the IMD file
 * \param FileDataEnd Endpointer
 * \return The shape, constructed from the data read
 */
// ppFileData is incremented to the end of the file on exit!
iIMDShape *iV_ProcessIMD(const std::string& filename, const char **ppFileData, const char *FileDataEnd)
{
    std::string file_string(*ppFileData);
    std::istringstream stream(file_string);

    const auto& imd_version = get_pie_version(filename, stream);

	// Read flag
    const auto& imd_flags = get_flags(stream);

	/* This can be either texture or levels */
    // get texture page if specified
    const auto texture_process_output = process_textures(stream, filename);
    auto keyword = std::get<0>(texture_process_output);
    std::array<iIMDShape *, ANIM_EVENT_COUNT> objanimpie{};
	while (std::get<0>(keyword) == "EVENT")
	{
		std::string animpie;

		//ASSERT(nlevels < ANIM_EVENT_COUNT && nlevels >= 0, "Invalid event type %d", nlevels);
		stream >> animpie;
		objanimpie[std::get<1>(keyword)] = modelGet(animpie.c_str());

		/* Try -yet again- to read in LEVELS directive */
		keyword = get_directive(stream);
	}

	if (std::get<0>(keyword) != "LEVELS")
	{
		//debug(LOG_ERROR, "%s: Expecting 'LEVELS' directive (%s)", filename.toUtf8().constData(), buffer);
		return NULL;
	}
	const auto nlevels = std::get<1>(keyword);

	/* Read first LEVEL directive */
	auto level = get_checked_directive_s_argument(stream, filename, "LEVEL");
	std::vector<iIMDShape*> levelList;
	for (unsigned i = 1; i <= nlevels; i++)
	{
		if (i != level) throw;
		iIMDShape *shape;
		std::tie(shape, level) = _imd_load_level(filename, stream, imd_version);
		if (!levelList.empty())
			levelList.back()->next = shape;
		levelList.push_back(shape);
		// FINALLY, massage the data into what can stream directly to OpenGL
		issue_gfx_copy(shape);
	}

	if (!stream.eof()) throw std::exception("Invalid content found after levels");

	iIMDShape *shape = levelList[0];
	if (shape == NULL)
	{
		debug(LOG_ERROR, "%s: Unsuccessful", filename.c_str());
		return NULL;
	}

	// load texture page if specified
    const auto& texture_data = std::get<1>(texture_process_output);
	/*if (texture_data.texture_path)
	{
		int texpage = iV_GetTexture(texture_data.texture_path.value().c_str());
		int normalpage = iV_TEX_INVALID;
		int specpage = iV_TEX_INVALID;

		//ASSERT_OR_RETURN(NULL, texpage >= 0, "%s could not load tex page %s", filename.toUtf8().constData(), texfile);

		if (texture_data.normalmap_path)
		{
			//debug(LOG_TEXTURE, "Loading normal map %s for %s", normalfile, filename.toUtf8().constData());
			normalpage = iV_GetTexture(texture_data.normalmap_path.value().c_str(), false);
			//ASSERT_OR_RETURN(NULL, normalpage >= 0, "%s could not load tex page %s", filename.toUtf8().constData(), normalfile);
		}

		if (texture_data.specularmap_path)
		{
			//debug(LOG_TEXTURE, "Loading specular map %s for %s", specfile, filename.toUtf8().constData());
			specpage = iV_GetTexture(texture_data.specularmap_path.value().c_str(), false);
			//ASSERT_OR_RETURN(NULL, specpage >= 0, "%s could not load tex page %s", filename.toUtf8().constData(), specfile);
		}

		// assign tex pages and flags to all levels
		for (iIMDShape *psShape = shape; psShape != NULL; psShape = psShape->next)
		{
			psShape->texpage = texpage;
			psShape->normalpage = normalpage;
			psShape->specularpage = specpage;
			psShape->flags = imd_flags;
		}

		// check if model should use team colour mask
		if (imd_flags & iV_IMD_TCMASK)
		{
			int texpage_mask;

			//pie_MakeTexPageTCMaskName(texfile);
			//sstrcat(texfile, ".png");
			//texpage_mask = iV_GetTexture(texfile);

			//ASSERT_OR_RETURN(shape, texpage_mask >= 0, "%s could not load tcmask %s", filename.toUtf8().constData(), texfile);

			// Propagate settings through levels
			for (iIMDShape *psShape = shape; psShape != NULL; psShape = psShape->next)
			{
				psShape->tcmaskpage = texpage_mask;
			}
		}
	}*/

	// copy over model-wide animation information, stored only in the first level
	std::copy(objanimpie.begin(), objanimpie.end(), shape->objanimpie);
	return shape;
}
