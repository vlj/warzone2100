cmake_minimum_required(VERSION 3.5)
project(warzone2100 CXX)

find_package(gflags REQUIRED)

file(GLOB HEADERS "*.h")
file(GLOB SRC "*.cpp")
qt5_wrap_cpp(MOCFILES multiint.h qtscriptdebug.h)

if("${CMAKE_CXX_COMPILER_ID}" MATCHES "MSVC")
  add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/autorevision.h"
    COMMAND "powershell.exe"
    ARGS "-exec" "bypass" "${CMAKE_SOURCE_DIR}/build_tools/autorevision.ps1"
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
elseif("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
  add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/autorevision.h"
    COMMAND "${CMAKE_SOURCE_DIR}/build_tools/autorevision"
    ARGS "-t" "h" "-o" "${CMAKE_CURRENT_BINARY_DIR}/autorevision.cache" ">" "${CMAKE_CURRENT_BINARY_DIR}/autorevision.h")
endif()

add_executable(warzone2100 ${HEADERS} ${SRC} ${MOCFILES} "${CMAKE_CURRENT_BINARY_DIR}/autorevision.h")

target_link_libraries(warzone2100 exception-handler gamelib ivis-opengl netplay script sdl-backend framework sequence sound widget miniupnpc)
target_link_libraries(warzone2100 ${OPENSSL_SSL_LIBRARY}
	${PHYSFS_LIBRARY}
	${PNG_LIBRARY}
	${ZLIB_LIBRARY}
	${GLEW_LIBRARY}
	${OPENGL_gl_LIBRARY}
	${OPENGL_glu_LIBRARY}
	${FREETYPE_LIBRARIES}
	${SSL_EAY_RELEASE}
	gflags
	Qt5::Script
	Qt5::Widgets
	)


if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    target_link_libraries(warzone2100
      crypto
      ogg
      vorbis
      vorbisfile
      theora
    )
  if (${CMAKE_CROSSCOMPILING})
    set_target_properties(warzone2100 PROPERTIES LINK_FLAGS "-Wl,--start-group,-subsystem,windows")
    target_link_libraries(warzone2100 ${QT5ALL_LIBRARIES} ${QT5ALL_LDFLAGS_OTHER})
    target_link_libraries(warzone2100 
      version
      ws2_32 iphlpapi
      Qt5::QWindowsIntegrationPlugin Qt5ThemeSupport Qt5FontDatabaseSupport Qt5EventDispatcherSupport
    )
  else()
    set_target_properties(warzone2100 PROPERTIES LINK_FLAGS "-Wl,--start-group")
  endif()
elseif("${CMAKE_CXX_COMPILER_ID}" MATCHES "MSVC")
    target_link_libraries(warzone2100
        "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib/libeay32.lib"
        "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib/harfbuzz.lib"
        "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib/SDL2.lib"
        "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib/ogg.lib"
        "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib/vorbis.lib"
        "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib/vorbisfile.lib"
        "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib/theora.lib"
        "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib/fmt.lib"
        "ws2_32.lib"
        "iphlpapi.lib")
endif()
