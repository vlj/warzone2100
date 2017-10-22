#include "gfx_api_vk.h"

static auto findProperties(const vk::PhysicalDeviceMemoryProperties& memprops, const uint32_t& memoryTypeBits, const vk::MemoryPropertyFlagBits& properties)
{
	for (int32_t i = 0; i < memprops.memoryTypeCount; ++i)
	{
		if ((memoryTypeBits & (1 << i)) &&
			((memprops.memoryTypes[i].propertyFlags & properties) == properties))
			return i;
	}
	return -1;
}

circularHostBuffer::circularHostBuffer(vk::Device &d, vk::PhysicalDeviceMemoryProperties memprops, uint32_t s)
	: dev(d), size(s), gpuReadLocation(size - 1), hostWriteLocation(0)
{
	buffer = dev.createBuffer(
		vk::BufferCreateInfo{}
		.setSize(s)
		.setUsage(vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eUniformBuffer)
	);
	const auto& memreq = dev.getBufferMemoryRequirements(buffer);
	memory = dev.allocateMemory(
		vk::MemoryAllocateInfo{}
		.setAllocationSize(memreq.size)
		.setMemoryTypeIndex(findProperties(memprops, memreq.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostCoherent))
	);
	dev.bindBufferMemory(buffer, memory, 0);
}

circularHostBuffer::~circularHostBuffer()
{
	dev.freeMemory(memory);
	dev.destroyBuffer(buffer);
}

bool circularHostBuffer::isBetween(uint32_t rangeBegin, uint32_t rangeEnd, uint32_t position)
{
	return !(rangeBegin > position ||
		position >= rangeEnd);
}

std::tuple<uint32_t, uint32_t> circularHostBuffer::getWritePosAndNewWriteLocation(uint32_t currentWritePos, uint32_t amount, uint32_t totalSize, uint32_t align)
{
	assert(amount < totalSize);
	currentWritePos = ((currentWritePos + align - 1) / align) * align;
	if (currentWritePos + amount < totalSize)
	{
		return std::make_tuple(currentWritePos, currentWritePos + amount);
	}
	return std::make_tuple(0u, amount);
}

uint32_t circularHostBuffer::alloc(uint32_t amount, uint32_t align)
{
	const auto& oldAndNewPos = getWritePosAndNewWriteLocation(hostWriteLocation, amount, size, align);
	if (isBetween(std::get<0>(oldAndNewPos), std::get<1>(oldAndNewPos), gpuReadLocation))
		throw;
	hostWriteLocation = std::get<1>(oldAndNewPos);
	return std::get<0>(oldAndNewPos);
}

perFrameResources_t::perFrameResources_t(vk::Device& _dev, vk::Image _swapchainImage, const uint32_t& graphicsQueueIndex) :
	dev(_dev), swapchainImages(_swapchainImage)
{
	const auto& descriptorSize =
		std::array<vk::DescriptorPoolSize, 2> {
		vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, 10000),
			vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, 10000)
	};
	descriptorPool = dev.createDescriptorPool(
		vk::DescriptorPoolCreateInfo{}
		.setMaxSets(10000)
		.setPPoolSizes(descriptorSize.data())
		.setPoolSizeCount(descriptorSize.size())
	);
	pool = dev.createCommandPool(
		vk::CommandPoolCreateInfo{}
		.setQueueFamilyIndex(graphicsQueueIndex)
		.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer));
	const auto& buffer = dev.allocateCommandBuffers(
		vk::CommandBufferAllocateInfo{}
		.setCommandPool(pool)
		.setCommandBufferCount(2)
		.setLevel(vk::CommandBufferLevel::ePrimary)
	);
	cmdDraw = buffer[0];
	cmdCopy = buffer[1];
	cmdCopy.begin(vk::CommandBufferBeginInfo{}.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
	previousSubmission = dev.createFence(
		vk::FenceCreateInfo{}.setFlags(vk::FenceCreateFlagBits::eSignaled)
	);
}

void perFrameResources_t::clean()
{
	buffer_to_delete.clear();
	image_view_to_delete.clear();
	image_to_delete.clear();
	memory_to_free.clear();
}

perFrameResources_t::~perFrameResources_t()
{
	dev.destroyCommandPool(pool);
	dev.destroyDescriptorPool(descriptorPool);
	dev.destroyFence(previousSubmission);
}

perFrameResources_t& buffering_mechanism::get_current_resources()
{
	return *perFrameResources[currentSwapchainIndex];
}

void buffering_mechanism::init(vk::Device dev)
{
	currentSwapchainIndex = 0;
	dev.resetFences({ buffering_mechanism::get_current_resources().previousSubmission });
	acquireSemaphore = dev.createSemaphore(vk::SemaphoreCreateInfo{});
	acquireSwapchainIndex(dev);
}

void buffering_mechanism::destroy(vk::Device dev)
{
	scratchBuffer.release();
	perFrameResources.clear();
	dev.destroySemaphore(acquireSemaphore);
}

void buffering_mechanism::swap(vk::Device dev)
{
	acquireSwapchainIndex(dev);
	// Note : currentSwapchainIndex has changed !
	buffering_mechanism::scratchBuffer->gpuReadLocation = buffering_mechanism::scratchBuffer->hostWriteLocation - 1;
	dev.waitForFences({ buffering_mechanism::get_current_resources().previousSubmission }, true, -1);
	dev.resetFences({ buffering_mechanism::get_current_resources().previousSubmission });
	dev.resetDescriptorPool(buffering_mechanism::get_current_resources().descriptorPool);
	dev.resetCommandPool(buffering_mechanism::get_current_resources().pool, vk::CommandPoolResetFlagBits{});

	buffering_mechanism::get_current_resources().clean();
	buffering_mechanism::get_current_resources().numalloc = 0;
}

void buffering_mechanism::acquireSwapchainIndex(vk::Device dev)
{
	currentSwapchainIndex = dev.acquireNextImageKHR(swapchain, -1, acquireSemaphore, vk::Fence{}).value;
}

std::unique_ptr<circularHostBuffer> buffering_mechanism::scratchBuffer;
std::vector<std::unique_ptr<perFrameResources_t>> buffering_mechanism::perFrameResources;
uint32_t buffering_mechanism::currentSwapchainIndex;
vk::Semaphore buffering_mechanism::acquireSemaphore;
vk::SwapchainKHR buffering_mechanism::swapchain;

VkBool32 messageCallback(
	VkDebugReportFlagsEXT flags,
	VkDebugReportObjectTypeEXT objType,
	uint64_t srcObject,
	std::size_t location,
	int32_t msgCode,
	const char* pLayerPrefix,
	const char* pMsg,
	void* pUserData)
{
	std::stringstream buf;
	if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
		buf << "ERROR: ";
	}
	else if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT) {
		buf << "WARNING: ";
	}
	else if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT) {
		buf << "PERF: ";
	}
	else {
		return false;
	}
	buf << "[" << pLayerPrefix << "] Code " << msgCode << " : " << pMsg;
	debug(LOG_FATAL, buf.str().c_str());
	return false;
}


struct shader_infos
{
	std::string vertex;
	std::string fragment;
};

