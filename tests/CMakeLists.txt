enable_testing()
find_package(gtest REQUIRED)
find_package(glog REQUIRED)
add_executable(mapTest maptest.cpp)
target_include_directories(mapTest PRIVATE ${CMAKE_SOURCE_DIR}/tools)
target_link_libraries(mapTest PRIVATE GTest::GTest GTest::Main framework maputils)

gtest_add_tests(TARGET mapTest WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../data")

set_target_properties (mapTest PROPERTIES
    FOLDER Tests
)

add_executable(modelTest modeltest.cpp)
target_include_directories(modelTest PRIVATE ${CMAKE_SOURCE_DIR}/tools)
target_link_libraries(modelTest PRIVATE GTest::GTest GTest::Main framework maputils)

gtest_add_tests(TARGET modelTest WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../data")

set_target_properties (modelTest PROPERTIES
    FOLDER Tests
)

find_package(Qt5 COMPONENTS Script REQUIRED)

add_executable(jsTest lint.cpp lint.h qslint.cpp qtscripttest.cpp)
target_include_directories(jsTest PRIVATE ${CMAKE_SOURCE_DIR}/tools)
target_link_libraries(jsTest PRIVATE GTest::GTest GTest::Main framework maputils Qt5::Script)

#gtest_add_tests(TARGET jsTest WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../data")

set_target_properties (jsTest PROPERTIES
    FOLDER Tests
)