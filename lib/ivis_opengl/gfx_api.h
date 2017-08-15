#pragma once

#include <memory>
#include <string>
#include <vector>
#include <tuple>
#include "pietypes.h"

namespace gfx_api
{
	enum class pixel_format
	{
		invalid,
		rgb,
		rgba,
		compressed_rgb,
		compressed_rgba,
	};

	struct texture
	{
		virtual ~texture() {};
		virtual void bind() = 0;
		virtual void upload(const size_t& mip_level, const size_t& offset_x, const size_t& offset_y, const size_t& width, const size_t& height, const pixel_format& buffer_format, const void* data) = 0;
		virtual void generate_mip_levels() = 0;
		virtual unsigned id() = 0;
	};

	struct buffer
	{
		enum class usage
		{
			vertex_buffer,
			index_buffer,
		};

		virtual void upload(const size_t& start, const size_t& size, const void* data) = 0;
		virtual void bind() = 0;
		virtual ~buffer() {};

	};

	enum class primitive_type
	{
		lines,
		triangles,
		triangle_strip,
		triangle_fan,
	};

	enum class index_type
	{
		u16,
		u32,
	};

	enum class polygon_offset
	{
		enabled,
		disabled,
	};

	enum class stencil_mode
	{
		stencil_shadow_silhouette,
		stencil_shadow_quad,
		stencil_disabled,
	};

	enum class cull_mode
	{
		back,
		none,
	};

	struct state_description
	{
		const REND_MODE blend_state;
		const DEPTH_MODE depth_mode;
		const uint8_t output_mask;
		const bool offset;
		const stencil_mode stencil;
		const cull_mode cull;

		constexpr state_description(REND_MODE _blend_state, DEPTH_MODE _depth_mode, uint8_t _output_mask, polygon_offset _polygon_offset, stencil_mode _stencil, cull_mode _cull) :
			blend_state(_blend_state), depth_mode(_depth_mode), output_mask(_output_mask), offset(_polygon_offset == polygon_offset::enabled), stencil(_stencil), cull(_cull) {}
	};

	enum class vertex_attribute_type
	{
		float2,
		float3,
		float4,
		u8x4_norm,
	};

	struct vertex_buffer_input
	{
		const size_t id;
		const vertex_attribute_type type;
		const size_t stride;
		const size_t offset;

		constexpr vertex_buffer_input(size_t _id, vertex_attribute_type _type, size_t _stride, size_t _offset)
			: id(_id), type(_type), stride(_stride), offset(_offset)
		{}
	};

	struct pipeline_state_object
	{
		virtual ~pipeline_state_object() {}
		virtual void bind() = 0;
	};

	struct context
	{
		virtual ~context() {};
		virtual texture* create_texture(const size_t& width, const size_t& height, const pixel_format& internal_format, const std::string& filename = "") = 0;
		virtual buffer* create_buffer(const enum class buffer::usage&, const size_t& width) = 0;
		virtual pipeline_state_object* build_pipeline(const state_description&) = 0;
		virtual void bind_vertex_buffers(const std::vector<std::vector<vertex_buffer_input>>& attribute_descriptions, const std::vector<buffer*>& vertex_buffers) = 0;
		virtual void draw(const size_t& offset, const size_t&, const primitive_type&) = 0;
		virtual void draw_elements(const size_t& offset, const size_t&, const primitive_type&, const index_type&) = 0;
		static context& get();
	};

	template<size_t id, vertex_attribute_type type, size_t stride, size_t offset>
	struct vertex_attribute_description
	{
		static vertex_buffer_input get_desc()
		{
			return vertex_buffer_input{id, type, stride, offset};
		}
	};

	/**
	 * A struct templated by a tuple.
	 * Describes a buffer input.
	 * input_description describes the various vertex attributes fetched from this buffer.
	 */
	template<typename... input_description>
	struct vertex_buffer_description
	{
		static std::vector<vertex_buffer_input> get_desc()
		{
			return { input_description::get_desc()... };
		}
	};