static const std::map<SHADER_MODE, shader_infos> spv_files
{
	std::make_pair(SHADER_COMPONENT, shader_infos{ "shaders/vk/tcmask.vert.spv", "shaders/vk/tcmask.frag.spv" }),
	std::make_pair(SHADER_BUTTON, shader_infos{ "shaders/vk/button.vert.spv", "shaders/vk/button.frag.spv" }),
	std::make_pair(SHADER_NOLIGHT, shader_infos{ "shaders/vk/nolight.vert.spv", "shaders/vk/nolight.frag.spv" }),
	std::make_pair(SHADER_TERRAIN, shader_infos{ "shaders/vk/terrain_water.vert.spv", "shaders/vk/terrain.frag.spv" }),
	std::make_pair(SHADER_TERRAIN_DEPTH, shader_infos{ "shaders/vk/terrain_water.vert.spv", "shaders/vk/terraindepth.frag.spv" }),
	std::make_pair(SHADER_DECALS, shader_infos{ "shaders/vk/decals.vert.spv", "shaders/vk/decals.frag.spv" }),
	std::make_pair(SHADER_WATER, shader_infos{ "shaders/vk/terrain_water.vert.spv", "shaders/vk/water.frag.spv" }),
	std::make_pair(SHADER_RECT, shader_infos{ "shaders/vk/rect.vert.spv", "shaders/vk/rect.frag.spv" }),
	std::make_pair(SHADER_TEXRECT, shader_infos{ "shaders/vk/rect.vert.spv", "shaders/vk/texturedrect.frag.spv" }),
	std::make_pair(SHADER_GFX_COLOUR, shader_infos{ "shaders/vk/gfx.vert.spv", "shaders/vk/gfx.frag.spv" }),
	std::make_pair(SHADER_GFX_TEXT, shader_infos{ "shaders/vk/gfx.vert.spv", "shaders/vk/texturedrect.frag.spv" }),
	std::make_pair(SHADER_GENERIC_COLOR, shader_infos{ "shaders/vk/generic.vert.spv", "shaders/vk/rect.frag.spv" }),
	std::make_pair(SHADER_LINE, shader_infos{ "shaders/vk/line.vert.spv", "shaders/vk/rect.frag.spv" }),
	std::make_pair(SHADER_TEXT, shader_infos{ "shaders/vk/rect.vert.spv", "shaders/vk/text.frag.spv" })
};


std::vector<uint32_t> VkPSO::readShaderBuf(const std::string& name)
{
	auto fp = PHYSFS_openRead(name.c_str());
	debug(LOG_3D, "Reading...[directory: %s] %s", PHYSFS_getRealDir(name.c_str()), name.c_str());
	assert(fp != nullptr);
	const auto& filesize = PHYSFS_fileLength(fp);
	auto&& buffer = std::vector<uint32_t>(filesize / sizeof(uint32_t));
	PHYSFS_read(fp, buffer.data(), 1, filesize);
	PHYSFS_close(fp);

	return buffer;
}

vk::ShaderModule VkPSO::get_module(const std::string& name)
{
	const auto& tmp = readShaderBuf(name);
	return dev.createShaderModule(
		vk::ShaderModuleCreateInfo{}
		.setCodeSize(tmp.size() * 4)
		.setPCode(tmp.data())
	);
}

std::array<vk::PipelineShaderStageCreateInfo, 2> VkPSO::get_stages(const vk::ShaderModule& vertexModule, const vk::ShaderModule& fragmentModule)
{
	return std::array<vk::PipelineShaderStageCreateInfo, 2> {
		vk::PipelineShaderStageCreateInfo{}
			.setModule(vertexModule)
			.setPName("main")
			.setStage(vk::ShaderStageFlagBits::eVertex),
			vk::PipelineShaderStageCreateInfo{}
			.setModule(fragmentModule)
			.setPName("main")
			.setStage(vk::ShaderStageFlagBits::eFragment)
	};
}

std::array<vk::PipelineColorBlendAttachmentState, 1> VkPSO::to_vk(const REND_MODE& blend_state, const uint8_t& color_mask)
{
	const auto& full_color_output = vk::ColorComponentFlagBits::eA | vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB;
	const auto vk_color_mask = color_mask == 0 ? vk::ColorComponentFlags{} : full_color_output;

	switch (blend_state)
	{
	case REND_ADDITIVE:
	{
		return std::array<vk::PipelineColorBlendAttachmentState, 1>{
			vk::PipelineColorBlendAttachmentState{}
				.setBlendEnable(true)
				.setColorBlendOp(vk::BlendOp::eAdd)
				.setAlphaBlendOp(vk::BlendOp::eAdd)
				.setSrcColorBlendFactor(vk::BlendFactor::eSrcAlpha)
				.setSrcAlphaBlendFactor(vk::BlendFactor::eSrcAlpha)
				.setDstColorBlendFactor(vk::BlendFactor::eOne)
				.setDstAlphaBlendFactor(vk::BlendFactor::eOne)
				.setColorWriteMask(vk_color_mask)
		};
	}
	case REND_ALPHA:
	{
		return std::array<vk::PipelineColorBlendAttachmentState, 1>{
			vk::PipelineColorBlendAttachmentState{}
				.setBlendEnable(true)
				.setColorBlendOp(vk::BlendOp::eAdd)
				.setAlphaBlendOp(vk::BlendOp::eAdd)
				.setSrcColorBlendFactor(vk::BlendFactor::eSrcAlpha)
				.setSrcAlphaBlendFactor(vk::BlendFactor::eSrcAlpha)
				.setDstColorBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha)
				.setDstAlphaBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha)
				.setColorWriteMask(vk_color_mask)
		};
	}
	case REND_PREMULTIPLIED:
	{
		return std::array<vk::PipelineColorBlendAttachmentState, 1>{
			vk::PipelineColorBlendAttachmentState{}
				.setBlendEnable(true)
				.setColorBlendOp(vk::BlendOp::eAdd)
				.setAlphaBlendOp(vk::BlendOp::eAdd)
				.setSrcColorBlendFactor(vk::BlendFactor::eOne)
				.setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
				.setDstColorBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha)
				.setDstAlphaBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha)
				.setColorWriteMask(vk_color_mask)
		};
	}
	case REND_MULTIPLICATIVE:
	{
		return std::array<vk::PipelineColorBlendAttachmentState, 1>{
			vk::PipelineColorBlendAttachmentState{}
				.setBlendEnable(true)
				.setColorBlendOp(vk::BlendOp::eAdd)
				.setAlphaBlendOp(vk::BlendOp::eAdd)
				.setSrcColorBlendFactor(vk::BlendFactor::eOne)
				.setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
				.setDstColorBlendFactor(vk::BlendFactor::eOneMinusSrcColor)
				.setDstAlphaBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha)
				.setColorWriteMask(vk_color_mask)
		};
	}
	case REND_OPAQUE:
	{
		return std::array<vk::PipelineColorBlendAttachmentState, 1>{
			vk::PipelineColorBlendAttachmentState{}
				.setBlendEnable(false)
				.setColorWriteMask(vk_color_mask)
		};
	}
	case REND_TEXT:
	{
		return std::array<vk::PipelineColorBlendAttachmentState, 1>{
			vk::PipelineColorBlendAttachmentState{}
				.setBlendEnable(true)
				.setColorBlendOp(vk::BlendOp::eAdd)
				.setAlphaBlendOp(vk::BlendOp::eAdd)
				.setSrcColorBlendFactor(vk::BlendFactor::eOne)
				.setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
				.setDstColorBlendFactor(vk::BlendFactor::eOneMinusSrcColor)
				.setDstAlphaBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha)
				.setColorWriteMask(vk_color_mask)
		};
	}
	default:
		debug(LOG_FATAL, "Wrong alpha state");
	}
}

