#pragma once
#include <memory>
#include <vector>

namespace gfx_api
{
	enum class format {
		rgb,
		rgba,
	};

	enum class drawtype {
	triangles,
	triangle_fan,
	triangle_strip,
	line_strip,
	};

	enum class filter {
		nearest,
		linear,
	};

	enum class buffer_type {
		vec1,
		vec2,
		vec3,
		vec4,
		ub4,
	};

	struct texture
	{
		virtual void upload_texture(const format& f, size_t width, size_t height, const void* ptr) = 0;
		virtual ~texture() {}
	};

	struct buffer
	{
		virtual void upload(size_t offset, size_t s, const void* ptr) = 0;
		virtual ~buffer() {}
	};

	struct buffer_layout
	{
		buffer_type type;
		size_t stride;
	};

	struct uniforms
	{
		template<typename T>
		T& map()
		{
			return *static_cast<T*>(map_impl());
		}

		virtual void unmap() = 0;
		virtual ~uniforms();
	private:
		virtual void *map_impl() const = 0;

	};

	struct program
	{
		virtual ~program() {}

		virtual void set_index_buffer(const buffer& b) = 0;
		virtual void set_vertex_buffers(const std::vector<buffer_layout>& layout,
			const std::vector<std::tuple<const buffer&, size_t> >& buffers_and_offset) = 0;
		virtual void set_uniforms(const uniforms& uniforms) = 0;

		virtual void use() = 0;
	};

	struct context
	{
		virtual ~context() {}
		virtual std::unique_ptr<texture> create_texture(format f, size_t width, size_t height, size_t level) = 0;
		virtual std::unique_ptr<buffer> create_buffer(size_t size, const bool& is_index = false) = 0;
		virtual std::unique_ptr<uniforms> get_uniform_storage(const size_t& capacity) = 0;
		virtual std::vector<std::unique_ptr<program> > build_programs() = 0;

		static context& get_context();
	};
}