	template<REND_MODE render_mode, DEPTH_MODE depth_mode, uint8_t output_mask, polygon_offset offset, stencil_mode stencil, cull_mode cull>
	struct rasterizer_state
	{
		static state_description get()
		{
			return state_description{ render_mode, depth_mode, output_mask, offset, stencil, cull };
		}
	};

	template<typename rasterizer, primitive_type primitive, index_type index, typename vertex_buffer_inputs>
	struct pipeline_state_helper
	{
		static pipeline_state_helper<rasterizer, primitive, index, vertex_buffer_inputs>& get()
		{
			static pipeline_state_helper<rasterizer, primitive, index, vertex_buffer_inputs> object;
			return object;
		}

		void bind()
		{
			pso->bind();
		}

		template<typename... Args>
		void bind_vertex_buffers(Args&&... args)
		{
			static_assert(sizeof...(args) == std::tuple_size<vertex_buffer_inputs>::value, "Wrong number of vertex buffer");
			gfx_api::context::get().bind_vertex_buffers(untuple(vertex_buffer_inputs{}), { args... });
		}

		void draw(const size_t& count, const size_t& offset)
		{
			context::get().draw(offset, count, primitive);
		}

		void draw_elements(const size_t& count, const size_t& offset)
		{
			context::get().draw_elements(offset, count, primitive, index);
		}
	private:
		pipeline_state_object* pso;
		pipeline_state_helper()
		{
			pso = gfx_api::context::get().build_pipeline(rasterizer::get());
		}

		template<typename...Args>
		std::vector<std::vector<vertex_buffer_input>> untuple(const std::tuple<Args...>&)
		{
			return { Args::get_desc()... };
		}
	};

	constexpr size_t position = 0;
	constexpr size_t texcoord = 1;
	constexpr size_t color = 2;
	constexpr size_t normal = 3;

	template<REND_MODE render_mode>
	using Draw3DShape = typename gfx_api::pipeline_state_helper<rasterizer_state<render_mode, DEPTH_CMP_LEQ_WRT_ON, 255, polygon_offset::disabled, stencil_mode::stencil_disabled, cull_mode::back>, primitive_type::triangles, index_type::u16,
		std::tuple<
			vertex_buffer_description<vertex_attribute_description<position, gfx_api::vertex_attribute_type::float3, 0, 0>>,
			vertex_buffer_description<vertex_attribute_description<normal, gfx_api::vertex_attribute_type::float3, 0, 0>>,
			vertex_buffer_description<vertex_attribute_description<texcoord, gfx_api::vertex_attribute_type::float2, 0, 0>>
		>>;

	using Draw3DButtonPSO = Draw3DShape<REND_OPAQUE>;
	using Draw3DShapeOpaque = Draw3DShape<REND_OPAQUE>;
	using Draw3DShapeAlpha = Draw3DShape<REND_ALPHA>;
	using Draw3DShapePremul = Draw3DShape<REND_PREMULTIPLIED>;
	using Draw3DShapeAdditive = Draw3DShape<REND_ADDITIVE>;

