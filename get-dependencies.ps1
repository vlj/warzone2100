git clone https://github.com/Microsoft/vcpkg.git;
pushd vcpkg;
.\bootstrap-vcpkg.bat;
.\vcpkg install harfbuzz libiconv libogg libtheora libvorbis libpng openal-soft openssl physfs sdl2
popd;