vk::PipelineDepthStencilStateCreateInfo VkPSO::to_vk(DEPTH_MODE depth_mode, const gfx_api::stencil_mode& stencil)
{
	auto&& state = vk::PipelineDepthStencilStateCreateInfo{};
	switch (stencil)
	{
	case gfx_api::stencil_mode::stencil_disabled:
		state.setStencilTestEnable(false);
		break;
	case gfx_api::stencil_mode::stencil_shadow_quad:
	{
		state.setStencilTestEnable(true);
		const auto stencil_mode = vk::StencilOpState{}
			.setDepthFailOp(vk::StencilOp::eKeep)
			.setFailOp(vk::StencilOp::eKeep)
			.setPassOp(vk::StencilOp::eKeep)
			.setCompareOp(vk::CompareOp::eLess)
			.setReference(0)
			.setWriteMask(~0)
			.setCompareMask(~0);
		state.setFront(stencil_mode);
		state.setBack(stencil_mode);
		break;
	}
	case gfx_api::stencil_mode::stencil_shadow_silhouette:
	{
		state.setStencilTestEnable(true);
		state.setFront(vk::StencilOpState{}
			.setDepthFailOp(vk::StencilOp::eKeep)
			.setFailOp(vk::StencilOp::eKeep)
			.setPassOp(vk::StencilOp::eIncrementAndWrap)
			.setCompareOp(vk::CompareOp::eAlways)
			.setReference(0)
			.setWriteMask(~0)
			.setCompareMask(~0));
		state.setBack(vk::StencilOpState{}
			.setDepthFailOp(vk::StencilOp::eKeep)
			.setFailOp(vk::StencilOp::eKeep)
			.setPassOp(vk::StencilOp::eDecrementAndWrap)
			.setCompareOp(vk::CompareOp::eAlways)
			.setReference(0)
			.setWriteMask(~0)
			.setCompareMask(~0));
		break;
	}
	}

	switch (depth_mode)
	{
	case DEPTH_CMP_LEQ_WRT_ON:
	{
		return state
			.setDepthTestEnable(true)
			.setDepthWriteEnable(true)
			.setDepthCompareOp(vk::CompareOp::eLessOrEqual);
	}
	case DEPTH_CMP_LEQ_WRT_OFF:
	{
		return state
			.setDepthTestEnable(true)
			.setDepthWriteEnable(false)
			.setDepthCompareOp(vk::CompareOp::eLessOrEqual);
	}
	case DEPTH_CMP_ALWAYS_WRT_OFF:
	{
		return state
			.setDepthTestEnable(false)
			.setDepthWriteEnable(false);
	}
	default:
		debug(LOG_FATAL, "Wrong depth mode");
	}
}

vk::PipelineRasterizationStateCreateInfo VkPSO::to_vk(const bool& offset, const gfx_api::cull_mode& cull)
{
	auto result = vk::PipelineRasterizationStateCreateInfo{}
		.setLineWidth(1.f)
		.setPolygonMode(vk::PolygonMode::eFill);
	if (offset)
	{
		result = result.setDepthBiasEnable(true);
	}
	switch (cull)
	{
	case gfx_api::cull_mode::back:
		result = result.setCullMode(vk::CullModeFlagBits::eBack)
			.setFrontFace(vk::FrontFace::eClockwise);
		break;
	case gfx_api::cull_mode::none:
		result = result.setCullMode(vk::CullModeFlagBits::eNone)
			.setFrontFace(vk::FrontFace::eClockwise);
		break;
	default:
		break;
	}
	return result;
}

vk::Format VkPSO::to_vk(const gfx_api::vertex_attribute_type& type)
{
	switch (type)
	{
	case gfx_api::vertex_attribute_type::float4:
		return vk::Format::eR32G32B32A32Sfloat;
	case gfx_api::vertex_attribute_type::float3:
		return vk::Format::eR32G32B32Sfloat;
	case gfx_api::vertex_attribute_type::float2:
		return vk::Format::eR32G32Sfloat;
	case gfx_api::vertex_attribute_type::u8x4_norm:
		return vk::Format::eR8G8B8A8Unorm;
	}
}

vk::SamplerCreateInfo VkPSO::to_vk(const gfx_api::sampler_type& type)
{
	switch (type)
	{
	case gfx_api::sampler_type::bilinear:
		return vk::SamplerCreateInfo{}
			.setMinFilter(vk::Filter::eLinear)
			.setMagFilter(vk::Filter::eLinear)
			.setMipmapMode(vk::SamplerMipmapMode::eNearest)
			.setMaxAnisotropy(1.f)
			.setMinLod(0.f)
			.setMaxLod(0.f)
			.setAddressModeU(vk::SamplerAddressMode::eClampToEdge)
			.setAddressModeV(vk::SamplerAddressMode::eClampToEdge)
			.setAddressModeW(vk::SamplerAddressMode::eClampToEdge);
	case gfx_api::sampler_type::bilinear_repeat:
		return vk::SamplerCreateInfo{}
			.setMinFilter(vk::Filter::eLinear)
			.setMagFilter(vk::Filter::eLinear)
			.setMipmapMode(vk::SamplerMipmapMode::eNearest)
			.setMaxAnisotropy(1.f)
			.setMinLod(0.f)
			.setMaxLod(0.f)
			.setAddressModeU(vk::SamplerAddressMode::eRepeat)
			.setAddressModeV(vk::SamplerAddressMode::eRepeat)
			.setAddressModeW(vk::SamplerAddressMode::eRepeat);
	case gfx_api::sampler_type::anisotropic:
		return vk::SamplerCreateInfo{}
			.setMinFilter(vk::Filter::eLinear)
			.setMagFilter(vk::Filter::eLinear)
			.setMipmapMode(vk::SamplerMipmapMode::eLinear)
			.setMaxAnisotropy(16.f)
			.setAnisotropyEnable(true)
			.setMinLod(0.f)
			.setMaxLod(10.f)
			.setAddressModeU(vk::SamplerAddressMode::eClampToEdge)
			.setAddressModeV(vk::SamplerAddressMode::eClampToEdge)
			.setAddressModeW(vk::SamplerAddressMode::eClampToEdge);
	case gfx_api::sampler_type::anisotropic_repeat:
		return vk::SamplerCreateInfo{}
			.setMinFilter(vk::Filter::eLinear)
			.setMagFilter(vk::Filter::eLinear)
			.setMipmapMode(vk::SamplerMipmapMode::eLinear)
			.setMaxAnisotropy(16.f)
			.setAnisotropyEnable(true)
			.setMinLod(0.f)
			.setMaxLod(10.f)
			.setAddressModeU(vk::SamplerAddressMode::eRepeat)
			.setAddressModeV(vk::SamplerAddressMode::eRepeat)
			.setAddressModeW(vk::SamplerAddressMode::eRepeat);
	case gfx_api::sampler_type::nearest_clamped:
		return vk::SamplerCreateInfo{}
			.setMinFilter(vk::Filter::eNearest)
			.setMagFilter(vk::Filter::eNearest)
			.setMipmapMode(vk::SamplerMipmapMode::eNearest)
			.setMaxAnisotropy(1.f)
			.setMinLod(0.f)
			.setMaxLod(0.f)
			.setAddressModeU(vk::SamplerAddressMode::eClampToEdge)
			.setAddressModeV(vk::SamplerAddressMode::eClampToEdge)
			.setAddressModeW(vk::SamplerAddressMode::eClampToEdge);
	}
}

vk::PrimitiveTopology VkPSO::to_vk(const gfx_api::primitive_type& primitive)
{
	switch (primitive)
	{
	case gfx_api::primitive_type::lines:
		return vk::PrimitiveTopology::eLineList;
	case gfx_api::primitive_type::triangles:
		return vk::PrimitiveTopology::eTriangleList;
	case gfx_api::primitive_type::triangle_strip:
		return vk::PrimitiveTopology::eTriangleStrip;
	case gfx_api::primitive_type::triangle_fan:
		return vk::PrimitiveTopology::eTriangleFan;
	}
}

