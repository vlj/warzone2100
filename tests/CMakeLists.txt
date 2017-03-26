enable_testing()
add_executable(checkTrigTable trigtest.cpp)
target_link_libraries(checkTrigTable
    framework
    ${SSL_EAY_RELEASE} ${OPENSSL_SSL_LIBRARY}
    "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib/libeay32.lib"
    "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib/fmt.lib"
    "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib/SDL2.lib")
add_test(NAME trigTable COMMAND checkTrigTable)