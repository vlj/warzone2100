#include "gfx_api_gl.h"
#include "gfx_api_vk.h"

bool uses_vulkan = false;
bool uses_gfx_debug = false;

gfx_api::context& gfx_api::context::get()
{
	if (uses_vulkan)
	{
		static VkRoot ctx(uses_gfx_debug);
		return ctx;
	}
	else
	{
		static gl_context ctx(uses_gfx_debug);
		return ctx;
	}
}