VkPSO::VkPSO(vk::Device _dev,
	const gfx_api::state_description &state_desc,
	const SHADER_MODE& shader_mode,
	const gfx_api::primitive_type& primitive,
	const std::vector<gfx_api::texture_input>& texture_desc,
	const std::vector<gfx_api::vertex_buffer>& attribute_descriptions,
	vk::RenderPass rp
	) : dev(_dev)
{
	const auto& cbuffer_layout_desc = std::array<vk::DescriptorSetLayoutBinding, 1>{
		vk::DescriptorSetLayoutBinding{}
			.setBinding(0)
			.setDescriptorCount(1)
			.setDescriptorType(vk::DescriptorType::eUniformBuffer)
			.setStageFlags(vk::ShaderStageFlagBits::eAllGraphics)
	};
	cbuffer_set_layout = dev.createDescriptorSetLayout(
		vk::DescriptorSetLayoutCreateInfo{}
			.setBindingCount(1)
			.setPBindings(cbuffer_layout_desc.data()));

		
	auto&& textures_layout_desc = std::vector<vk::DescriptorSetLayoutBinding>{};
	samplers.reset(new vk::Sampler[texture_desc.size()]);
	for (const auto& texture : texture_desc)
	{
		samplers[texture.id] = dev.createSampler(to_vk(texture.sampler));

		textures_layout_desc.emplace_back(
			vk::DescriptorSetLayoutBinding{}
				.setBinding(texture.id)
				.setDescriptorCount(1)
				.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
				.setPImmutableSamplers(&samplers[texture.id])
				.setStageFlags(vk::ShaderStageFlagBits::eFragment)
		);
	}
	textures_set_layout = dev.createDescriptorSetLayout(
		vk::DescriptorSetLayoutCreateInfo{}
			.setBindingCount(textures_layout_desc.size())
			.setPBindings(textures_layout_desc.data()));

	const auto& layout_desc = std::array<vk::DescriptorSetLayout, 2>{cbuffer_set_layout, textures_set_layout};

	layout = dev.createPipelineLayout(vk::PipelineLayoutCreateInfo{}
		.setPSetLayouts(layout_desc.data())
		.setSetLayoutCount(layout_desc.size()));

	const auto dynamicStates = std::array<vk::DynamicState, 3>{vk::DynamicState::eScissor, vk::DynamicState::eViewport, vk::DynamicState::eDepthBias};
	const auto multisampleState = vk::PipelineMultisampleStateCreateInfo{}
	.setRasterizationSamples(vk::SampleCountFlagBits::e1);
	const auto dynamicS = vk::PipelineDynamicStateCreateInfo{}
		.setDynamicStateCount(3)
		.setPDynamicStates(dynamicStates.data());
	const auto viewportState = vk::PipelineViewportStateCreateInfo{}
		.setViewportCount(1)
		.setScissorCount(1);

	const auto iassembly = vk::PipelineInputAssemblyStateCreateInfo{}
		.setTopology(to_vk(primitive));

	std::size_t buffer_id = 0;
	std::vector<vk::VertexInputBindingDescription> buffers{};
	std::vector<vk::VertexInputAttributeDescription> attributes{};
	for (const auto& buffer : attribute_descriptions)
	{
		buffers.emplace_back(
			vk::VertexInputBindingDescription{}
			.setBinding(buffer_id)
			.setStride(buffer.stride)
			.setInputRate(vk::VertexInputRate::eVertex)
		);
		for (const auto& attribute : buffer.attributes)
		{
			attributes.emplace_back(
				vk::VertexInputAttributeDescription{}
				.setBinding(buffer_id)
				.setFormat(to_vk(attribute.type))
				.setOffset(attribute.offset)
				.setLocation(attribute.id)
			);
		}
		buffer_id++;
	}

	const auto vertex_desc = vk::PipelineVertexInputStateCreateInfo{}
		.setPVertexBindingDescriptions(buffers.data())
		.setVertexBindingDescriptionCount(buffers.size())
		.setPVertexAttributeDescriptions(attributes.data())
		.setVertexAttributeDescriptionCount(attributes.size());

	const auto& color_blend_attachments = to_vk(state_desc.blend_state, state_desc.output_mask);
	const auto color_blend_state = vk::PipelineColorBlendStateCreateInfo{}
		.setAttachmentCount(color_blend_attachments.size())
		.setPAttachments(color_blend_attachments.data());

	const auto pso = vk::GraphicsPipelineCreateInfo{}
		.setPColorBlendState(&color_blend_state)
		.setPDepthStencilState(&to_vk(state_desc.depth_mode, state_desc.stencil))
		.setPRasterizationState(&to_vk(state_desc.offset, state_desc.cull))
		.setPDynamicState(&dynamicS)
		.setPViewportState(&viewportState)
		.setLayout(layout)
		.setPStages(get_stages(get_module(spv_files.at(shader_mode).vertex), get_module(spv_files.at(shader_mode).fragment)).data())
		.setStageCount(2)
		.setSubpass(0)
		.setPInputAssemblyState(&iassembly)
		.setPVertexInputState(&vertex_desc)
		.setPMultisampleState(&multisampleState)
		.setRenderPass(rp);

	object = dev.createGraphicsPipeline(VK_NULL_HANDLE, pso);
}

VkPSO::~VkPSO()
{
	dev.destroyPipeline(object);
	dev.destroyPipelineLayout(layout);
	dev.destroyDescriptorSetLayout(textures_set_layout);
	dev.destroyDescriptorSetLayout(cbuffer_set_layout);
}

VkPSO::VkPSO(vk::Pipeline&& p) : object(p) {}

VkBuf::VkBuf(vk::Device _dev, const gfx_api::buffer::usage&, const std::size_t& width, vk::PhysicalDeviceMemoryProperties memprops) : dev(_dev)
{
	const auto& size = std::max(width, 4u);
	object = dev.createBufferUnique(
		vk::BufferCreateInfo{}
		.setSize(size)
		.setUsage(vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst)
	);
	const auto memreq = dev.getBufferMemoryRequirements(*object);
	memory = dev.allocateMemoryUnique(
		vk::MemoryAllocateInfo{}
		.setAllocationSize(memreq.size)
		.setMemoryTypeIndex(findProperties(memprops, memreq.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal))
	);
	dev.bindBufferMemory(*object, *memory, 0);
}

VkBuf::~VkBuf()
{
	buffering_mechanism::get_current_resources().buffer_to_delete.emplace_back(std::move(object));
	buffering_mechanism::get_current_resources().memory_to_free.emplace_back(std::move(memory));
}

void VkBuf::upload(const std::size_t& start, const std::size_t& width, const void* data)
{
	const auto& size = std::max(width, 4u);
	const auto &scratch_offset = buffering_mechanism::scratchBuffer->alloc(size, 1);
	const auto mappedMem = dev.mapMemory(buffering_mechanism::scratchBuffer->memory, scratch_offset, size);
	memcpy(mappedMem, data, size);
	dev.unmapMemory(buffering_mechanism::scratchBuffer->memory);
	const auto& cmdBuffer = buffering_mechanism::get_current_resources().cmdCopy;
	cmdBuffer.copyBuffer(buffering_mechanism::scratchBuffer->buffer, *object, { vk::BufferCopy(scratch_offset, start, size) });
}

void VkBuf::bind() {}

size_t VkTexture::format_size(const gfx_api::texel_format& format)
{
	return gli::block_size(format);
}

size_t VkTexture::format_size(const vk::Format& format)
{
	switch (format)
	{
	case vk::Format::eR8G8B8Unorm: return 3 * sizeof(uint8_t);
	case vk::Format::eB8G8R8A8Unorm:
	case vk::Format::eR8G8B8A8Unorm: return 4 * sizeof(uint8_t);
	}
	throw;
}

