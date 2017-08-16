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

	struct state_description
	{
		const REND_MODE blend_state;
		const DEPTH_MODE depth_mode;

		constexpr state_description(REND_MODE _blend_state, DEPTH_MODE _depth_mode) :
			blend_state(_blend_state), depth_mode(_depth_mode) {}
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

	template<REND_MODE render_mode, DEPTH_MODE depth_mode, typename vertex_buffer_inputs>
	struct pipeline_state_helper
	{
		static pipeline_state_helper<render_mode, depth_mode, vertex_buffer_inputs>& get()
		{
			static pipeline_state_helper<render_mode, depth_mode, vertex_buffer_inputs> object;
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
	private:
		pipeline_state_object* pso;
		pipeline_state_helper()
		{
			pso = gfx_api::context::get().build_pipeline(state_description{render_mode, depth_mode});
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
	using Draw3DShape = typename gfx_api::pipeline_state_helper<render_mode, DEPTH_CMP_LEQ_WRT_ON,
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

	using TransColouredTrianglePSO = typename gfx_api::pipeline_state_helper<REND_ADDITIVE, DEPTH_CMP_LEQ_WRT_ON,
		std::tuple<
			vertex_buffer_description<vertex_attribute_description<position, gfx_api::vertex_attribute_type::float3, 0, 0>>
		>>;
	using DrawStencilShadow = typename gfx_api::pipeline_state_helper<REND_OPAQUE, DEPTH_CMP_LEQ_WRT_OFF,
		std::tuple<
			vertex_buffer_description<vertex_attribute_description<position, gfx_api::vertex_attribute_type::float3, 0, 0>>
		>>;
	using TerrainDepth = typename gfx_api::pipeline_state_helper<REND_OPAQUE, DEPTH_CMP_LEQ_WRT_ON,
		std::tuple<
			vertex_buffer_description<vertex_attribute_description<position, gfx_api::vertex_attribute_type::float3, sizeof(glm::vec3), 0>>
		>>;
	using TerrainLayer = typename gfx_api::pipeline_state_helper<REND_ADDITIVE, DEPTH_CMP_LEQ_WRT_OFF,
		std::tuple<
			vertex_buffer_description<vertex_attribute_description<position, gfx_api::vertex_attribute_type::float3, sizeof(glm::vec3), 0>>,
			vertex_buffer_description<vertex_attribute_description<color, gfx_api::vertex_attribute_type::u8x4_norm, sizeof(unsigned), 0>>
		>>;
	using TerrainDecals = typename gfx_api::pipeline_state_helper<REND_ALPHA, DEPTH_CMP_LEQ_WRT_OFF,
		std::tuple<
			vertex_buffer_description<
				vertex_attribute_description<position, gfx_api::vertex_attribute_type::float3, sizeof(glm::vec3) + sizeof(glm::vec2), 0>,
				vertex_attribute_description<texcoord, gfx_api::vertex_attribute_type::float2, sizeof(glm::vec3) + sizeof(glm::vec2), sizeof(glm::vec3)>
			>
		>>;
	using WaterPSO = typename gfx_api::pipeline_state_helper<REND_MULTIPLICATIVE, DEPTH_CMP_LEQ_WRT_OFF,
		std::tuple<
			vertex_buffer_description<vertex_attribute_description<position, gfx_api::vertex_attribute_type::float3, sizeof(glm::vec3), 0>>
		>>;
	using gfx_tc = vertex_buffer_description<vertex_attribute_description<texcoord, gfx_api::vertex_attribute_type::float2, 0, 0>>;
	using gfx_colour = vertex_buffer_description<vertex_attribute_description<color, gfx_api::vertex_attribute_type::u8x4_norm, 0, 0>>;
	using gfx_vtx2 = vertex_buffer_description<vertex_attribute_description<position, gfx_api::vertex_attribute_type::float2, 0, 0>>;
	using gfx_vtx3 = vertex_buffer_description<vertex_attribute_description<position, gfx_api::vertex_attribute_type::float3, 0, 0>>;

	template<REND_MODE rm, DEPTH_MODE dm, typename VTX, typename Second>
	using GFX = typename gfx_api::pipeline_state_helper<rm, dm, std::tuple<VTX, Second>>;
	using VideoPSO = GFX<REND_OPAQUE, DEPTH_CMP_ALWAYS_WRT_OFF, gfx_vtx2, gfx_tc>;
	using BackDropPSO = GFX<REND_OPAQUE, DEPTH_CMP_ALWAYS_WRT_OFF, gfx_vtx2, gfx_tc>;
	using SkyboxPSO = GFX<REND_OPAQUE, DEPTH_CMP_LEQ_WRT_OFF, gfx_vtx3, gfx_tc>;
	using RadarPSO = GFX<REND_ALPHA, DEPTH_CMP_ALWAYS_WRT_OFF, gfx_vtx2, gfx_tc>;
	using RadarViewPSO = GFX<REND_ALPHA, DEPTH_CMP_ALWAYS_WRT_OFF, gfx_vtx2, gfx_colour>;

	using DrawImageTextPSO = typename gfx_api::pipeline_state_helper<REND_TEXT, DEPTH_CMP_ALWAYS_WRT_OFF,
		std::tuple<
			vertex_buffer_description<vertex_attribute_description<position, gfx_api::vertex_attribute_type::u8x4_norm, 0, 0>>
		>>;

	using ShadowBox2DPSO = typename pipeline_state_helper<REND_OPAQUE, DEPTH_CMP_ALWAYS_WRT_OFF,
		std::tuple<
			vertex_buffer_description<vertex_attribute_description<position, gfx_api::vertex_attribute_type::u8x4_norm, 0, 0>>
		>>;
	using UniTransBoxPSO = typename pipeline_state_helper<REND_ALPHA, DEPTH_CMP_ALWAYS_WRT_OFF,
		std::tuple<
			vertex_buffer_description<vertex_attribute_description<position, gfx_api::vertex_attribute_type::u8x4_norm, 0, 0>>
		>>;
	using DrawImagePSO = typename pipeline_state_helper<REND_ALPHA, DEPTH_CMP_ALWAYS_WRT_OFF,
		std::tuple<
			vertex_buffer_description<vertex_attribute_description<position, gfx_api::vertex_attribute_type::u8x4_norm, 0, 0>>
		>>;
	using BoxFillPSO = typename pipeline_state_helper<REND_OPAQUE, DEPTH_CMP_ALWAYS_WRT_OFF,
		std::tuple<
			vertex_buffer_description<vertex_attribute_description<position, gfx_api::vertex_attribute_type::u8x4_norm, 0, 0>>
		>>;
	using BoxFillAlphaPSO = typename pipeline_state_helper<REND_ALPHA, DEPTH_CMP_ALWAYS_WRT_OFF,
		std::tuple<
			vertex_buffer_description<vertex_attribute_description<position, gfx_api::vertex_attribute_type::u8x4_norm, 0, 0>>
		>>;
	using LinePSO = typename pipeline_state_helper<REND_ALPHA, DEPTH_CMP_ALWAYS_WRT_OFF,
		std::tuple<
			vertex_buffer_description<vertex_attribute_description<position, gfx_api::vertex_attribute_type::u8x4_norm, 0, 0>>
		>>;
}
