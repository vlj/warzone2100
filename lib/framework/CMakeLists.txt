cmake_minimum_required (VERSION 3.5)
project (framework CXX)

file(GLOB HEADERS "*.h")
file(GLOB SRC "*.cpp")

add_library(framework ${HEADERS} ${SRC})
target_include_directories(framework PRIVATE ${SDL2_INCLUDE_DIR})