VkTexture::VkTexture(vk::Device _dev, const std::size_t& mipmap_count, const std::size_t& width, const std::size_t& height, const vk::Format& _internal_format, const std::string& filename, const vk::PhysicalDeviceMemoryProperties& memprops)
	: dev(_dev), internal_format(_internal_format)
{
	object = dev.createImageUnique(
		vk::ImageCreateInfo{}
		.setArrayLayers(1)
		.setExtent(vk::Extent3D(width, height, 1))
		.setImageType(vk::ImageType::e2D)
		.setMipLevels(mipmap_count)
		.setTiling(vk::ImageTiling::eOptimal)
		.setFormat(internal_format)
		.setUsage(vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled)
		.setInitialLayout(vk::ImageLayout::eUndefined)
		.setSamples(vk::SampleCountFlagBits::e1)
	);
	const auto& memreq = dev.getImageMemoryRequirements(*object);
	memory = dev.allocateMemoryUnique(
		vk::MemoryAllocateInfo{}
		.setAllocationSize(memreq.size)
		.setMemoryTypeIndex(findProperties(memprops, memreq.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal))
	);
	dev.bindImageMemory(*object, *memory, 0);
	view = dev.createImageViewUnique(
		vk::ImageViewCreateInfo{}
		.setImage(*object)
		.setViewType(vk::ImageViewType::e2D)
		.setFormat(internal_format)
		.setComponents(vk::ComponentMapping())
		.setSubresourceRange(vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, mipmap_count, 0, 1))
	);
}

VkTexture::~VkTexture()
{
	buffering_mechanism::get_current_resources().image_view_to_delete.emplace_back(std::move(view));
	buffering_mechanism::get_current_resources().image_to_delete.emplace_back(std::move(object));
	buffering_mechanism::get_current_resources().memory_to_free.emplace_back(std::move(memory));
}

void VkTexture::bind() {}

void VkTexture::upload(const std::size_t& mip_level, const std::size_t& offset_x, const std::size_t& offset_y, const std::size_t& width, const std::size_t& height, const gfx_api::texel_format& buffer_format, const void* data)
{
	const auto &offset = buffering_mechanism::scratchBuffer->alloc(width * height * format_size(internal_format), 0x4 * format_size(internal_format));
	auto* mappedMem = reinterpret_cast<uint8_t*>(dev.mapMemory(buffering_mechanism::scratchBuffer->memory, offset, width * height * format_size(internal_format)));
	auto* srcMem = reinterpret_cast<const uint8_t*>(data);

	for (unsigned row = 0; row < height; row++)
	{
		for (unsigned col = 0; col < width; col++)
		{
			unsigned byte = 0;
			for (; byte < format_size(buffer_format); byte++)
			{
				const auto& texel = srcMem[(row * width + col) * format_size(buffer_format) + byte];
				mappedMem[(row * width + col) * format_size(internal_format) + byte] = texel;
			}
			for (; byte < format_size(internal_format); byte++)
			{
				mappedMem[(row * width + col) * format_size(internal_format) + byte] = 255;
			}
		}
	}

	dev.unmapMemory(buffering_mechanism::scratchBuffer->memory);
	const auto& cmdBuffer = buffering_mechanism::get_current_resources().cmdCopy;
	cmdBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eBottomOfPipe, vk::PipelineStageFlagBits::eTransfer,
		vk::DependencyFlags{}, {}, {},
		{
			vk::ImageMemoryBarrier{}
				.setImage(*object)
				.setSubresourceRange(vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, mip_level, 1, 0, 1))
				.setOldLayout(vk::ImageLayout::eUndefined)
				.setNewLayout(vk::ImageLayout::eTransferDstOptimal)
				.setDstAccessMask(vk::AccessFlagBits::eTransferWrite)
		});
	cmdBuffer.copyBufferToImage(buffering_mechanism::scratchBuffer->buffer, *object, vk::ImageLayout::eTransferDstOptimal,
	{
		vk::BufferImageCopy{}
			.setBufferOffset(offset)
			.setBufferImageHeight(height)
			.setBufferRowLength(width)
			.setImageOffset(vk::Offset3D(offset_x, offset_y, 0))
			.setImageSubresource(vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, mip_level, 0, 1))
			.setImageExtent(vk::Extent3D(width, height, 1))
	});
	cmdBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader,
		vk::DependencyFlags{}, {}, {},
		{
			vk::ImageMemoryBarrier{}
				.setImage(*object)
				.setSubresourceRange(vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, mip_level, 1, 0, 1))
				.setOldLayout(vk::ImageLayout::eTransferDstOptimal)
				.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite)
				.setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
				.setDstAccessMask(vk::AccessFlagBits::eShaderRead)
		});
}

void VkTexture::generate_mip_levels() {}
unsigned VkTexture::id() { return 0; }

VkRoot::VkRoot(bool _debug) : debugLayer(_debug)
	{
		const auto appInfo = vk::ApplicationInfo()
			.setPApplicationName("Warzone2100")
			.setApplicationVersion(1)
			.setPEngineName("Warzone2100")
			.setEngineVersion(1)
			.setApiVersion(VK_API_VERSION_1_0);

		std::array<const char*, 3> extensions{
			VK_KHR_SURFACE_EXTENSION_NAME,
			VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
			VK_EXT_DEBUG_REPORT_EXTENSION_NAME
		};

		const auto& layers = debugLayer ? std::vector<const char*>{ "VK_LAYER_LUNARG_standard_validation" } : std::vector<const char*>{};

		inst = vk::createInstance(
			vk::InstanceCreateInfo()
			.setPpEnabledLayerNames(layers.data())
			.setEnabledLayerCount(layers.size())
			.setPApplicationInfo(&appInfo)
			.setPpEnabledExtensionNames(extensions.data())
			.setEnabledExtensionCount(extensions.size())
		);

		if (!debugLayer)
			return;
		CreateDebugReportCallback = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(VkInstance(inst), "vkCreateDebugReportCallbackEXT");
		DestroyDebugReportCallback = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(VkInstance(inst), "vkDestroyDebugReportCallbackEXT");
		dbgBreakCallback = (PFN_vkDebugReportMessageEXT)vkGetInstanceProcAddr(VkInstance(inst), "vkDebugReportMessageEXT");

		VkDebugReportCallbackCreateInfoEXT dbgCreateInfo = {};
		dbgCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
		dbgCreateInfo.pfnCallback = (PFN_vkDebugReportCallbackEXT)messageCallback;
		dbgCreateInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT |
			VK_DEBUG_REPORT_WARNING_BIT_EXT;// |
											//VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;

		const VkResult err = CreateDebugReportCallback(
			VkInstance(inst),
			&dbgCreateInfo,
			nullptr,
			&msgCallback);
		assert(!err);
	}

VkRoot::~VkRoot()
{
	gfxqueue.waitIdle();
	buffering_mechanism::destroy(dev);
	DestroyDebugReportCallback(VkInstance(inst), msgCallback, nullptr);

	for (auto f : fbo)
	{
		dev.destroyFramebuffer(f);
	}
	dev.destroyImageView(depthStencilView);
	dev.freeMemory(depthStencilMemory);
	dev.destroyImage(depthStencilImage);
	dev.destroyImageView(default_texture_view);
	dev.freeMemory(default_texture_memory);
	dev.destroyImage(default_texture);
	dev.destroyRenderPass(rp);
	for (auto& imgview : swapchainImageView)
	{
		dev.destroyImageView(imgview);
	}
	dev.destroySwapchainKHR(buffering_mechanism::swapchain);
	dev.destroy();
	inst.destroy();
}

gfx_api::pipeline_state_object * VkRoot::build_pipeline(const gfx_api::state_description &state_desc, const SHADER_MODE& shader_mode, const gfx_api::primitive_type& primitive,
	const std::vector<gfx_api::texture_input>& texture_desc,
	const std::vector<gfx_api::vertex_buffer>& attribute_descriptions)
{
	return new VkPSO(dev, state_desc, shader_mode, primitive, texture_desc, attribute_descriptions, rp);
}

