#include "gfx_api.h"

#include <GL/glew.h>
#include <algorithm>
#include <cmath>
#include "lib/framework/frame.h"
#include <glm/gtc/type_ptr.hpp>
#include <physfs.h>
#include <map>
#include <functional>


struct gl_texture : public gfx_api::texture
{
private:
	friend struct gl_context;
	GLuint _id;

	gl_texture();
	virtual ~gl_texture();
public:
	virtual void bind() override;
	virtual void upload(const size_t& mip_level, const size_t& offset_x, const size_t& offset_y, const size_t & width, const size_t & height, const gfx_api::texel_format & buffer_format, const void * data) override;
	virtual unsigned id() override;
	virtual void generate_mip_levels() override;
};

struct gl_buffer : public gfx_api::buffer
{
	gfx_api::buffer::usage usage;
	GLuint buffer;

	virtual ~gl_buffer() override;
	gl_buffer(const gfx_api::buffer::usage& usage, const size_t& size);
	void bind() override;
	virtual void upload(const size_t & start, const size_t & size, const void * data) override;
};

struct gl_pipeline_state_object : public gfx_api::pipeline_state_object
{
	gfx_api::state_description desc;
	GLuint program;
	std::vector<gfx_api::vertex_buffer> vertex_buffer_desc;
	std::vector<GLint> locations;

	std::function<void(const void*)> uniform_bind_function;

	template<SHADER_MODE shader>
	typename std::pair<SHADER_MODE, std::function<void(const void*)>> uniform_binding_entry();

	gl_pipeline_state_object(const gfx_api::state_description& _desc, const SHADER_MODE& shader, const std::vector<gfx_api::vertex_buffer>& vertex_buffer_desc);
	void set_constants(const void* buffer);

	void bind();

private:
	// Read shader into text buffer
	static char *readShaderBuf(const std::string& name);

	// Retrieve shader compilation errors
	static void printShaderInfoLog(code_part part, GLuint shader);

	// Retrieve shader linkage errors
	static void printProgramInfoLog(code_part part, GLuint program);

	void getLocs();

	void build_program(const std::string& programName, const std::string& vertexPath, const std::string& fragmentPath,
		const std::vector<std::string> &uniformNames);

	void fetch_uniforms(const std::vector<std::string>& uniformNames);

	/**
	* setUniforms is an overloaded wrapper around glUniform* functions
	* accepting glm structures.
	* Please do not use directly, use pie_ActivateShader below.
	*/
	void setUniforms(GLint location, const ::glm::vec4 &v);
	void setUniforms(GLint location, const ::glm::mat4 &m);
	void setUniforms(GLint location, const Vector2i &v);
	void setUniforms(GLint location, const Vector2f &v);
	void setUniforms(GLint location, const int32_t &v);
	void setUniforms(GLint location, const float &v);


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

	void set_constants(const gfx_api::constant_buffer_type<SHADER_BUTTON>& cbuf);
	void set_constants(const gfx_api::constant_buffer_type<SHADER_COMPONENT>& cbuf);
	void set_constants(const gfx_api::constant_buffer_type<SHADER_NOLIGHT>& cbuf);
	void set_constants(const gfx_api::constant_buffer_type<SHADER_TERRAIN>& cbuf);
	void set_constants(const gfx_api::constant_buffer_type<SHADER_TERRAIN_DEPTH>& cbuf);
	void set_constants(const gfx_api::constant_buffer_type<SHADER_DECALS>& cbuf);
	void set_constants(const gfx_api::constant_buffer_type<SHADER_WATER>& cbuf);
	void set_constants(const gfx_api::constant_buffer_type<SHADER_RECT>& cbuf);
	void set_constants(const gfx_api::constant_buffer_type<SHADER_TEXRECT>& cbuf);
	void set_constants(const gfx_api::constant_buffer_type<SHADER_GFX_COLOUR>& cbuf);
	void set_constants(const gfx_api::constant_buffer_type<SHADER_GFX_TEXT>& cbuf);
	void set_constants(const gfx_api::constant_buffer_type<SHADER_GENERIC_COLOR>& cbuf);
	void set_constants(const gfx_api::constant_buffer_type<SHADER_LINE>& cbuf);
	void set_constants(const gfx_api::constant_buffer_type<SHADER_TEXT>& cbuf);
};

struct gl_context : public gfx_api::context
{
	gl_pipeline_state_object* current_program = nullptr;
	SDL_Window* WZwindow;
	GLuint scratchbuffer;

	gl_context();
	~gl_context();

	virtual gfx_api::texture* create_texture(const size_t& mipmap_count, const size_t & width, const size_t & height, const gfx_api::texel_format & internal_format, const std::string& filename) override;
	virtual gfx_api::buffer * create_buffer(const gfx_api::buffer::usage &usage, const size_t & width) override;
	virtual gfx_api::pipeline_state_object * build_pipeline(const gfx_api::state_description &state_desc,
		const SHADER_MODE& shader_mode,
		const gfx_api::primitive_type& primitive,
		const std::vector<gfx_api::texture_input>& texture_desc,
		const std::vector<gfx_api::vertex_buffer>& attribute_descriptions) override;
	virtual void bind_pipeline(gfx_api::pipeline_state_object* pso) override;
	virtual void bind_index_buffer(gfx_api::buffer&, const gfx_api::index_type&) override;
	virtual void bind_vertex_buffers(const std::size_t& first, const std::vector<std::tuple<gfx_api::buffer*, std::size_t>>& vertex_buffers_offset) override;
	virtual void bind_streamed_vertex_buffers(const void* data, const std::size_t size) override;
	virtual void bind_textures(const std::vector<gfx_api::texture_input>& texture_descriptions, const std::vector<gfx_api::texture*>& textures) override;
	virtual void setSwapchain(struct SDL_Window* window) override;
	virtual void set_constants(const void* buffer, const size_t& size) override;
	virtual void draw(const size_t& offset, const size_t &count, const gfx_api::primitive_type &primitive) override;
	virtual void draw_elements(const size_t& offset, const size_t &count, const gfx_api::primitive_type &primitive, const gfx_api::index_type& index) override;
	virtual void flip() override;
};
