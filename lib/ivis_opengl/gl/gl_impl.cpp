#include "gl_impl.h"
#include <physfs.h>
#include "lib/framework/debug.h"
#include "..\piestate.h"
#include "..\pieclip.h"
#include "../uniform_buffer_types.h"


// Read shader into text buffer
static char *readShaderBuf(const char *name)
{
	PHYSFS_file	*fp;
	int	filesize;
	char *buffer;

	fp = PHYSFS_openRead(name);
	debug(LOG_3D, "Reading...[directory: %s] %s", PHYSFS_getRealDir(name), name);
	ASSERT_OR_RETURN(0, fp != NULL, "Could not open %s", name);
	filesize = PHYSFS_fileLength(fp);
	buffer = (char *)malloc(filesize + 1);
	if (buffer)
	{
		PHYSFS_read(fp, buffer, 1, filesize);
		buffer[filesize] = '\0';
	}
	PHYSFS_close(fp);

	return buffer;
}

// Retrieve shader compilation errors
static void printShaderInfoLog(code_part part, GLuint shader)
{
	GLint infologLen = 0;

	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infologLen);
	if (infologLen > 0)
	{
		GLint charsWritten = 0;
		GLchar *infoLog = (GLchar *)malloc(infologLen);

		glGetShaderInfoLog(shader, infologLen, &charsWritten, infoLog);
		debug(part, "Shader info log: %s", infoLog);
		free(infoLog);
	}
}

// Retrieve shader linkage errors
static void printProgramInfoLog(code_part part, GLuint program)
{
	GLint infologLen = 0;

	glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infologLen);
	if (infologLen > 0)
	{
		GLint charsWritten = 0;
		GLchar *infoLog = (GLchar *)malloc(infologLen);

		glGetProgramInfoLog(program, infologLen, &charsWritten, infoLog);
		debug(part, "Program info log: %s", infoLog);
		free(infoLog);
	}
}

template<typename T>
struct uniform_buffer_traits
{
};

/**
* setUniforms is an overloaded wrapper around glUniform* functions
* accepting glm structures.
*/

inline void setUniforms(GLint location, const ::glm::vec4 &v)
{
	glUniform4f(location, v.x, v.y, v.z, v.w);
}

inline void setUniforms(GLint location, const ::glm::mat4 &m)
{
	glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(m));
}

inline void setUniforms(GLint location, const Vector2i &v)
{
	glUniform2i(location, v.x, v.y);
}

inline void setUniforms(GLint location, const Vector2f &v)
{
	glUniform2f(location, v.x, v.y);
}

inline void setUniforms(GLint location, const int32_t &v)
{
	glUniform1i(location, v);
}

inline void setUniforms(GLint location, const float &v)
{
	glUniform1f(location, v);
}

template<>
struct uniform_buffer_traits<ivis::TerrainDepthUniforms>
{
	static constexpr std::array<const char*, 3> uniform_names = { "ModelViewProjectionMatrix", "paramx2", "paramy2" };

	static auto get_bind_uniforms()
	{
		return [](const auto& v, const ivis::TerrainDepthUniforms& data) {
			setUniforms(v[0], data.ModelViewProjectionMatrix);
			setUniforms(v[1], data.paramsXLight);
			setUniforms(v[2], data.paramsYLight);
		};
	}
};

template<>
struct uniform_buffer_traits<ivis::TerrainLayers>
{
	static constexpr std::array<const char*, 10> uniform_names = { "ModelViewProjectionMatrix", "paramx1", "paramy1", "paramx2", "paramy2", "textureMatrix2",
		"fogEnabled", "fogEnd", "fogStart", "fogColor" };

	static auto get_bind_uniforms()
	{
		return [](const auto& v, const ivis::TerrainLayers& data) {
			setUniforms(v[0], data.ModelViewProjection);
			setUniforms(v[1], data.paramsX);
			setUniforms(v[2], data.paramsY);
		};
	}
};

