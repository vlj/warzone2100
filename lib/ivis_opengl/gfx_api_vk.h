#define VK_USE_PLATFORM_WIN32_KHR
#include "gfx_api.h"
#include <algorithm>
#include <sstream>
#include <map>
#include <physfs.h>
#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan.hpp>


struct circularHostBuffer
{
	vk::Buffer buffer;
	vk::DeviceMemory memory;
	vk::Device &dev;
	uint32_t gpuReadLocation;
	uint32_t hostWriteLocation;
	uint32_t size;

	circularHostBuffer(vk::Device &d, vk::PhysicalDeviceMemoryProperties memprops, uint32_t s);
	~circularHostBuffer();

private:
	static bool isBetween(uint32_t rangeBegin, uint32_t rangeEnd, uint32_t position);
	static std::tuple<uint32_t, uint32_t> getWritePosAndNewWriteLocation(uint32_t currentWritePos, uint32_t amount, uint32_t totalSize, uint32_t align);
public:
	uint32_t alloc(uint32_t amount, uint32_t align);
};

struct perFrameResources_t
{
	vk::Device dev;
	vk::DescriptorPool descriptorPool;
	uint32_t numalloc;
	vk::CommandPool pool;
	vk::CommandBuffer cmdDraw;
	vk::CommandBuffer cmdCopy;
	vk::Fence previousSubmission;
	vk::Image swapchainImages;
	std::vector<vk::UniqueBuffer> buffer_to_delete;
	std::vector<vk::UniqueImage> image_to_delete;
	std::vector<vk::UniqueImageView> image_view_to_delete;
	std::vector<vk::UniqueDeviceMemory> memory_to_free;

	perFrameResources_t(vk::Device& _dev, vk::Image _swapchainImage, const uint32_t& graphicsQueueIndex);
	void clean();
	~perFrameResources_t();
};

struct buffering_mechanism
{
	static std::unique_ptr<circularHostBuffer> scratchBuffer;
	static std::vector<std::unique_ptr<perFrameResources_t>> perFrameResources;
	static uint32_t currentSwapchainIndex;
	static vk::Semaphore acquireSemaphore;
	static vk::SwapchainKHR swapchain;

	static perFrameResources_t& get_current_resources();
	static void init(vk::Device dev);
	static void destroy(vk::Device dev);
	static void swap(vk::Device dev);
private:
	static void acquireSwapchainIndex(vk::Device dev);
};

VkBool32 messageCallback(
	VkDebugReportFlagsEXT flags,
	VkDebugReportObjectTypeEXT objType,
	uint64_t srcObject,
	std::size_t location,
	int32_t msgCode,
	const char* pLayerPrefix,
	const char* pMsg,
	void* pUserData);

struct VkPSO final : public gfx_api::pipeline_state_object
{
	vk::Pipeline object;
	vk::DescriptorSetLayout cbuffer_set_layout;
	vk::DescriptorSetLayout textures_set_layout;
	vk::PipelineLayout layout;
	vk::Device dev;
	std::unique_ptr<vk::Sampler[]> samplers;

private:
	// Read shader into text buffer
	static std::vector<uint32_t> readShaderBuf(const std::string& name);

	vk::ShaderModule get_module(const std::string& name);

	static std::array<vk::PipelineShaderStageCreateInfo, 2> get_stages(const vk::ShaderModule& vertexModule, const vk::ShaderModule& fragmentModule);

	static std::array<vk::PipelineColorBlendAttachmentState, 1> to_vk(const REND_MODE& blend_state, const uint8_t& color_mask);

	static vk::PipelineDepthStencilStateCreateInfo to_vk(DEPTH_MODE depth_mode, const gfx_api::stencil_mode& stencil);

	static vk::PipelineRasterizationStateCreateInfo to_vk(const bool& offset, const gfx_api::cull_mode& cull);

	static vk::Format to_vk(const gfx_api::vertex_attribute_type& type);

	static vk::SamplerCreateInfo to_vk(const gfx_api::sampler_type& type);

	static vk::PrimitiveTopology to_vk(const gfx_api::primitive_type& primitive);

public:
	VkPSO(vk::Device _dev,
		const gfx_api::state_description &state_desc,
		const SHADER_MODE& shader_mode,
		const gfx_api::primitive_type& primitive,
		const std::vector<gfx_api::texture_input>& texture_desc,
		const std::vector<gfx_api::vertex_buffer>& attribute_descriptions,
		vk::RenderPass rp
	);

	~VkPSO();

	VkPSO(vk::Pipeline&& p);
	VkPSO(const VkPSO&) = default;
	VkPSO(VkPSO&&) = default;
	VkPSO& operator=(VkPSO&&) = default;

};

struct VkBuf final : public gfx_api::buffer
{
	vk::Device dev;
	vk::UniqueBuffer object;
	vk::UniqueDeviceMemory memory;

