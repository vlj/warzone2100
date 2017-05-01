#pragma once
#include "lib/ivis_opengl/api_object.h"
#include "lib/ivis_opengl/opengl.h"
#include <vector>
#include <functional>

namespace gfx_api
{
	namespace gl_api
	{
		struct gl_buffer final : gfx_api::buffer
		{
			GLuint object;
			GLenum target;
			virtual ~gl_buffer();

			// Hérité via buffer
			virtual void upload(size_t offset, size_t s, const void * ptr) override;
		};

		struct gl_texture final : gfx_api::texture
		{
			GLuint object;
			virtual ~gl_texture();

			// Hérité via texture
			virtual void upload_texture(const format& f, size_t width, size_t height, const void * ptr) override;

		};

		struct gl_uniforms final : gfx_api::uniforms
		{
			void* memory;
			size_t size;
			gl_uniforms(const size_t size);
			virtual ~gl_uniforms();

			// Inherited via uniforms
			virtual void unmap() override;
			virtual void * map_impl() const override;
		};

		struct gl_program final : gfx_api::program
		{
			GLuint program;
			GLuint vao;
			std::function<void(const void*)> bind_uniforms;
			std::function<void(const std::vector<texture*>&)> bind_textures;

			// Uniforms
			std::vector<GLint> locations;

			// Attributes
			std::vector<GLint> attributes;

			gl_program();
			virtual ~gl_program() override;

			// Hérité via program
			virtual void set_vertex_buffers(const std::vector<buffer_layout>& layout,
				const std::vector<std::tuple<const buffer&, size_t>>& buffers_and_offset) override;

			// Hérité via program
			virtual void use() override;

			// Hérité via program
			virtual void set_index_buffer(const buffer & b) override;

			// Inherited via program
			virtual void set_uniforms(const uniforms & uniforms) override;

			virtual void set_textures(const std::vector<texture*>& textures) override;
		};

		struct gl_context final : gfx_api::context
		{
			gl_context();
			virtual std::unique_ptr<texture> create_texture(format f, size_t width, size_t height, size_t levels) override;
			virtual std::unique_ptr<buffer> create_buffer(size_t size, const bool &is_index) override;

			// Hérité via context
			virtual std::vector<std::unique_ptr<program>> build_programs() override;

			// Inherited via context
			virtual std::unique_ptr<uniforms> get_uniform_storage(const size_t & capacity) override;
		};
	}
}

/*namespace pie_internal
{
struct SHADER_PROGRAM
{



};

extern std::vector<SHADER_PROGRAM> shaderProgram;
extern SHADER_MODE currentShaderMode;*/


/**
* uniformSetter is a variadic function object.
* It's recursively expanded so that uniformSetter(array, arg0, arg1...);
* will yield the following code:
* {
*     setUniforms(arr[0], arg0);
*     setUniforms(arr[1], arg1);
*     setUniforms(arr[2], arg2);
*     ...
*     setUniforms(arr[n], argn);
* }
*/
/*template<typename...T>
struct uniformSetter
{
void operator()(const std::vector<GLint> &locations, T...) const;
};

template<>
struct uniformSetter<>
{
void operator()(const std::vector<GLint> &) const {}
};

template<typename T, typename...Args>
struct uniformSetter<T, Args...>
{
void operator()(const std::vector<GLint> &locations, const T& value, const Args&...args) const
{
constexpr int N = sizeof...(Args) + 1;
setUniforms(locations[locations.size() - N], value);
uniformSetter<Args...>()(locations, args...);
}
};
}
*/

/**
* Bind program and sets the uniforms Args.
* The uniform binding mechanism works in conjonction with pie_LoadShader.
* When shader is loaded uniform location is fetched in a certain order.
* in pie_ActivateShader the uniforms passed as variadic Args template
* must be given in the same order.
*
* For instance if a shader is loaded by :
* pie_LoadShader(..., { vector_uniform_name, matrix_uniform_name });
* then subsequent pie_ActivateShader must be called like :
* pie_ActivateShader(..., some_vector, some_matrix);
* It will be expanded as :
* {
*     glUseProgram(...);
*     glUniform4f(vector_uniform_location, some_vector);
*     glUniform4fv(matrix_uniform_location, some_matrix);
* }
* Note that uniform count is checked at run time.
* Uniform type is not checked but the GL implementation will complain
* if the uniform type doesn't match.
*/
/*template<typename...T>
pie_internal::SHADER_PROGRAM &pie_ActivateShader(SHADER_MODE shaderMode, const T&... Args)
{
pie_internal::SHADER_PROGRAM &program = pie_internal::shaderProgram[shaderMode];
if (shaderMode != pie_internal::currentShaderMode)
{
glUseProgram(program.program);
pie_internal::currentShaderMode = shaderMode;
}
assert(program.locations.size() == sizeof...(T));
pie_internal::uniformSetter<T...>()(program.locations, Args...);
return program;
}*/