template<>
struct uniform_buffer_traits<ivis::TerrainDecals>
{
	static constexpr std::array<const char*, 8> uniform_names = { "ModelViewProjectionMatrix", "paramxlight", "paramylight", "lightTextureMatrix",
		"fogEnabled", "fogEnd", "fogStart", "fogColor" };

	static auto get_bind_uniforms()
	{
		return [](const auto& v, const ivis::TerrainDecals& data) {
			setUniforms(v[0], data.ModelViewProjection);
			setUniforms(v[1], data.paramsXLight);
			setUniforms(v[2], data.paramsYLight);
		};
	}
};


template<typename... T>
static void getLocs(gfx_api::program& p, const T&... locations)
{
	auto& prog = dynamic_cast<gfx_api::gl_api::gl_program&>(p);
	glUseProgram(prog.program);

	// Attributes
	prog.attributes = { glGetAttribLocation(prog.program, locations)... };

	// Uniforms, these never change.
/*	GLint locTex0 = glGetUniformLocation(prog.program, "Texture");
	GLint locTex1 = glGetUniformLocation(prog.program, "TextureTcmask");
	GLint locTex2 = glGetUniformLocation(prog.program, "TextureNormal");
	GLint locTex3 = glGetUniformLocation(prog.program, "TextureSpecular");*/
	GLint locTex0 = glGetUniformLocation(prog.program, "tex");
	GLint locTex1 = glGetUniformLocation(prog.program, "lightmap_tex");
	glUniform1i(locTex0, 0);
	glUniform1i(locTex1, 1);
}

// Read/compile/link shaders
template<typename UniformBufferType>
static std::unique_ptr<gfx_api::gl_api::gl_program>
pie_LoadShader(const char *programName, const char *vertexPath, const char *fragmentPath)
{
	gfx_api::gl_api::gl_program* program = new gfx_api::gl_api::gl_program();
	GLint status;
	bool success = true; // Assume overall success
	char *buffer[2];

	program->program = glCreateProgram();
	glBindAttribLocation(program->program, 0, "vertex");
	glBindAttribLocation(program->program, 1, "vertexTexCoord");
	glBindAttribLocation(program->program, 2, "vertexColor");
	//ASSERT_OR_RETURN(SHADER_NONE, program->program, "Could not create shader program!");

	*buffer = (char *)"";

	if (vertexPath)
	{
		success = false; // Assume failure before reading shader file

		if ((*(buffer + 1) = readShaderBuf(vertexPath)))
		{
			GLuint shader = glCreateShader(GL_VERTEX_SHADER);

			glShaderSource(shader, 2, (const char **)buffer, NULL);
			glCompileShader(shader);

			// Check for compilation errors
			glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
			if (!status)
			{
				debug(LOG_ERROR, "Vertex shader compilation has failed [%s]", vertexPath);
				printShaderInfoLog(LOG_ERROR, shader);
			}
			else
			{
				printShaderInfoLog(LOG_3D, shader);
				glAttachShader(program->program, shader);
				success = true;
			}
			if (GLEW_VERSION_4_3 || GLEW_KHR_debug)
			{
				glObjectLabel(GL_SHADER, shader, -1, vertexPath);
			}
			free(*(buffer + 1));
		}
	}

	if (success && fragmentPath)
	{
		success = false; // Assume failure before reading shader file

		if ((*(buffer + 1) = readShaderBuf(fragmentPath)))
		{
			GLuint shader = glCreateShader(GL_FRAGMENT_SHADER);

			glShaderSource(shader, 2, (const char **)buffer, NULL);
			glCompileShader(shader);

			// Check for compilation errors
			glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
			if (!status)
			{
				debug(LOG_ERROR, "Fragment shader compilation has failed [%s]", fragmentPath);
				printShaderInfoLog(LOG_ERROR, shader);
			}
			else
			{
				printShaderInfoLog(LOG_3D, shader);
				glAttachShader(program->program, shader);
				success = true;
			}
			if (GLEW_VERSION_4_3 || GLEW_KHR_debug)
			{
				glObjectLabel(GL_SHADER, shader, -1, fragmentPath);
			}
			free(*(buffer + 1));
		}
	}
	if (success)
	{
		glLinkProgram(program->program);

		// Check for linkage errors
		glGetProgramiv(program->program, GL_LINK_STATUS, &status);
		if (!status)
		{
			debug(LOG_ERROR, "Shader program linkage has failed [%s, %s]", vertexPath, fragmentPath);
			printProgramInfoLog(LOG_ERROR, program->program);
			success = false;
		}
		else
		{
			printProgramInfoLog(LOG_3D, program->program);
		}
		if (GLEW_VERSION_4_3 || GLEW_KHR_debug)
		{
			glObjectLabel(GL_PROGRAM, program->program, -1, programName);
		}
	}
	std::transform(
		uniform_buffer_traits<UniformBufferType>::uniform_names.begin(),
		uniform_buffer_traits<UniformBufferType>::uniform_names.end(),
		std::back_inserter(program->locations),
		[&](const std::string name) { return glGetUniformLocation(program->program, name.data()); }
	);
	program->bind_uniforms = [program](const void* data) { uniform_buffer_traits<UniformBufferType>::get_bind_uniforms()(program->locations, *static_cast<const UniformBufferType*>(data)); };

	glUseProgram(0);

	return std::unique_ptr<gfx_api::gl_api::gl_program>(program);
}

