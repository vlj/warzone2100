#include "gl_impl.h"
#include <physfs.h>
#include "lib/framework/debug.h"
#include "..\piestate.h"
#include "..\pieclip.h"


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

template<typename... T>
static void getLocs(gfx_api::program& p, const T&... locations)
{
	auto& prog = dynamic_cast<gfx_api::gl_api::gl_program&>(p);
	glUseProgram(prog.program);

	// Attributes
	prog.attributes = { glGetAttribLocation(prog.program, locations)... };

	// Uniforms, these never change.
	GLint locTex0 = glGetUniformLocation(prog.program, "Texture");
	GLint locTex1 = glGetUniformLocation(prog.program, "TextureTcmask");
	GLint locTex2 = glGetUniformLocation(prog.program, "TextureNormal");
	GLint locTex3 = glGetUniformLocation(prog.program, "TextureSpecular");
	glUniform1i(locTex0, 0);
	glUniform1i(locTex1, 1);
	glUniform1i(locTex2, 2);
	glUniform1i(locTex3, 3);
}

// Read/compile/link shaders
std::unique_ptr<gfx_api::gl_api::gl_program> pie_LoadShader(const char *programName, const char *vertexPath, const char *fragmentPath,
	const std::vector<std::string> &uniformNames)
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
	std::transform(uniformNames.begin(), uniformNames.end(),
		std::back_inserter(program->locations),
		[&](const std::string name) { return glGetUniformLocation(program->program, name.data()); });

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
		pie_LoadShader("Component program", "shaders/tcmask.vert", "shaders/tcmask.frag",
		{ "colour", "teamcolour", "stretch", "tcmask", "fogEnabled", "normalmap", "specularmap", "ecmEffect", "alphaTest", "graphicsCycle" })
	);
	getLocs(*results.back());

	// TCMask shader for buttons with flat lighting
	debug(LOG_3D, "Loading shader: SHADER_BUTTON");
	results.push_back(
		pie_LoadShader("Button program", "shaders/button.vert", "shaders/button.frag",
		{ "colour", "teamcolour", "stretch", "tcmask", "fogEnabled", "normalmap", "specularmap", "ecmEffect", "alphaTest", "graphicsCycle" })
	);

	// Plain shader for no lighting
	debug(LOG_3D, "Loading shader: SHADER_NOLIGHT");
	results.push_back(
		pie_LoadShader("Plain program", "shaders/nolight.vert", "shaders/nolight.frag",
		{ "colour", "teamcolour", "stretch", "tcmask", "fogEnabled", "normalmap", "specularmap", "ecmEffect", "alphaTest", "graphicsCycle" })
	);

	debug(LOG_3D, "Loading shader: TERRAIN");
	results.push_back(
		pie_LoadShader("terrain program", "shaders/terrain_water.vert", "shaders/terrain.frag",
		{ "ModelViewProjectionMatrix", "paramx1", "paramy1", "paramx2", "paramy2", "tex", "lightmap_tex", "textureMatrix2",
			"fogEnabled", "fogEnd", "fogStart", "fogColor" })
	);

	debug(LOG_3D, "Loading shader: TERRAIN_DEPTH");
	results.push_back(
		pie_LoadShader("terrain_depth program", "shaders/terrain_water.vert", "shaders/terraindepth.frag",
		{ "ModelViewProjectionMatrix", "paramx2", "paramy2", "lightmap_tex" })
	);
	getLocs(*results.back(), "vertex");
	
	debug(LOG_3D, "Loading shader: DECALS");
	results.push_back(pie_LoadShader("decals program", "shaders/decals.vert", "shaders/decals.frag",
		{ "ModelViewProjectionMatrix", "paramxlight", "paramylight", "tex", "lightmap_tex", "lightTextureMatrix",
			"fogEnabled", "fogEnd", "fogStart", "fogColor" })
	);

	debug(LOG_3D, "Loading shader: WATER");
	results.push_back(pie_LoadShader("water program", "shaders/terrain_water.vert", "shaders/water.frag",
		{ "ModelViewProjectionMatrix", "paramx1", "paramy1", "paramx2", "paramy2", "tex1", "tex2", "textureMatrix1",
			"fogEnabled", "fogEnd", "fogStart" })
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
