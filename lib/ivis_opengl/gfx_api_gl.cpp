#include "gfx_api.h"

#include <GL/glew.h>
#include <algorithm>
#include <cmath>
#include "lib/framework/frame.h"
#include <glm/gtc/type_ptr.hpp>
#include <physfs.h>
#include <map>
#include <functional>

static GLenum to_gl(const gfx_api::pixel_format& format)
{
	switch (format)
	{
	case gfx_api::pixel_format::rgba:
		return GL_RGBA;
	case gfx_api::pixel_format::rgb:
		return GL_RGB;
	case gfx_api::pixel_format::compressed_rgb:
		return GL_RGB_S3TC;
	case gfx_api::pixel_format::compressed_rgba:
		return GL_RGBA_S3TC;
	default:
		debug(LOG_FATAL, "Unrecognised pixel format");
	}
	return GL_INVALID_ENUM;
}

static GLenum to_gl(const gfx_api::buffer::usage& usage)
{
	switch (usage)
	{
	case gfx_api::buffer::usage::index_buffer:
		return GL_ELEMENT_ARRAY_BUFFER;
	case gfx_api::buffer::usage::vertex_buffer:
		return GL_ARRAY_BUFFER;
	default:
		debug(LOG_FATAL, "Unrecognised buffer usage");
	}
	return GL_INVALID_ENUM;
}

static GLenum to_gl(const gfx_api::primitive_type& primitive)
{
	switch (primitive)
	{
	case gfx_api::primitive_type::lines:
		return GL_LINES;
	case gfx_api::primitive_type::triangles:
		return GL_TRIANGLES;
	case gfx_api::primitive_type::triangle_fan:
		return GL_TRIANGLE_FAN;
	case gfx_api::primitive_type::triangle_strip:
		return GL_TRIANGLE_STRIP;
	default:
		debug(LOG_FATAL, "Unrecognised primitive type");
	}
	return GL_INVALID_ENUM;
}

static GLenum to_gl(const gfx_api::index_type& index)
{
	switch (index)
	{
	case gfx_api::index_type::u16:
		return GL_UNSIGNED_SHORT;
	case gfx_api::index_type::u32:
		return GL_UNSIGNED_INT;
	default:
		debug(LOG_FATAL, "Unrecognised index type");
	}
	return GL_INVALID_ENUM;
}

struct gl_texture : public gfx_api::texture
{
private:
	friend struct gl_context;
	GLuint _id;

	gl_texture()
	{
		glGenTextures(1, &_id);
	}

	~gl_texture()
	{
		glDeleteTextures(1, &_id);
	}
public:
	virtual void bind() override
	{
		glBindTexture(GL_TEXTURE_2D, _id);
	}

	virtual void upload(const size_t& mip_level, const size_t& offset_x, const size_t& offset_y, const size_t & width, const size_t & height, const gfx_api::pixel_format & buffer_format, const void * data) override
	{
		bind();
		glTexSubImage2D(GL_TEXTURE_2D, mip_level, offset_x, offset_y, width, height, to_gl(buffer_format), GL_UNSIGNED_BYTE, data);
	}

	virtual unsigned id() override
	{
		return _id;
	}

	virtual void generate_mip_levels() override
	{
		glGenerateMipmap(GL_TEXTURE_2D);
	}

};

struct gl_buffer : public gfx_api::buffer
{
	gfx_api::buffer::usage usage;
	GLuint buffer;

	virtual ~gl_buffer() override
	{
		glDeleteBuffers(1, &buffer);
	}

	gl_buffer(const gfx_api::buffer::usage& usage, const size_t& size)
		: usage(usage)
	{
		glGenBuffers(1, &buffer);
		glBindBuffer(to_gl(usage), buffer);
		glBufferData(to_gl(usage), size, nullptr, GL_STATIC_DRAW);
	}

	void bind() override
	{
		glBindBuffer(to_gl(usage), buffer);
	}