void VkRoot::createDefaultRenderpass(vk::Format swapchainFormat)
{
	const auto& attachments =
		std::array<vk::AttachmentDescription, 2>{
		vk::AttachmentDescription{}
			.setFormat(swapchainFormat)
			.setInitialLayout(vk::ImageLayout::eColorAttachmentOptimal)
			.setFinalLayout(vk::ImageLayout::eColorAttachmentOptimal)
			.setLoadOp(vk::AttachmentLoadOp::eClear)
			.setStencilStoreOp(vk::AttachmentStoreOp::eStore),
			vk::AttachmentDescription{}
			.setFormat(vk::Format::eD32SfloatS8Uint)
			.setInitialLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
			.setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
			.setLoadOp(vk::AttachmentLoadOp::eClear)
			.setStencilLoadOp(vk::AttachmentLoadOp::eClear)
			.setStoreOp(vk::AttachmentStoreOp::eDontCare)
			.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
	};
	const auto& colorAttachmentRef =
		std::array<vk::AttachmentReference, 1>{
		vk::AttachmentReference{}
			.setAttachment(0)
			.setLayout(vk::ImageLayout::eColorAttachmentOptimal)
	};
	const auto depthStencilAttachmentRef =
		vk::AttachmentReference{}
		.setAttachment(1)
		.setLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

	const auto& subpasses =
		std::array<vk::SubpassDescription, 1> {
		vk::SubpassDescription{}
			.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
			.setColorAttachmentCount(colorAttachmentRef.size())
			.setPColorAttachments(colorAttachmentRef.data())
			.setPDepthStencilAttachment(&depthStencilAttachmentRef)
	};
	rp = dev.createRenderPass(
		vk::RenderPassCreateInfo{}
		.setAttachmentCount(attachments.size())
		.setPAttachments(attachments.data())
		.setPSubpasses(subpasses.data())
		.setSubpassCount(subpasses.size()));
}


void VkRoot::setSwapchain(SDL_Window* window)
{
	VkSurfaceKHR _surface{};
	SDL_Vulkan_CreateSurface(window, VkInstance(inst), &_surface);
	const auto& surface = vk::SurfaceKHR{ _surface };
	const auto& physicalDevices = inst.enumeratePhysicalDevices();
	const auto& tmp = physicalDevices[0].getProperties();
	const auto& queuesFamilies = physicalDevices[0].getQueueFamilyProperties();
	const auto& findQueue = [&]() {
		for (auto&& i = 0; i < queuesFamilies.size(); ++i)
		{
			const auto& isGraphic = !!(queuesFamilies[i].queueFlags & vk::QueueFlagBits::eGraphics);
			if (!isGraphic)
				continue;
			if (!!physicalDevices[0].getSurfaceSupportKHR(i, surface))
				return i;
		}
		return -1;
	};
	const auto& graphicsQueueIndex = findQueue();
	const auto& queuePriorities = std::array<float, 1>{ 0.0f };
	const auto& queueCreateInfo =
		std::array<vk::DeviceQueueCreateInfo, 1>{
		vk::DeviceQueueCreateInfo{}
			.setPQueuePriorities(queuePriorities.data())
			.setQueueCount(1)
			.setQueueFamilyIndex(graphicsQueueIndex)
	};
	const auto& layers = debugLayer ? std::vector<const char*>{ "VK_LAYER_LUNARG_standard_validation" } : std::vector<const char*>{};
	const auto& enabledExtensions = std::vector<const char*>{ VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	const auto enabledFeatures = vk::PhysicalDeviceFeatures{}.setSamplerAnisotropy(true);
	dev = physicalDevices[0].createDevice(
		vk::DeviceCreateInfo{}
		.setEnabledLayerCount(layers.size())
		.setPpEnabledLayerNames(layers.data())
		.setEnabledExtensionCount(enabledExtensions.size())
		.setPpEnabledExtensionNames(enabledExtensions.data())
		.setPEnabledFeatures(&enabledFeatures)
		.setQueueCreateInfoCount(queueCreateInfo.size())
		.setPQueueCreateInfos(queueCreateInfo.data())
	);

	const auto& surfCap = physicalDevices[0].getSurfaceCapabilitiesKHR(surface);
	const auto& surfFormat = physicalDevices[0].getSurfaceFormatsKHR(surface);
	const auto& surfMode = physicalDevices[0].getSurfacePresentModesKHR(surface);

	buffering_mechanism::swapchain = dev.createSwapchainKHR(
		vk::SwapchainCreateInfoKHR{}
		.setSurface(surface)
		.setMinImageCount(2)
		.setPresentMode(surfMode[0])
		.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
		.setImageArrayLayers(1)
		.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
		.setClipped(true)
		.setImageExtent(surfCap.currentExtent)
		.setImageFormat(surfFormat[0].format)
		.setImageSharingMode(vk::SharingMode::eExclusive)
	);
	swapchainFormat = surfFormat[0].format;
	swapChainWidth = surfCap.currentExtent.width;
	swapChainHeight = surfCap.currentExtent.height;

	const auto& swapchainImages = dev.getSwapchainImagesKHR(buffering_mechanism::swapchain);
	std::transform(swapchainImages.begin(), swapchainImages.end(), std::back_inserter(swapchainImageView),
		[&](auto&& img) {
		return dev.createImageView(
			vk::ImageViewCreateInfo{}
			.setImage(img)
			.setFormat(swapchainFormat)
			.setViewType(vk::ImageViewType::e2D)
			.setComponents(vk::ComponentMapping())
			.setSubresourceRange(vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1))
		);
	});

	gfxqueue = dev.getQueue(graphicsQueueIndex, 0);
	std::transform(swapchainImages.begin(), swapchainImages.end(), std::back_inserter(buffering_mechanism::perFrameResources),
		[&](auto&& img) {
			return std::make_unique<perFrameResources_t>(dev, img, graphicsQueueIndex);
	});

	memprops = physicalDevices[0].getMemoryProperties();
	const auto& formatSupport = physicalDevices[0].getFormatProperties(vk::Format::eR8G8B8Unorm);
	supports_rgb = bool(formatSupport.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImage);
	depthStencilImage = dev.createImage(
		vk::ImageCreateInfo{}
		.setFormat(vk::Format::eD32SfloatS8Uint)
		.setArrayLayers(1)
		.setExtent(vk::Extent3D(swapChainWidth, swapChainHeight, 1))
		.setImageType(vk::ImageType::e2D)
		.setMipLevels(1)
		.setSamples(vk::SampleCountFlagBits::e1)
		.setTiling(vk::ImageTiling::eOptimal)
		.setUsage(vk::ImageUsageFlagBits::eDepthStencilAttachment)
	);

	const auto& memreq = dev.getImageMemoryRequirements(depthStencilImage);
	depthStencilMemory = dev.allocateMemory(
		vk::MemoryAllocateInfo{}
		.setAllocationSize(memreq.size)
		.setMemoryTypeIndex(findProperties(memprops, memreq.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal))
	);
	dev.bindImageMemory(depthStencilImage, depthStencilMemory, 0);

	depthStencilView = dev.createImageView(
		vk::ImageViewCreateInfo{}
		.setFormat(vk::Format::eD32SfloatS8Uint)
		.setImage(depthStencilImage)
		.setViewType(vk::ImageViewType::e2D)
		.setComponents(vk::ComponentMapping())
		.setSubresourceRange(vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil, 0, 1, 0, 1))
	);
	setupSwapchainImages();
	buffering_mechanism::scratchBuffer = std::make_unique<circularHostBuffer>(dev, memprops, 1024 * 1024 * 128);

	createDefaultRenderpass(swapchainFormat);
	std::transform(swapchainImageView.begin(), swapchainImageView.end(), std::back_inserter(fbo),
		[&](auto&& imageView) {
		const auto &attachments = std::array<vk::ImageView, 2>{imageView, depthStencilView};
		return dev.createFramebuffer(
			vk::FramebufferCreateInfo{}
			.setAttachmentCount(attachments.size())
			.setPAttachments(attachments.data())
			.setLayers(1)
			.setWidth(swapChainWidth)
			.setHeight(swapChainHeight)
			.setRenderPass(rp)
		);
	});

	default_texture = dev.createImage(vk::ImageCreateInfo{}
		.setArrayLayers(1)
		.setExtent(vk::Extent3D(2, 2, 1))
		.setImageType(vk::ImageType::e2D)
		.setMipLevels(1)
		.setTiling(vk::ImageTiling::eOptimal)
		.setFormat(vk::Format::eR8G8B8A8Srgb)
		.setUsage(vk::ImageUsageFlagBits::eSampled)
		.setInitialLayout(vk::ImageLayout::ePreinitialized)
		.setSamples(vk::SampleCountFlagBits::e1));
	const auto& memreq2 = dev.getImageMemoryRequirements(default_texture);
	default_texture_memory = dev.allocateMemory(
		vk::MemoryAllocateInfo{}
		.setAllocationSize(memreq2.size)
		.setMemoryTypeIndex(findProperties(memprops, memreq2.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal))
	);
	dev.bindImageMemory(default_texture, default_texture_memory, 0);
	default_texture_view = dev.createImageView(vk::ImageViewCreateInfo{}
		.setImage(default_texture)
		.setViewType(vk::ImageViewType::e2D)
		.setFormat(vk::Format::eR8G8B8A8Srgb)
		.setComponents(vk::ComponentMapping(vk::ComponentSwizzle::eZero, vk::ComponentSwizzle::eZero, vk::ComponentSwizzle::eZero, vk::ComponentSwizzle::eZero))
		.setSubresourceRange(vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)));

	buffering_mechanism::init(dev);
	startRenderPass();
}

