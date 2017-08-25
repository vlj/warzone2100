cmake_minimum_required (VERSION 3.5)

file(GLOB HEADERS
	"bitimage.h"
	"gfx_api.h"
	"imd.h"
	"ivisdef.h"
	"jpeg_encoder.h"
	"pieblitfunc.h"
	"pieclip.h"
	"piedef.h"
	"piefunc.h"
	"piematrix.h"
	"piemode.h"
	"piepalette.h"
	"piestate.h"
	"pietypes.h"
	"png_util.h"
	"screen.h"
	"tex.h"
	"textdraw.h"
)

file(GLOB SRC
	"bitimage.cpp"
	"gfx_api_gl.cpp"
	"gfx_api_vk.cpp"
	"gfx_api.cpp"
	"imdload.cpp"
	"jpeg_encoder.cpp"
	"pieblitfunc.cpp"
	"pieclip.cpp"
	"piedraw.cpp"
	"piefunc.cpp"
	"piematrix.cpp"
	"piemode.cpp"
	"piepalette.cpp"
	"piestate.cpp"
	"png_util.cpp"
	"screen.cpp"
	"tex.cpp"
	"textdraw.cpp"
)

find_package(PNG REQUIRED)
find_package(OpenGL REQUIRED)
find_package(Vulkan REQUIRED)
find_package(GLEW REQUIRED)
find_package(Freetype REQUIRED)
find_package(SDL2 REQUIRED)

if("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
  find_package(PkgConfig)
  pkg_check_modules(HARFBUZZ harfbuzz)
else()
  set(HARFBUZZ_INCLUDE_DIRS ${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include/harfbuzz)
  set(HARFBUZZ_LIBRARIES ${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib/harfbuzz.lib)
endif()

find_program (GLSLC "glslc")

file(GLOB SHADERS
	"${CMAKE_SOURCE_DIR}/data/base/shaders/vk/generic.vert"
	"${CMAKE_SOURCE_DIR}/data/base/shaders/vk/rect.vert"
	"${CMAKE_SOURCE_DIR}/data/base/shaders/vk/gfx.vert"
	"${CMAKE_SOURCE_DIR}/data/base/shaders/vk/line.vert"
	"${CMAKE_SOURCE_DIR}/data/base/shaders/vk/terrain_water.vert"
	"${CMAKE_SOURCE_DIR}/data/base/shaders/vk/decals.vert"
	"${CMAKE_SOURCE_DIR}/data/base/shaders/vk/nolight.vert"
	"${CMAKE_SOURCE_DIR}/data/base/shaders/vk/button.vert"
	"${CMAKE_SOURCE_DIR}/data/base/shaders/vk/tcmask.vert"
	"${CMAKE_SOURCE_DIR}/data/base/shaders/vk/rect.frag"
	"${CMAKE_SOURCE_DIR}/data/base/shaders/vk/texturedrect.frag"
	"${CMAKE_SOURCE_DIR}/data/base/shaders/vk/gfx.frag"
	"${CMAKE_SOURCE_DIR}/data/base/shaders/vk/text.frag"
	"${CMAKE_SOURCE_DIR}/data/base/shaders/vk/terrain.frag"
	"${CMAKE_SOURCE_DIR}/data/base/shaders/vk/terraindepth.frag"
	"${CMAKE_SOURCE_DIR}/data/base/shaders/vk/water.frag"
	"${CMAKE_SOURCE_DIR}/data/base/shaders/vk/decals.frag"
	"${CMAKE_SOURCE_DIR}/data/base/shaders/vk/nolight.frag"
	"${CMAKE_SOURCE_DIR}/data/base/shaders/vk/button.frag"
	"${CMAKE_SOURCE_DIR}/data/base/shaders/vk/tcmask.frag"
)


set(SHADER_LIST "")
foreach(SHADER ${SHADERS})
	get_filename_component(SHADER_FILE_PATH ${SHADER} DIRECTORY)
	get_filename_component(SHADER_FILE ${SHADER} NAME_WE)
	add_custom_command(OUTPUT "${SHADER}.spv"
		COMMAND "${GLSLC}"
		ARGS "-c" "${SHADER}"
		WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/data/base/shaders/vk"
		DEPENDS "${SHADER}"
	)
	list(APPEND SHADER_LIST "${SHADER}.spv;")
	install(FILES "${SHADER}.spv" DESTINATION "data/base/shaders/vk")
endforeach()
add_custom_target(glsl_compilation DEPENDS ${SHADER_LIST})

add_library(ivis-opengl ${HEADERS} ${SRC})
add_dependencies(ivis-opengl glsl_compilation)
target_include_directories(ivis-opengl PRIVATE ${HARFBUZZ_INCLUDE_DIRS} ${FREETYPE_INCLUDE_DIR_ft2build})
target_link_libraries(ivis-opengl PRIVATE framework PNG::PNG ${HARFBUZZ_LIBRARIES} ${FREETYPE_LIBRARIES} Vulkan::Vulkan SDL2::SDL2)
target_link_libraries(ivis-opengl PUBLIC ${OPENGL_gl_LIBRARY} ${OPENGL_glu_LIBRARY} GLEW::GLEW)

if("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
  if(${CMAKE_CROSSCOMPILING})
    target_compile_definitions(ivis-opengl PUBLIC "GLEW_STATIC")
  endif()
endif()