	virtual void upload(const size_t & start, const size_t & size, const void * data) override
	{
		glBindBuffer(to_gl(usage), buffer);
		glBufferSubData(to_gl(usage), start, size, data);
	}

};

struct program_data
{
	std::string friendly_name;
	std::string vertex_file;
	std::string fragment_file;
	std::vector<std::string> uniform_names;
};

static const std::map<SHADER_MODE, program_data> shader_to_file_table =
{
	std::make_pair(SHADER_COMPONENT, program_data{ "Component program", "shaders/tcmask.vert", "shaders/tcmask.frag",
		{ "colour", "teamcolour", "stretch", "tcmask", "fogEnabled", "normalmap", "specularmap", "ecmEffect", "alphaTest", "graphicsCycle",
		"ModelViewMatrix", "ModelViewProjectionMatrix", "NormalMatrix", "lightPosition", "sceneColor", "ambient", "diffuse", "specular",
		"fogEnd", "fogStart", "fogColor" } }),
	std::make_pair(SHADER_BUTTON, program_data{ "Button program", "shaders/button.vert", "shaders/button.frag",
		{ "colour", "teamcolour", "stretch", "tcmask", "fogEnabled", "normalmap", "specularmap", "ecmEffect", "alphaTest", "graphicsCycle",
		"ModelViewMatrix", "ModelViewProjectionMatrix", "NormalMatrix", "lightPosition", "sceneColor", "ambient", "diffuse", "specular",
		"fogEnd", "fogStart", "fogColor" } }),
	std::make_pair(SHADER_NOLIGHT, program_data{ "Plain program", "shaders/nolight.vert", "shaders/nolight.frag",
		{ "colour", "teamcolour", "stretch", "tcmask", "fogEnabled", "normalmap", "specularmap", "ecmEffect", "alphaTest", "graphicsCycle",
		"ModelViewMatrix", "ModelViewProjectionMatrix", "NormalMatrix", "lightPosition", "sceneColor", "ambient", "diffuse", "specular",
		"fogEnd", "fogStart", "fogColor" } }),
	std::make_pair(SHADER_TERRAIN, program_data{ "terrain program", "shaders/terrain_water.vert", "shaders/terrain.frag",
		{ "ModelViewProjectionMatrix", "paramx1", "paramy1", "paramx2", "paramy2", "tex", "lightmap_tex", "textureMatrix1", "textureMatrix2",
		"fogEnabled", "fogEnd", "fogStart", "fogColor" } }),
	std::make_pair(SHADER_TERRAIN_DEPTH, program_data{ "terrain_depth program", "shaders/terrain_water.vert", "shaders/terraindepth.frag",
		{ "ModelViewProjectionMatrix", "paramx2", "paramy2", "lightmap_tex", "textureMatrix1", "textureMatrix2" } }),
	std::make_pair(SHADER_DECALS, program_data{ "decals program", "shaders/decals.vert", "shaders/decals.frag",
		{ "ModelViewProjectionMatrix", "paramxlight", "paramylight", "tex", "lightmap_tex", "lightTextureMatrix",
		"fogEnabled", "fogEnd", "fogStart", "fogColor" } }),
	std::make_pair(SHADER_WATER, program_data{ "water program", "shaders/terrain_water.vert", "shaders/water.frag",
		{ "ModelViewProjectionMatrix", "paramx1", "paramy1", "paramx2", "paramy2", "tex1", "tex2", "textureMatrix1", "textureMatrix2",
		"fogEnabled", "fogEnd", "fogStart" } }),
	std::make_pair(SHADER_RECT, program_data{ "Rect program", "shaders/rect.vert", "shaders/rect.frag",
		{ "transformationMatrix", "color" } }),
	std::make_pair(SHADER_TEXRECT, program_data{ "Textured rect program", "shaders/rect.vert", "shaders/texturedrect.frag",
		{ "transformationMatrix", "tuv_offset", "tuv_scale", "color", "texture" } }),
	std::make_pair(SHADER_GFX_COLOUR, program_data{ "gfx_color program", "shaders/gfx.vert", "shaders/gfx.frag",
		{ "posMatrix" } }),
	std::make_pair(SHADER_GFX_TEXT, program_data{ "gfx_text program", "shaders/gfx.vert", "shaders/texturedrect.frag",
		{ "posMatrix", "color", "texture" } }),
	std::make_pair(SHADER_GENERIC_COLOR, program_data{ "generic color program", "shaders/generic.vert", "shaders/rect.frag",{ "ModelViewProjectionMatrix", "color" } }),
	std::make_pair(SHADER_LINE, program_data{ "line program", "shaders/line.vert", "shaders/rect.frag",{ "from", "to", "color", "ModelViewProjectionMatrix" } }),
	std::make_pair(SHADER_TEXT, program_data{ "Text program", "shaders/rect.vert", "shaders/text.frag",
		{ "transformationMatrix", "tuv_offset", "tuv_scale", "color", "texture" } })
};