/*pie_internal::SHADER_PROGRAM &pie_ActivateShaderDeprecated(SHADER_MODE shaderMode, const iIMDShape *shape, PIELIGHT teamcolour, PIELIGHT colour)
{
	int maskpage = shape->tcmaskpage;
	int normalpage = shape->normalpage;
	int specularpage = shape->specularpage;
	pie_internal::SHADER_PROGRAM &program = pie_internal::shaderProgram[shaderMode];

	if (shaderMode != pie_internal::currentShaderMode)
	{
		glUseProgram(program.program);
		pie_internal::SHADER_PROGRAM &program = pie_internal::shaderProgram[shaderMode];

		// These do not change during our drawing pass
		glUniform1i(program.locations[4], rendStates.fog);
		glUniform1f(program.locations[9], timeState);

		pie_internal::currentShaderMode = shaderMode;
	}
	glUniform4fv(program.locations[0], 1, &pal_PIELIGHTtoVec4(colour)[0]);
	pie_SetTexturePage(shape->texpage);

	glUniform4fv(program.locations[1], 1, &pal_PIELIGHTtoVec4(teamcolour)[0]);
	glUniform1i(program.locations[3], maskpage != iV_TEX_INVALID);

	glUniform1i(program.locations[8], 0);

	if (program.locations[2] >= 0)
	{
		glUniform1f(program.locations[2], shaderStretch);
	}
	if (program.locations[5] >= 0)
	{
		glUniform1i(program.locations[5], normalpage != iV_TEX_INVALID);
	}
	if (program.locations[6] >= 0)
	{
		glUniform1i(program.locations[6], specularpage != iV_TEX_INVALID);
	}
	if (program.locations[7] >= 0)
	{
		glUniform1i(program.locations[7], ecmState);
	}

	if (maskpage != iV_TEX_INVALID)
	{
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, pie_Texture(maskpage));
	}
	if (normalpage != iV_TEX_INVALID)
	{
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, pie_Texture(normalpage));
	}
	if (specularpage != iV_TEX_INVALID)
	{
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, pie_Texture(specularpage));
	}
	glActiveTexture(GL_TEXTURE0);

	return program;
}*/