	using TransColouredTrianglePSO = typename gfx_api::pipeline_state_helper<rasterizer_state<REND_ADDITIVE, DEPTH_CMP_LEQ_WRT_ON, 255, polygon_offset::disabled, stencil_mode::stencil_disabled, cull_mode::back>, primitive_type::triangle_fan, index_type::u16,
		std::tuple<
			vertex_buffer_description<vertex_attribute_description<position, gfx_api::vertex_attribute_type::float3, 0, 0>>
		>>;
	using DrawStencilShadow = typename gfx_api::pipeline_state_helper<rasterizer_state<REND_OPAQUE, DEPTH_CMP_LEQ_WRT_OFF, 0, polygon_offset::disabled, stencil_mode::stencil_shadow_silhouette, cull_mode::none>, primitive_type::triangles, index_type::u16,
		std::tuple<
			vertex_buffer_description<vertex_attribute_description<position, gfx_api::vertex_attribute_type::float3, 0, 0>>
		>>;
	using TerrainDepth = typename gfx_api::pipeline_state_helper<rasterizer_state<REND_OPAQUE, DEPTH_CMP_LEQ_WRT_ON, 0, polygon_offset::enabled, stencil_mode::stencil_disabled, cull_mode::back>, primitive_type::triangles, index_type::u32,
		std::tuple<
			vertex_buffer_description<vertex_attribute_description<position, gfx_api::vertex_attribute_type::float3, sizeof(glm::vec3), 0>>
		>>;
	using TerrainLayer = typename gfx_api::pipeline_state_helper<rasterizer_state<REND_ADDITIVE, DEPTH_CMP_LEQ_WRT_OFF, 255, polygon_offset::disabled, stencil_mode::stencil_disabled, cull_mode::back>, primitive_type::triangles, index_type::u32,
		std::tuple<
			vertex_buffer_description<vertex_attribute_description<position, gfx_api::vertex_attribute_type::float3, sizeof(glm::vec3), 0>>,
			vertex_buffer_description<vertex_attribute_description<color, gfx_api::vertex_attribute_type::u8x4_norm, sizeof(unsigned), 0>>
		>>;
	using TerrainDecals = typename gfx_api::pipeline_state_helper<rasterizer_state<REND_ALPHA, DEPTH_CMP_LEQ_WRT_OFF, 255, polygon_offset::disabled, stencil_mode::stencil_disabled, cull_mode::back>, primitive_type::triangles, index_type::u16,
		std::tuple<
			vertex_buffer_description<
				vertex_attribute_description<position, gfx_api::vertex_attribute_type::float3, sizeof(glm::vec3) + sizeof(glm::vec2), 0>,
				vertex_attribute_description<texcoord, gfx_api::vertex_attribute_type::float2, sizeof(glm::vec3) + sizeof(glm::vec2), sizeof(glm::vec3)>
			>
		>>;
	using WaterPSO = typename gfx_api::pipeline_state_helper<rasterizer_state<REND_MULTIPLICATIVE, DEPTH_CMP_LEQ_WRT_OFF, 255, polygon_offset::disabled, stencil_mode::stencil_disabled, cull_mode::back>, primitive_type::triangles, index_type::u32,
		std::tuple<
			vertex_buffer_description<vertex_attribute_description<position, gfx_api::vertex_attribute_type::float3, sizeof(glm::vec3), 0>>
		>>;
	using gfx_tc = vertex_buffer_description<vertex_attribute_description<texcoord, gfx_api::vertex_attribute_type::float2, 0, 0>>;
	using gfx_colour = vertex_buffer_description<vertex_attribute_description<color, gfx_api::vertex_attribute_type::u8x4_norm, 0, 0>>;
	using gfx_vtx2 = vertex_buffer_description<vertex_attribute_description<position, gfx_api::vertex_attribute_type::float2, 0, 0>>;
	using gfx_vtx3 = vertex_buffer_description<vertex_attribute_description<position, gfx_api::vertex_attribute_type::float3, 0, 0>>;

	template<REND_MODE rm, DEPTH_MODE dm, primitive_type primitive, typename VTX, typename Second>
	using GFX = typename gfx_api::pipeline_state_helper<rasterizer_state<rm, dm, 255, polygon_offset::disabled, stencil_mode::stencil_disabled, cull_mode::back>, primitive, index_type::u16, std::tuple<VTX, Second>>;
	using VideoPSO = GFX<REND_OPAQUE, DEPTH_CMP_ALWAYS_WRT_OFF, primitive_type::triangle_strip, gfx_vtx2, gfx_tc>;
	using BackDropPSO = GFX<REND_OPAQUE, DEPTH_CMP_ALWAYS_WRT_OFF, primitive_type::triangle_strip, gfx_vtx2, gfx_tc>;
	using SkyboxPSO = GFX<REND_OPAQUE, DEPTH_CMP_LEQ_WRT_OFF, primitive_type::triangles, gfx_vtx3, gfx_tc>;
	using RadarPSO = GFX<REND_ALPHA, DEPTH_CMP_ALWAYS_WRT_OFF, primitive_type::triangle_strip, gfx_vtx2, gfx_tc>;
	using RadarViewPSO = GFX<REND_ALPHA, DEPTH_CMP_ALWAYS_WRT_OFF, primitive_type::triangle_strip, gfx_vtx2, gfx_colour>;

