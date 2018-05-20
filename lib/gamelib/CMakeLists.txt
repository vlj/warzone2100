cmake_minimum_required (VERSION 3.5)
project (gamelib CXX)

if(ENABLE_NLS)
	find_package (Intl REQUIRED)
endif()

file(GLOB SRC "*.cpp")
set(HEADERS
  ${CMAKE_SOURCE_DIR}/includes/map/map.h
  ${CMAKE_SOURCE_DIR}/includes/map/terrain_types.h
  audp_parser.h
  gtime.h
  parser.h
  )

add_library(gamelib STATIC ${HEADERS} ${SRC})
target_link_libraries(gamelib PRIVATE framework)
target_include_directories(gamelib PUBLIC ${CMAKE_SOURCE_DIR}/includes)
if(ENABLE_NLS)
	target_include_directories(gamelib PRIVATE "${Intl_INCLUDE_DIRS}")
	target_link_libraries(gamelib PRIVATE ${Intl_LIBRARIES})
endif()
target_compile_definitions(gamelib PRIVATE "YY_NO_UNISTD_H")