std::vector<std::unique_ptr<gfx_api::program>> pie_LoadShaders()
{
	std::vector<std::unique_ptr<gfx_api::program>> results(1);

	// Load some basic shaders

	// TCMask shader for map-placed models with advanced lighting
	debug(LOG_3D, "Loading shader: SHADER_COMPONENT");
	results.push_back(
		pie_LoadShader<ivis::TerrainDepthUniforms>("Component program", "shaders/tcmask.vert", "shaders/tcmask.frag")
	);
	getLocs(*results.back());

	// TCMask shader for buttons with flat lighting
	debug(LOG_3D, "Loading shader: SHADER_BUTTON");
	results.push_back(
		pie_LoadShader<ivis::TerrainDepthUniforms>("Button program", "shaders/button.vert", "shaders/button.frag")
	);

	// Plain shader for no lighting
	debug(LOG_3D, "Loading shader: SHADER_NOLIGHT");
	results.push_back(
		pie_LoadShader<ivis::TerrainDepthUniforms>("Plain program", "shaders/nolight.vert", "shaders/nolight.frag")
	);

	debug(LOG_3D, "Loading shader: TERRAIN");
	results.push_back(
		pie_LoadShader<ivis::TerrainLayers>("terrain program", "shaders/terrain_water.vert", "shaders/terrain.frag")
	);
	getLocs(*results.back(), "vertex", "vertexColor");
	dynamic_cast<gfx_api::gl_api::gl_program&>(*results.back()).bind_textures = [](const auto& textures) {
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, dynamic_cast<const gfx_api::gl_api::gl_texture*>(textures[0])->object);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	};

	debug(LOG_3D, "Loading shader: TERRAIN_DEPTH");
	results.push_back(
		pie_LoadShader<ivis::TerrainDepthUniforms>("terrain_depth program", "shaders/terrain_water.vert", "shaders/terraindepth.frag")
	);
	getLocs(*results.back(), "vertex");
	
	debug(LOG_3D, "Loading shader: DECALS");
	results.push_back(
		pie_LoadShader<ivis::TerrainDecals>("decals program", "shaders/decals.vert", "shaders/decals.frag")
	);
	dynamic_cast<gfx_api::gl_api::gl_program&>(*results.back()).bind_textures = [](const auto& textures) {
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, dynamic_cast<const gfx_api::gl_api::gl_texture*>(textures[0])->object);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	};

	debug(LOG_3D, "Loading shader: WATER");
	results.push_back(
		pie_LoadShader<ivis::TerrainDepthUniforms>("water program", "shaders/terrain_water.vert", "shaders/water.frag")
	);

	return results;
}

auto get_internal_format_type(gfx_api::format f)
{
	switch (f)
	{
	case gfx_api::format::rgba: return std::make_tuple(GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE);
	case gfx_api::format::rgb: return std::make_tuple(GL_RGBA, GL_RGB, GL_UNSIGNED_BYTE);
	}
	throw;
}

gfx_api::gl_api::gl_context::gl_context()
{
	// FIXME: aspect ratio
	glViewport(0, 0, pie_GetVideoBufferWidth(), pie_GetVideoBufferHeight());
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, pie_GetVideoBufferWidth(), pie_GetVideoBufferHeight(), 0, 1, -1);

	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glCullFace(GL_FRONT);
	glEnable(GL_CULL_FACE);

	glEnable(GL_TEXTURE_2D);
}

std::unique_ptr<gfx_api::texture> gfx_api::gl_api::gl_context::create_texture(format f, size_t width, size_t height, size_t levels)
{
	auto* tex = new gl_texture();
	glGenTextures(1, &tex->object);
	glBindTexture(GL_TEXTURE_2D, tex->object);
	const auto& internal_format_type = get_internal_format_type(f);
	auto mipmap_width = width;
	auto mipmap_height = height;
	for (unsigned i = 0; i < levels; i++)
	{
		glTexImage2D(GL_TEXTURE_2D, i, std::get<0>(internal_format_type), mipmap_width, mipmap_height, 0, std::get<1>(internal_format_type), std::get<2>(internal_format_type), nullptr);
		mipmap_width >>= 1;
		mipmap_height >>= 1;
	}
	return std::unique_ptr<texture>(tex);
}

gfx_api::gl_api::gl_texture::~gl_texture()
{
	glDeleteTextures(1, &object);
}

std::unique_ptr<gfx_api::buffer> gfx_api::gl_api::gl_context::create_buffer(size_t size, const bool& is_index)
{
	auto* buf = new gl_buffer();
	glGenBuffers(1, &buf->object);
	buf->target = is_index ? GL_ELEMENT_ARRAY_BUFFER : GL_ARRAY_BUFFER;
	glBindBuffer(buf->target, buf->object);
	glBufferData(buf->target, size, nullptr, GL_STATIC_DRAW);
	return std::unique_ptr<gfx_api::buffer>(buf);
}

