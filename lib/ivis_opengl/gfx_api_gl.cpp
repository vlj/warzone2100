#include "gfx_api.h"

#include <GL/glew.h>
#include <algorithm>
#include <cmath>
#include "lib/framework/frame.h"

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

struct gl_pipeline_state_object : public gfx_api::pipeline_state_object
{
	gfx_api::state_description desc;
	gl_pipeline_state_object(const gfx_api::state_description& _desc) :
		desc(_desc)
	{

	}

	virtual void bind() override
	{
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
	virtual gfx_api::texture* create_texture(const size_t & width, const size_t & height, const gfx_api::pixel_format & internal_format, const std::string& filename) override
	{
		auto* new_texture = new gl_texture();
		new_texture->bind();
		if (!filename.empty() && (GLEW_VERSION_4_3 || GLEW_KHR_debug))
		{
			glObjectLabel(GL_TEXTURE, new_texture->id(), -1, filename.c_str());
		}
		for (unsigned i = 0; i < floor(log(std::max(width, height))); ++i)
		{
			glTexImage2D(GL_TEXTURE_2D, i, to_gl(internal_format), width >> i, height >> i, 0, to_gl(internal_format), GL_UNSIGNED_BYTE, nullptr);
		}
		return new_texture;
	}

	virtual gfx_api::buffer * create_buffer(const gfx_api::buffer::usage &usage, const size_t & width) override
	{
		return new gl_buffer(usage, width);
	}

	virtual gfx_api::pipeline_state_object * build_pipeline(const gfx_api::state_description &state_desc) override
	{
		return new gl_pipeline_state_object(state_desc);
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

	virtual void draw(const size_t &count, const gfx_api::primitive_type &primitive) override
	{
		glDrawArrays(to_gl(primitive), 0, count);
	}

	virtual void draw_elements(const size_t &count, const gfx_api::primitive_type &primitive) override
	{
		glDrawElements(to_gl(primitive), count, GL_UNSIGNED_SHORT, nullptr);
	}
};



gfx_api::context& gfx_api::context::get()
{
	static gl_context ctx;
	return ctx;
}
