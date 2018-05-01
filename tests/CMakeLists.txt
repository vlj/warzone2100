enable_testing()
find_package(GTest REQUIRED)
add_executable(mapTest maptest.cpp)
target_include_directories(mapTest PRIVATE ${CMAKE_SOURCE_DIR}/tools)
target_link_libraries(mapTest PRIVATE GTest::GTest GTest::Main framework maputils)

gtest_discover_tests(mapTest WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../data")

add_executable(modelTest modeltest.cpp)
target_include_directories(modelTest PRIVATE ${CMAKE_SOURCE_DIR}/tools)
target_link_libraries(modelTest PRIVATE GTest::GTest GTest::Main framework maputils)

gtest_discover_tests(modelTest WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../data")

add_executable(jsTest lint.cpp lint.h qslint.cpp qtscripttest.cpp)
target_include_directories(jsTest PRIVATE ${CMAKE_SOURCE_DIR}/tools)
target_link_libraries(jsTest PRIVATE GTest::GTest GTest::Main framework maputils)

gtest_discover_tests(jsTest WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../data")