gfx_api::gl_api::gl_buffer::~gl_buffer()
{
	glDeleteBuffers(1, &object);
}

std::vector<std::unique_ptr<gfx_api::program>> gfx_api::gl_api::gl_context::build_programs()
{
	return pie_LoadShaders();
}

std::unique_ptr<gfx_api::uniforms> gfx_api::gl_api::gl_context::get_uniform_storage(const size_t & capacity)
{

	return std::unique_ptr<uniforms>(new gl_uniforms(capacity));
}

gfx_api::gl_api::gl_program::gl_program()
{
	glGenVertexArrays(1, &vao);
}

gfx_api::gl_api::gl_program::~gl_program()
{
	glDeleteVertexArrays(1, &vao);
	glDeleteShader(program);
}

auto get_type_info(const gfx_api::buffer_layout& layout)
{
	switch (layout.type)
	{
	case gfx_api::buffer_type::vec1: return std::make_tuple(1, GL_FLOAT, false);
	case gfx_api::buffer_type::vec2: return std::make_tuple(2, GL_FLOAT, false);
	case gfx_api::buffer_type::vec3: return std::make_tuple(3, GL_FLOAT, false);
	case gfx_api::buffer_type::vec4: return std::make_tuple(4, GL_FLOAT, false);
	case gfx_api::buffer_type::ub4: return std::make_tuple(4, GL_UNSIGNED_BYTE, true);
	}
	throw;
}


void gfx_api::gl_api::gl_program::set_vertex_buffers(const std::vector<gfx_api::buffer_layout>& layout,
	const std::vector<std::tuple<const buffer&, size_t>>& buffers_and_offset)
{
	glBindVertexArray(vao);
	auto&& attrib = 0;
	for (const auto attribute_index : attributes)
	{
		const auto& buffer_and_offset = buffers_and_offset[attrib];
		const auto& current_layout = layout[attrib];
		const auto& infos = get_type_info(current_layout);
		attrib++;
		const auto& as_gl_buffer = dynamic_cast<const gl_buffer&>(std::get<0>(buffer_and_offset));
		glBindBuffer(GL_ARRAY_BUFFER, as_gl_buffer.object);
		glEnableVertexAttribArray(attribute_index);
		glVertexAttribPointer(attribute_index, std::get<0>(infos), std::get<1>(infos), std::get<2>(infos), current_layout.stride,
			(char*)std::get<1>(buffer_and_offset));
	}

}

void gfx_api::gl_api::gl_program::use()
{
	glUseProgram(program);
}

void gfx_api::gl_api::gl_program::set_index_buffer(const buffer & b)
{
	glBindVertexArray(vao);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, dynamic_cast<const gl_buffer&>(b).object);
}

void gfx_api::gl_api::gl_program::set_uniforms(const uniforms & uniforms)
{
	bind_uniforms(dynamic_cast<const gl_uniforms&>(uniforms).memory);
}

void gfx_api::gl_api::gl_program::set_textures(const std::vector<texture*>& textures)
{
	bind_textures(textures);
}

void gfx_api::gl_api::gl_buffer::upload(size_t offset, size_t s, const void * ptr)
{
	glBindBuffer(target, object);
	glBufferSubData(target, offset, s, ptr);
}

void gfx_api::gl_api::gl_texture::upload_texture(const format& f, size_t width, size_t height, const void * ptr)
{
	glBindTexture(GL_TEXTURE_2D, object);
	const auto& gl_format = get_internal_format_type(f);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, std::get<1>(gl_format), std::get<2>(gl_format), ptr);
}

gfx_api::gl_api::gl_uniforms::gl_uniforms(const size_t size)
{
	memory = malloc(size);
}

gfx_api::gl_api::gl_uniforms::~gl_uniforms()
{
	free(memory);
}

void gfx_api::gl_api::gl_uniforms::unmap()
{
}

void * gfx_api::gl_api::gl_uniforms::map_impl() const
{
	return memory;
}