	using DrawImageTextPSO = typename gfx_api::pipeline_state_helper<rasterizer_state<REND_TEXT, DEPTH_CMP_ALWAYS_WRT_OFF, 255, polygon_offset::disabled, stencil_mode::stencil_disabled, cull_mode::none>, primitive_type::triangle_strip, index_type::u16,
		std::tuple<
			vertex_buffer_description<vertex_attribute_description<position, gfx_api::vertex_attribute_type::u8x4_norm, 0, 0>>
		>>;

	using ShadowBox2DPSO = typename pipeline_state_helper<rasterizer_state<REND_OPAQUE, DEPTH_CMP_ALWAYS_WRT_OFF, 255, polygon_offset::disabled, stencil_mode::stencil_disabled, cull_mode::back>, primitive_type::triangle_strip, index_type::u16,
		std::tuple<
			vertex_buffer_description<vertex_attribute_description<position, gfx_api::vertex_attribute_type::u8x4_norm, 0, 0>>
		>>;
	using UniTransBoxPSO = typename pipeline_state_helper<rasterizer_state<REND_ALPHA, DEPTH_CMP_ALWAYS_WRT_OFF, 255, polygon_offset::disabled, stencil_mode::stencil_disabled, cull_mode::back>, primitive_type::triangle_strip, index_type::u16,
		std::tuple<
			vertex_buffer_description<vertex_attribute_description<position, gfx_api::vertex_attribute_type::u8x4_norm, 0, 0>>
		>>;
	using DrawImagePSO = typename pipeline_state_helper<rasterizer_state<REND_ALPHA, DEPTH_CMP_ALWAYS_WRT_OFF, 255, polygon_offset::disabled, stencil_mode::stencil_disabled, cull_mode::back>, primitive_type::triangle_strip, index_type::u16,
		std::tuple<
			vertex_buffer_description<vertex_attribute_description<position, gfx_api::vertex_attribute_type::u8x4_norm, 0, 0>>
		>>;
	using BoxFillPSO = typename pipeline_state_helper<rasterizer_state<REND_OPAQUE, DEPTH_CMP_ALWAYS_WRT_OFF, 255, polygon_offset::disabled, stencil_mode::stencil_disabled, cull_mode::back>, primitive_type::triangle_strip, index_type::u16,
		std::tuple<
			vertex_buffer_description<vertex_attribute_description<position, gfx_api::vertex_attribute_type::u8x4_norm, 0, 0>>
		>>;
	using BoxFillAlphaPSO = typename pipeline_state_helper<rasterizer_state<REND_ALPHA, DEPTH_CMP_ALWAYS_WRT_OFF, 255, polygon_offset::disabled, stencil_mode::stencil_shadow_quad, cull_mode::back>, primitive_type::triangle_strip, index_type::u16,
		std::tuple<
			vertex_buffer_description<vertex_attribute_description<position, gfx_api::vertex_attribute_type::u8x4_norm, 0, 0>>
		>>;
	using LinePSO = typename pipeline_state_helper<rasterizer_state<REND_ALPHA, DEPTH_CMP_ALWAYS_WRT_OFF, 255, polygon_offset::disabled, stencil_mode::stencil_disabled, cull_mode::back>, primitive_type::lines, index_type::u16,
		std::tuple<
			vertex_buffer_description<vertex_attribute_description<position, gfx_api::vertex_attribute_type::u8x4_norm, 0, 0>>
		>>;
}