struct gl_pipeline_state_object : public gfx_api::pipeline_state_object
{
	gfx_api::state_description desc;
	GLuint program;
	std::vector<GLint> locations;

	std::function<void(const void*)> uniform_bind_function;

	template<SHADER_MODE shader>
	auto uniform_binding_entry()
	{
		return std::make_pair(shader, [this](const void* buffer) { this->set_constants(*reinterpret_cast<const gfx_api::constant_buffer_type<shader>*>(buffer)); });
	}

	gl_pipeline_state_object(const gfx_api::state_description& _desc, const SHADER_MODE& shader) :
		desc(_desc)
	{
		build_program(
			shader_to_file_table.at(shader).friendly_name,
			shader_to_file_table.at(shader).vertex_file,
			shader_to_file_table.at(shader).fragment_file,
			shader_to_file_table.at(shader).uniform_names);

		const std::map < SHADER_MODE, std::function<void(const void*)>> uniforms_bind_table =
		{
			uniform_binding_entry<SHADER_COMPONENT>(),
			uniform_binding_entry<SHADER_BUTTON>(),
			uniform_binding_entry<SHADER_NOLIGHT>(),
			uniform_binding_entry<SHADER_TERRAIN>(),
			uniform_binding_entry<SHADER_TERRAIN_DEPTH>(),
			uniform_binding_entry<SHADER_DECALS>(),
			uniform_binding_entry<SHADER_WATER>(),
			uniform_binding_entry<SHADER_RECT>(),
			uniform_binding_entry<SHADER_TEXRECT>(),
			uniform_binding_entry<SHADER_GFX_COLOUR>(),
			uniform_binding_entry<SHADER_GFX_TEXT>(),
			uniform_binding_entry<SHADER_GENERIC_COLOR>(),
			uniform_binding_entry<SHADER_LINE>(),
			uniform_binding_entry<SHADER_TEXT>()
		};

		uniform_bind_function = uniforms_bind_table.at(shader);
	}

	void set_constants(const void* buffer)
	{
		uniform_bind_function(buffer);
	}