void VkRoot::draw(const std::size_t& offset, const std::size_t& count, const gfx_api::primitive_type&)
{
	buffering_mechanism::get_current_resources().cmdDraw.draw(count, 1, offset, 0);
}

void VkRoot::draw_elements(const std::size_t& offset, const std::size_t& count, const gfx_api::primitive_type&, const gfx_api::index_type&)
{
	buffering_mechanism::get_current_resources().cmdDraw.drawIndexed(count, 1, offset >> 2, 0, 0);
}

void VkRoot::bind_vertex_buffers(const std::size_t& first, const std::vector<std::tuple<gfx_api::buffer*, std::size_t>>& vertex_buffers_offset)
{
	auto&& temp = std::vector<vk::Buffer>{};
	std::transform(vertex_buffers_offset.begin(), vertex_buffers_offset.end(), std::back_inserter(temp),
		[](auto&& input) { return *static_cast<const VkBuf *>(std::get<0>(input))->object; });
	auto&& offsets = std::vector<VkDeviceSize>{};
	std::transform(vertex_buffers_offset.begin(), vertex_buffers_offset.end(), std::back_inserter(offsets),
		[](auto&& input) { return std::get<1>(input); });
	buffering_mechanism::get_current_resources().cmdDraw.bindVertexBuffers(first, temp, offsets);
}

void VkRoot::bind_streamed_vertex_buffers(const void* data, const std::size_t size)
{
	const auto& offset = buffering_mechanism::scratchBuffer->alloc(size, 16);
	const auto mappedPtr = dev.mapMemory(buffering_mechanism::scratchBuffer->memory, offset, size);
	memcpy(mappedPtr, data, size);
	dev.unmapMemory(buffering_mechanism::scratchBuffer->memory);
	buffering_mechanism::get_current_resources().cmdDraw.bindVertexBuffers(0, { buffering_mechanism::scratchBuffer->buffer }, { offset });
}

void VkRoot::setupSwapchainImages()
{
	const auto& buffers = dev.allocateCommandBuffers(
		vk::CommandBufferAllocateInfo{}
		.setCommandPool(buffering_mechanism::perFrameResources[0]->pool)
		.setCommandBufferCount(1)
		.setLevel(vk::CommandBufferLevel::ePrimary)
	);
	internalCommandBuffer = buffers[0];
	internalCommandBuffer.begin(
		vk::CommandBufferBeginInfo{}
		.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit)
	);
	internalCommandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands, vk::PipelineStageFlagBits::eAllCommands,
		vk::DependencyFlagBits{}, {}, {},
		{
			vk::ImageMemoryBarrier{}
			.setOldLayout(vk::ImageLayout::eUndefined)
			.setNewLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
			.setImage(depthStencilImage)
			.setSubresourceRange(vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil, 0, 1, 0, 1))
			.setDstAccessMask(vk::AccessFlagBits::eDepthStencilAttachmentWrite)
		});

	for (const auto& tmp : buffering_mechanism::perFrameResources)
	{
		const auto img = tmp->swapchainImages;
		internalCommandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands, vk::PipelineStageFlagBits::eAllCommands,
			vk::DependencyFlagBits{}, {}, {},
			{
				vk::ImageMemoryBarrier{}
				.setOldLayout(vk::ImageLayout::eUndefined)
			.setNewLayout(vk::ImageLayout::ePresentSrcKHR)
			.setImage(img)
			.setSubresourceRange(vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1))
			.setDstAccessMask(vk::AccessFlagBits::eMemoryRead)
			});
	}
	internalCommandBuffer.end();
	gfxqueue.submit(
		vk::SubmitInfo{}
		.setCommandBufferCount(1)
		.setPCommandBuffers(&internalCommandBuffer),
		vk::Fence());
}

vk::Format VkRoot::get_format(const gfx_api::texel_format& format)
{
	switch (format)
	{
	case gfx_api::texel_format::FORMAT_RGB8_UNORM_PACK8:
		return supports_rgb ? vk::Format::eR8G8B8Unorm : vk::Format::eR8G8B8A8Unorm;
	case gfx_api::texel_format::FORMAT_RGBA8_UNORM_PACK8:
		return vk::Format::eR8G8B8A8Unorm;
	case gfx_api::texel_format::FORMAT_BGRA8_UNORM_PACK8:
		return vk::Format::eB8G8R8A8Unorm;
	}
	throw;
}

gfx_api::texture* VkRoot::create_texture(const std::size_t& mipmap_count, const std::size_t& width, const std::size_t& height, const gfx_api::texel_format& internal_format, const std::string& filename)
{
	auto result = new VkTexture(dev, mipmap_count, width, height, get_format(internal_format), filename, memprops);
	return result;
}

gfx_api::buffer* VkRoot::create_buffer(const gfx_api::buffer::usage& usage, const std::size_t& width)
{
	return new VkBuf(dev, usage, width, memprops);
}

auto VkRoot::allocateDescriptorSets(vk::DescriptorSetLayout arg)
{
	const auto& descriptorSet = std::array<vk::DescriptorSetLayout, 1>{ arg };
	buffering_mechanism::get_current_resources().numalloc++;
	return dev.allocateDescriptorSets(
		vk::DescriptorSetAllocateInfo{}
			.setDescriptorPool(buffering_mechanism::get_current_resources().descriptorPool)
			.setPSetLayouts(descriptorSet.data())
			.setDescriptorSetCount(descriptorSet.size())
	);
}

vk::IndexType VkRoot::to_vk(const gfx_api::index_type& index)
{
	switch (index)
	{
	case gfx_api::index_type::u16:
		return vk::IndexType::eUint16;
	case gfx_api::index_type::u32:
		return vk::IndexType::eUint32;
	}
}

