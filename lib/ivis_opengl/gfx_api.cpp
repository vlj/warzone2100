#include "gfx_api_gl.h"
#include "gfx_api_vk.h"

bool uses_vulkan = false;

gfx_api::context& gfx_api::context::get()
{
	if (uses_vulkan)
	{
		static VkRoot ctx;
		return ctx;
	}
	else
	{
		static gl_context ctx;
		return ctx;
	}
}
