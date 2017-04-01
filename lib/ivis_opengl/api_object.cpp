#include "api_object.h"
#include "gl/gl_impl.h"

gfx_api::context& gfx_api::context::get_context()
{
	static gl_api::gl_context object;
	return object;
}