void VkRoot::bind_index_buffer(gfx_api::buffer& index_buffer, const gfx_api::index_type& index)
{
	auto& casted_buf = static_cast<VkBuf&>(index_buffer);
	buffering_mechanism::get_current_resources().cmdDraw.bindIndexBuffer(*casted_buf.object, 0, to_vk(index));
}

void VkRoot::bind_textures(const std::vector<gfx_api::texture_input>& attribute_descriptions, const std::vector<gfx_api::texture*>& textures)
{
	const auto& set = allocateDescriptorSets(currentPSO->textures_set_layout);

	auto&& image_descriptor = std::vector<vk::DescriptorImageInfo>{};
	for (auto* texture : textures)
	{
		image_descriptor.emplace_back(vk::DescriptorImageInfo{}
			.setImageView(texture != nullptr ? *static_cast<VkTexture*>(texture)->view : default_texture_view)
			.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal));
	}
	std::size_t i = 0;
	auto&& write_info = std::vector<vk::WriteDescriptorSet>{};
	for (auto* texture : textures)
	{
		write_info.emplace_back(
			vk::WriteDescriptorSet{}
				.setDescriptorCount(1)
				.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
				.setDstSet(set[0])
				.setPImageInfo(&image_descriptor[i])
				.setDstBinding(i)
		);
		i++;
	}
	dev.updateDescriptorSets(write_info, {});
	buffering_mechanism::get_current_resources().cmdDraw.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, currentPSO->layout, 1, set, {});
}

vk::DescriptorBufferInfo VkRoot::uploadOnScratchBuffer(const void* data, const std::size_t& size)
{
	const auto& offset = buffering_mechanism::scratchBuffer->alloc(size, 0x100);
	const auto mappedPtr = dev.mapMemory(buffering_mechanism::scratchBuffer->memory, offset, size);
	memcpy(mappedPtr, data, size);
	dev.unmapMemory(buffering_mechanism::scratchBuffer->memory);
	return vk::DescriptorBufferInfo(buffering_mechanism::scratchBuffer->buffer, offset, size);
};

void VkRoot::set_constants(const void* buffer, const std::size_t& size)
{
	const auto& sets = allocateDescriptorSets(currentPSO->cbuffer_set_layout);
	const auto& bufferInfo = uploadOnScratchBuffer(buffer, size);
	const auto& descriptorWrite = std::array<vk::WriteDescriptorSet, 1>{
		vk::WriteDescriptorSet{}
			.setDescriptorCount(1)
			.setDescriptorType(vk::DescriptorType::eUniformBuffer)
			.setDstBinding(0)
			.setPBufferInfo(&bufferInfo)
			.setDstSet(sets[0])
	};
	dev.updateDescriptorSets(descriptorWrite, {});
	buffering_mechanism::get_current_resources().cmdDraw.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, currentPSO->layout, 0,
		sets, {});
}

void VkRoot::bind_pipeline(gfx_api::pipeline_state_object* pso)
{
	currentPSO = static_cast<VkPSO*>(pso);
	buffering_mechanism::get_current_resources().cmdDraw.bindPipeline(vk::PipelineBindPoint::eGraphics, currentPSO->object);
}

void VkRoot::flip()
{
	buffering_mechanism::get_current_resources().cmdDraw.endRenderPass();
	buffering_mechanism::get_current_resources().cmdDraw.pipelineBarrier(
		vk::PipelineStageFlagBits::eAllCommands,
		vk::PipelineStageFlagBits::eAllCommands,
		vk::DependencyFlagBits{}, {}, {},
		{
			vk::ImageMemoryBarrier{}
			.setOldLayout(vk::ImageLayout::eColorAttachmentOptimal)
		.setNewLayout(vk::ImageLayout::ePresentSrcKHR)
		.setImage(buffering_mechanism::get_current_resources().swapchainImages)
		.setSubresourceRange(vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1))
		.setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentWrite)
		.setDstAccessMask(vk::AccessFlagBits::eMemoryRead)
		});
	buffering_mechanism::get_current_resources().cmdDraw.end();
	buffering_mechanism::get_current_resources().cmdCopy.end();
	const auto& executableCmdBuffer = std::array<vk::CommandBuffer, 2>{buffering_mechanism::get_current_resources().cmdCopy, buffering_mechanism::get_current_resources().cmdDraw}; // copy before render
	const vk::PipelineStageFlags& waitStage = vk::PipelineStageFlagBits::eAllCommands;
	gfxqueue.submit(
		vk::SubmitInfo{}
		.setWaitSemaphoreCount(1)
		.setPWaitSemaphores(&buffering_mechanism::acquireSemaphore)
		.setPWaitDstStageMask(&waitStage)
		.setCommandBufferCount(executableCmdBuffer.size())
		.setPCommandBuffers(executableCmdBuffer.data()),
		buffering_mechanism::get_current_resources().previousSubmission);
	gfxqueue.presentKHR(
		vk::PresentInfoKHR{}
		.setPSwapchains(&buffering_mechanism::swapchain)
		.setSwapchainCount(1)
		.setPImageIndices(&buffering_mechanism::currentSwapchainIndex)
	);
	buffering_mechanism::swap(dev);
	startRenderPass();
	buffering_mechanism::get_current_resources().cmdCopy.begin(vk::CommandBufferBeginInfo{}.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
}

void VkRoot::startRenderPass()
{
	buffering_mechanism::get_current_resources().cmdDraw.begin(vk::CommandBufferBeginInfo{}.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
	buffering_mechanism::get_current_resources().cmdDraw.pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands, vk::PipelineStageFlagBits::eAllCommands,
		vk::DependencyFlagBits{}, {}, {},
		{
			vk::ImageMemoryBarrier{}
			.setImage(buffering_mechanism::get_current_resources().swapchainImages)
		.setSrcAccessMask(vk::AccessFlagBits::eMemoryRead)
		.setOldLayout(vk::ImageLayout::ePresentSrcKHR)
		.setNewLayout(vk::ImageLayout::eColorAttachmentOptimal)
		.setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite)
		.setSubresourceRange(vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1))
		});
	const auto& clearValue = std::array<vk::ClearValue, 2> {
		vk::ClearValue(), vk::ClearValue(vk::ClearDepthStencilValue(1.f, 0u))
	};
	buffering_mechanism::get_current_resources().cmdDraw.beginRenderPass(
		vk::RenderPassBeginInfo{}
		.setFramebuffer(fbo[buffering_mechanism::currentSwapchainIndex])
		.setClearValueCount(clearValue.size())
		.setPClearValues(clearValue.data())
		.setRenderPass(rp)
		.setRenderArea(vk::Rect2D(vk::Offset2D(), vk::Extent2D(swapChainWidth, swapChainHeight))),
		vk::SubpassContents::eInline);
	buffering_mechanism::get_current_resources().cmdDraw.setViewport(0, { vk::Viewport{}.setHeight(swapChainHeight).setWidth(swapChainWidth).setMinDepth(0.f).setMaxDepth(1.f) });
	buffering_mechanism::get_current_resources().cmdDraw.setScissor(0, { vk::Rect2D{}.setExtent(vk::Extent2D{}.setHeight(swapChainHeight).setWidth(swapChainWidth)) });
}

void VkRoot::set_polygon_offset(const float& offset, const float& slope)
{
	buffering_mechanism::get_current_resources().cmdDraw.setDepthBias(offset, 1.0f, slope);
}

void VkRoot::set_depth_range(const float& min, const float& max)
{
	buffering_mechanism::get_current_resources().cmdDraw.setViewport(0, { vk::Viewport{}.setHeight(swapChainHeight).setWidth(swapChainWidth).setMinDepth(min).setMaxDepth(max) });
}