	void bind()
	{
		glUseProgram(program);
		switch (desc.blend_state)
		{
		case REND_OPAQUE:
			glDisable(GL_BLEND);
			break;

		case REND_ALPHA:
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			break;

		case REND_ADDITIVE:
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE);
			break;

		case REND_MULTIPLICATIVE:
			glEnable(GL_BLEND);
			glBlendFunc(GL_ZERO, GL_SRC_COLOR);
			break;

		case REND_PREMULTIPLIED:
			glEnable(GL_BLEND);
			glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
			break;

		case REND_TEXT:
			glEnable(GL_BLEND);
			glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA /* Should be GL_ONE_MINUS_SRC1_COLOR, if supported. Also, gl_FragData[1] then needs to be set in text.frag. */);
			break;
		}

		switch (desc.depth_mode)
		{
		case DEPTH_CMP_LEQ_WRT_OFF:
			glEnable(GL_DEPTH_TEST);
			glDepthFunc(GL_LEQUAL);
			glDepthMask(GL_FALSE);
			break;
		case DEPTH_CMP_LEQ_WRT_ON:
			glEnable(GL_DEPTH_TEST);
			glDepthFunc(GL_LEQUAL);
			glDepthMask(GL_TRUE);
			break;
		case DEPTH_CMP_ALWAYS_WRT_ON:
			glDisable(GL_DEPTH_TEST);
			glDepthMask(GL_TRUE);
			break;

		case DEPTH_CMP_ALWAYS_WRT_OFF:
			glDisable(GL_DEPTH_TEST);
			glDepthMask(GL_FALSE);
			break;
		}

		if (desc.output_mask == 0)
			glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
		else
			glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

		if (desc.offset)
			glEnable(GL_POLYGON_OFFSET_FILL);
		else
			glDisable(GL_POLYGON_OFFSET_FILL);

		switch (desc.stencil)
		{
		case gfx_api::stencil_mode::stencil_shadow_quad:
			glEnable(GL_STENCIL_TEST);
			glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
			glStencilMask(~0);
			glStencilFunc(GL_LESS, 0, ~0);
			break;
		case gfx_api::stencil_mode::stencil_shadow_silhouette:
			glEnable(GL_STENCIL_TEST);
			if (GLEW_VERSION_2_0)
			{
				glStencilMask(~0);
				glStencilOpSeparate(GL_BACK, GL_KEEP, GL_KEEP, GL_INCR_WRAP);
				glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_KEEP, GL_DECR_WRAP);
				glStencilFunc(GL_ALWAYS, 0, ~0);
			}
			else if (GLEW_EXT_stencil_two_side)
			{
				glEnable(GL_STENCIL_TEST_TWO_SIDE_EXT);
				glStencilMask(~0);
				glActiveStencilFaceEXT(GL_BACK);
				glStencilOp(GL_KEEP, GL_KEEP, GL_DECR_WRAP);
				glStencilFunc(GL_ALWAYS, 0, ~0);
				glActiveStencilFaceEXT(GL_FRONT);
				glStencilOp(GL_KEEP, GL_KEEP, GL_INCR_WRAP);
				glStencilFunc(GL_ALWAYS, 0, ~0);
			}
			else if (GLEW_ATI_separate_stencil)
			{
				glStencilMask(~0);
				glStencilOpSeparateATI(GL_BACK, GL_KEEP, GL_KEEP, GL_INCR_WRAP);
				glStencilOpSeparateATI(GL_FRONT, GL_KEEP, GL_KEEP, GL_DECR_WRAP);
				glStencilFunc(GL_ALWAYS, 0, ~0);
			}

			break;
		case gfx_api::stencil_mode::stencil_disabled:
			glDisable(GL_STENCIL_TEST);
			glDisable(GL_STENCIL_TEST_TWO_SIDE_EXT);
			break;
		}

		switch (desc.cull)
		{
		case gfx_api::cull_mode::back:
			glEnable(GL_CULL_FACE);
			break;
		case gfx_api::cull_mode::none:
			glDisable(GL_CULL_FACE);
			break;
		}
	}

	private:

		// Read shader into text buffer
		static char *readShaderBuf(const std::string& name)
		{
			PHYSFS_file	*fp;
			int	filesize;
			char *buffer;

			fp = PHYSFS_openRead(name.c_str());
			debug(LOG_3D, "Reading...[directory: %s] %s", PHYSFS_getRealDir(name.c_str()), name.c_str());
			ASSERT_OR_RETURN(nullptr, fp != nullptr, "Could not open %s", name.c_str());
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

		void getLocs()
		{
			glUseProgram(program);

			// Uniforms, these never change.
			GLint locTex0 = glGetUniformLocation(program, "Texture");
			GLint locTex1 = glGetUniformLocation(program, "TextureTcmask");
			GLint locTex2 = glGetUniformLocation(program, "TextureNormal");
			GLint locTex3 = glGetUniformLocation(program, "TextureSpecular");
			glUniform1i(locTex0, 0);
			glUniform1i(locTex1, 1);
			glUniform1i(locTex2, 2);
			glUniform1i(locTex3, 3);
		}


		void build_program(const std::string& programName, const std::string& vertexPath, const std::string& fragmentPath,
			const std::vector<std::string> &uniformNames)
		{
			GLint status;
			bool success = true; // Assume overall success
			char *buffer[2];

			program = glCreateProgram();
			glBindAttribLocation(program, 0, "vertex");
			glBindAttribLocation(program, 1, "vertexTexCoord");
			glBindAttribLocation(program, 2, "vertexColor");
			glBindAttribLocation(program, 3, "vertexNormal");
			//ASSERT_OR_RETURN(SHADER_NONE, program, "Could not create shader program!");

			*buffer = (char *)"";

			{
				success = false; // Assume failure before reading shader file

				if ((*(buffer + 1) = readShaderBuf(vertexPath)))
				{
					GLuint shader = glCreateShader(GL_VERTEX_SHADER);

					glShaderSource(shader, 2, (const char **)buffer, nullptr);
					glCompileShader(shader);

					// Check for compilation errors
					glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
					if (!status)
					{
						debug(LOG_ERROR, "Vertex shader compilation has failed [%s]", vertexPath.c_str());
						printShaderInfoLog(LOG_ERROR, shader);
					}
					else
					{
						printShaderInfoLog(LOG_3D, shader);
						glAttachShader(program, shader);
						success = true;
					}
					if (GLEW_VERSION_4_3 || GLEW_KHR_debug)
					{
						glObjectLabel(GL_SHADER, shader, -1, vertexPath.c_str());
					}
					free(*(buffer + 1));
				}
			}


			{
				success = false; // Assume failure before reading shader file

				if ((*(buffer + 1) = readShaderBuf(fragmentPath)))
				{
					GLuint shader = glCreateShader(GL_FRAGMENT_SHADER);

					glShaderSource(shader, 2, (const char **)buffer, nullptr);
					glCompileShader(shader);

					// Check for compilation errors
					glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
					if (!status)
					{
						debug(LOG_ERROR, "Fragment shader compilation has failed [%s]", fragmentPath.c_str());
						printShaderInfoLog(LOG_ERROR, shader);
					}
					else
					{
						printShaderInfoLog(LOG_3D, shader);
						glAttachShader(program, shader);
						success = true;
					}
					if (GLEW_VERSION_4_3 || GLEW_KHR_debug)
					{
						glObjectLabel(GL_SHADER, shader, -1, fragmentPath.c_str());
					}
					free(*(buffer + 1));
				}
			}

			if (success)
			{
				glLinkProgram(program);

				// Check for linkage errors
				glGetProgramiv(program, GL_LINK_STATUS, &status);
				if (!status)
				{
					debug(LOG_ERROR, "Shader program linkage has failed [%s, %s]", vertexPath, fragmentPath.c_str());
					printProgramInfoLog(LOG_ERROR, program);
					success = false;
				}
				else
				{
					printProgramInfoLog(LOG_3D, program);
				}
				if (GLEW_VERSION_4_3 || GLEW_KHR_debug)
				{
					glObjectLabel(GL_PROGRAM, program, -1, programName.c_str());
				}
			}
			fetch_uniforms(uniformNames);
			getLocs();
		}

		void fetch_uniforms(const std::vector<std::string>& uniformNames)
		{
			std::transform(uniformNames.begin(), uniformNames.end(),
				std::back_inserter(locations),
				[&](const std::string& name) { return glGetUniformLocation(program, name.data()); });
		}

		/**
		* setUniforms is an overloaded wrapper around glUniform* functions
		* accepting glm structures.
		* Please do not use directly, use pie_ActivateShader below.
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


		// Wish there was static reflection in C++...
		template<typename T>
		void set_constants_for_component(const T& cbuf)
		{
			setUniforms(locations[0], cbuf.colour);
			setUniforms(locations[1], cbuf.teamcolour);
			setUniforms(locations[2], cbuf.shaderStretch);
			setUniforms(locations[3], cbuf.tcmask);
			setUniforms(locations[4], cbuf.fogEnabled);
			setUniforms(locations[5], cbuf.normalMap);
			setUniforms(locations[6], cbuf.specularMap);
			setUniforms(locations[7], cbuf.ecmState);
			setUniforms(locations[8], cbuf.alphaTest);
			setUniforms(locations[9], cbuf.timeState);
			setUniforms(locations[10], cbuf.ModelViewMatrix);
			setUniforms(locations[11], cbuf.ModelViewProjectionMatrix);
			setUniforms(locations[12], cbuf.NormalMatrix);
			setUniforms(locations[13], cbuf.sunPos);
			setUniforms(locations[14], cbuf.sceneColor);
			setUniforms(locations[15], cbuf.ambient);
			setUniforms(locations[16], cbuf.diffuse);
			setUniforms(locations[17], cbuf.specular);
			setUniforms(locations[18], cbuf.fogEnd);
			setUniforms(locations[19], cbuf.fogBegin);
			setUniforms(locations[20], cbuf.fogColour);
		}

		void set_constants(const gfx_api::constant_buffer_type<SHADER_BUTTON>& cbuf)
		{
			set_constants_for_component(cbuf);
		}

		void set_constants(const gfx_api::constant_buffer_type<SHADER_COMPONENT>& cbuf)
		{
			set_constants_for_component(cbuf);
		}

		void set_constants(const gfx_api::constant_buffer_type<SHADER_NOLIGHT>& cbuf)
		{
			set_constants_for_component(cbuf);
		}

		void set_constants(const gfx_api::constant_buffer_type<SHADER_TERRAIN>& cbuf)
		{
			setUniforms(locations[0], cbuf.transform_matrix);
			setUniforms(locations[1], cbuf.paramX);
			setUniforms(locations[2], cbuf.paramY);
			setUniforms(locations[3], cbuf.paramXLight);
			setUniforms(locations[4], cbuf.paramYLight);
			setUniforms(locations[5], cbuf.texture0);
			setUniforms(locations[6], cbuf.texture1);
			setUniforms(locations[7], cbuf.unused);
			setUniforms(locations[8], cbuf.texture_matrix);
			setUniforms(locations[9], cbuf.fog_enabled);
			setUniforms(locations[10], cbuf.fog_begin);
			setUniforms(locations[11], cbuf.fog_end);
			setUniforms(locations[12], cbuf.fog_colour);
		}

		void set_constants(const gfx_api::constant_buffer_type<SHADER_TERRAIN_DEPTH>& cbuf)
		{
			setUniforms(locations[0], cbuf.transform_matrix);
			setUniforms(locations[1], cbuf.param1);
			setUniforms(locations[2], cbuf.param2);
			setUniforms(locations[3], cbuf.texture);
			setUniforms(locations[4], cbuf.unused0);
			setUniforms(locations[5], cbuf.unused1);
		}

		void set_constants(const gfx_api::constant_buffer_type<SHADER_DECALS>& cbuf)
		{
			setUniforms(locations[0], cbuf.transform_matrix);
			setUniforms(locations[1], cbuf.param1);
			setUniforms(locations[2], cbuf.param2);
			setUniforms(locations[3], cbuf.texture0);
			setUniforms(locations[4], cbuf.texture1);
			setUniforms(locations[5], cbuf.texture_matrix);
			setUniforms(locations[6], cbuf.fog_enabled);
			setUniforms(locations[7], cbuf.fog_begin);
			setUniforms(locations[8], cbuf.fog_end);
			setUniforms(locations[9], cbuf.fog_colour);
		}

		void set_constants(const gfx_api::constant_buffer_type<SHADER_WATER>& cbuf)
		{
			setUniforms(locations[0], cbuf.transform_matrix);
			setUniforms(locations[1], cbuf.param1);
			setUniforms(locations[2], cbuf.param2);
			setUniforms(locations[3], cbuf.param3);
			setUniforms(locations[4], cbuf.param4);
			setUniforms(locations[5], cbuf.texture0);
			setUniforms(locations[6], cbuf.texture1);
			setUniforms(locations[7], cbuf.translation);
			setUniforms(locations[8], cbuf.texture_matrix);
			setUniforms(locations[9], cbuf.fog_enabled);
			setUniforms(locations[10], cbuf.fog_begin);
			setUniforms(locations[11], cbuf.fog_end);
		}

		void set_constants(const gfx_api::constant_buffer_type<SHADER_RECT>& cbuf)
		{
			setUniforms(locations[0], cbuf.transform_matrix);
			setUniforms(locations[1], cbuf.colour);
		}

		void set_constants(const gfx_api::constant_buffer_type<SHADER_TEXRECT>& cbuf)
		{
			setUniforms(locations[0], cbuf.transform_matrix);
			setUniforms(locations[1], cbuf.offset);
			setUniforms(locations[2], cbuf.size);
			setUniforms(locations[3], cbuf.color);
			setUniforms(locations[4], cbuf.texture);
		}

		void set_constants(const gfx_api::constant_buffer_type<SHADER_GFX_COLOUR>& cbuf)
		{
			setUniforms(locations[0], cbuf.mat);
		}

		void set_constants(const gfx_api::constant_buffer_type<SHADER_GFX_TEXT>& cbuf)
		{
			setUniforms(locations[0], cbuf.mat);
			setUniforms(locations[1], cbuf.vec);
			setUniforms(locations[2], cbuf.texture);
		}

		void set_constants(const gfx_api::constant_buffer_type<SHADER_GENERIC_COLOR>& cbuf)
		{
			setUniforms(locations[0], cbuf.transform_matrix);
			setUniforms(locations[1], cbuf.colour);
		}

		void set_constants(const gfx_api::constant_buffer_type<SHADER_LINE>& cbuf)
		{
			setUniforms(locations[0], cbuf.p0);
			setUniforms(locations[1], cbuf.p1);
			setUniforms(locations[2], cbuf.colour);
			setUniforms(locations[3], cbuf.mat);
		}

		void set_constants(const gfx_api::constant_buffer_type<SHADER_TEXT>& cbuf)
		{
			setUniforms(locations[0], cbuf.transform_matrix);
			setUniforms(locations[1], cbuf.offset);
			setUniforms(locations[2], cbuf.size);
			setUniforms(locations[3], cbuf.color);
			setUniforms(locations[4], cbuf.texture);
		}
};

size_t get_size(const gfx_api::vertex_attribute_type& type)
{
	switch (type)
	{
	case gfx_api::vertex_attribute_type::float2:
		return 2;
	case gfx_api::vertex_attribute_type::float3:
		return 3;
	case gfx_api::vertex_attribute_type::float4:
	case gfx_api::vertex_attribute_type::u8x4_norm:
		return 4;
	}
}

GLenum get_type(const gfx_api::vertex_attribute_type& type)
{
	switch (type)
	{
	case gfx_api::vertex_attribute_type::float2:
	case gfx_api::vertex_attribute_type::float3:
	case gfx_api::vertex_attribute_type::float4:
		return GL_FLOAT;
	case gfx_api::vertex_attribute_type::u8x4_norm:
		return GL_UNSIGNED_BYTE;
	}
}

GLboolean get_normalisation(const gfx_api::vertex_attribute_type& type)
{
	switch (type)
	{
	case gfx_api::vertex_attribute_type::float2:
	case gfx_api::vertex_attribute_type::float3:
	case gfx_api::vertex_attribute_type::float4:
		return GL_FALSE;
	case gfx_api::vertex_attribute_type::u8x4_norm:
		return true;
	}
}

struct gl_context : public gfx_api::context
{
	gl_pipeline_state_object* current_program = nullptr;

	virtual gfx_api::texture* create_texture(const size_t & width, const size_t & height, const gfx_api::pixel_format & internal_format, const std::string& filename) override
	{
		auto* new_texture = new gl_texture();
		new_texture->bind();
		if (!filename.empty() && (GLEW_VERSION_4_3 || GLEW_KHR_debug))
		{
			glObjectLabel(GL_TEXTURE, new_texture->id(), -1, filename.c_str());
		}
		for (unsigned i = 0; i < floor(log(std::max(width, height))) + 1; ++i)
		{
			glTexImage2D(GL_TEXTURE_2D, i, to_gl(internal_format), width >> i, height >> i, 0, to_gl(internal_format), GL_UNSIGNED_BYTE, nullptr);
		}
		return new_texture;
	}

	virtual gfx_api::buffer * create_buffer(const gfx_api::buffer::usage &usage, const size_t & width) override
	{
		return new gl_buffer(usage, width);
	}

	virtual gfx_api::pipeline_state_object * build_pipeline(const gfx_api::state_description &state_desc, const SHADER_MODE& shader_mode) override
	{
		return new gl_pipeline_state_object(state_desc, shader_mode);
	}

	virtual void bind_pipeline(gfx_api::pipeline_state_object* pso) override
	{
		current_program = static_cast<gl_pipeline_state_object*>(pso);
		current_program->bind();
	}

	virtual void bind_vertex_buffers(const std::vector<std::vector<gfx_api::vertex_buffer_input>>& attribute_descriptions, const std::vector<gfx_api::buffer*>& vertex_buffers) override
	{
		for (size_t i = 0; i < attribute_descriptions.size() && i < vertex_buffers.size(); i++)
		{
			auto* buffer = static_cast<gl_buffer*>(vertex_buffers[i]);
			buffer->bind();
			for (const auto& attribute : attribute_descriptions[i])
			{
				glEnableVertexAttribArray(attribute.id);
				glVertexAttribPointer(attribute.id, get_size(attribute.type), get_type(attribute.type), get_normalisation(attribute.type), attribute.stride, reinterpret_cast<void*>(attribute.offset));
			}
		}
	}

	virtual void bind_textures(const std::vector<gfx_api::texture_input>& texture_descriptions, const std::vector<gfx_api::texture*>& textures) override
	{
		for (size_t i = 0; i < texture_descriptions.size() && i < textures.size(); ++i)
		{
			const auto& desc = texture_descriptions[i];
			glActiveTexture(GL_TEXTURE0 + desc.id);
			if (textures[i] == nullptr)
			{
				glBindTexture(GL_TEXTURE_2D, 0);
				continue;
			}
			textures[i]->bind();
			switch (desc.sampler)
			{
			case gfx_api::sampler_type::nearest_clamped:
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				break;
			case gfx_api::sampler_type::bilinear:
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				break;
			case gfx_api::sampler_type::anisotropic_repeat:
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
				if (GLEW_EXT_texture_filter_anisotropic)
				{
					GLfloat max;
					glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &max);
					glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, MIN(4.0f, max));
				}
				break;
			case gfx_api::sampler_type::anisotropic:
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				if (GLEW_EXT_texture_filter_anisotropic)
				{
					GLfloat max;
					glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &max);
					glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, MIN(4.0f, max));
				}
				break;
			}
		}
	}


	virtual void set_constants(const void* buffer) override
	{
		current_program->set_constants(buffer);
	}

	virtual void draw(const size_t& offset, const size_t &count, const gfx_api::primitive_type &primitive) override
	{
		glDrawArrays(to_gl(primitive), offset, count);
	}

	virtual void draw_elements(const size_t& offset, const size_t &count, const gfx_api::primitive_type &primitive, const gfx_api::index_type& index) override
	{
		glDrawElements(to_gl(primitive), count, to_gl(index), reinterpret_cast<void*>(offset));
	}
};



gfx_api::context& gfx_api::context::get()
{
	static gl_context ctx;
	return ctx;
}