	VkBuf(vk::Device _dev, const gfx_api::buffer::usage&, const std::size_t& width, vk::PhysicalDeviceMemoryProperties memprops);

	virtual ~VkBuf() override;

	virtual void upload(const std::size_t& start, const std::size_t& width, const void* data) override;

	virtual void bind() override;
};

struct VkTexture final : public gfx_api::texture
{
	vk::Device dev;
	vk::UniqueImage object;
	vk::UniqueImageView view;
	vk::UniqueDeviceMemory memory;
	vk::Format internal_format;

	static size_t format_size(const gfx_api::texel_format& format);

	static size_t format_size(const vk::Format& format);

	VkTexture(vk::Device _dev, const std::size_t& mipmap_count, const std::size_t& width, const std::size_t& height, const vk::Format& _internal_format, const std::string& filename, const vk::PhysicalDeviceMemoryProperties& memprops);

	virtual ~VkTexture() override;

	virtual void bind() override;

	virtual void upload(const std::size_t& mip_level, const std::size_t& offset_x, const std::size_t& offset_y, const std::size_t& width, const std::size_t& height, const gfx_api::texel_format& buffer_format, const void* data) override;
	virtual void generate_mip_levels() override;
	virtual unsigned id() override;
};

struct VkRoot final : gfx_api::context
{
	vk::Instance inst;
	vk::Device dev;
	vk::SurfaceKHR surface;
	vk::Queue gfxqueue;

	vk::RenderPass rp;
	std::vector<vk::Framebuffer> fbo;

	vk::Image depthStencilImage;
	vk::DeviceMemory depthStencilMemory;
	vk::ImageView depthStencilView;
	std::vector<vk::ImageView> swapchainImageView;
	vk::PhysicalDeviceMemoryProperties memprops;
	bool supports_rgb;

	vk::Image default_texture;
	vk::ImageView default_texture_view;
	vk::DeviceMemory default_texture_memory;

	vk::CommandBuffer internalCommandBuffer;

	vk::Format swapchainFormat;
	uint32_t swapChainWidth;
	uint32_t swapChainHeight;

	PFN_vkCreateDebugReportCallbackEXT CreateDebugReportCallback = nullptr;
	PFN_vkDestroyDebugReportCallbackEXT DestroyDebugReportCallback = nullptr;
	PFN_vkDebugReportMessageEXT dbgBreakCallback = nullptr;
	VkDebugReportCallbackEXT msgCallback = 0;

	VkPSO* currentPSO = nullptr;

	bool debugLayer = false;

	VkRoot(bool _debug);
	~VkRoot();

	virtual gfx_api::pipeline_state_object * build_pipeline(const gfx_api::state_description &state_desc, const SHADER_MODE& shader_mode, const gfx_api::primitive_type& primitive,
		const std::vector<gfx_api::texture_input>& texture_desc,
		const std::vector<gfx_api::vertex_buffer>& attribute_descriptions) override;

	void createDefaultRenderpass(vk::Format swapchainFormat);
	virtual void setSwapchain(SDL_Window* window) override;
	virtual void draw(const std::size_t& offset, const std::size_t& count, const gfx_api::primitive_type&) override;
	virtual void draw_elements(const std::size_t& offset, const std::size_t& count, const gfx_api::primitive_type&, const gfx_api::index_type&) override;
	virtual void bind_vertex_buffers(const std::size_t& first, const std::vector<std::tuple<gfx_api::buffer*, std::size_t>>& vertex_buffers_offset) override;
	virtual void bind_streamed_vertex_buffers(const void* data, const std::size_t size) override;
	void setupSwapchainImages();
	vk::Format get_format(const gfx_api::texel_format& format);

private:
	virtual gfx_api::texture* create_texture(const std::size_t& mipmap_count, const std::size_t& width, const std::size_t& height, const gfx_api::texel_format& internal_format, const std::string& filename = "") override;

	virtual gfx_api::buffer* create_buffer(const gfx_api::buffer::usage& usage, const std::size_t& width) override;

	auto allocateDescriptorSets(vk::DescriptorSetLayout arg);

private:
	static vk::IndexType to_vk(const gfx_api::index_type& index);

public:
	virtual void bind_index_buffer(gfx_api::buffer& index_buffer, const gfx_api::index_type& index) override;

	virtual void bind_textures(const std::vector<gfx_api::texture_input>& attribute_descriptions, const std::vector<gfx_api::texture*>& textures) override;

private:
	vk::DescriptorBufferInfo uploadOnScratchBuffer(const void* data, const std::size_t& size);
public:
	virtual void set_constants(const void* buffer, const std::size_t& size) override;

	virtual void bind_pipeline(gfx_api::pipeline_state_object* pso) override;

	virtual void flip() override;
	virtual void set_polygon_offset(const float& offset, const float& slope) override;
	virtual void set_depth_range(const float& min, const float& max) override;
private:
	void startRenderPass();
};
