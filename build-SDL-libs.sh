source "$HOME/mulVid/libs/emscripten/emsdk_env.sh" && \
cmake -S ./libs/SDL -B sdl_build \
    -DSDL_STATIC=ON \
    -DSDL_SHARED=ON \
    -DSDL_TESTS=ON \
    -DSDL_EXAMPLES=ON \
    -DCMAKE_LIBRARY_OUTPUT_DIRECTORY=build-artifacts/shared \
    -DCMAKE_RUNTIME_OUTPUT_DIRECTORY=build-artifacts \
    -DCMAKE_ARCHIVE_OUTPUT_DIRECTORY=build-artifacts/static && \
cmake --build sdl_build && \
emcmake cmake -S ./libs/SDL -B sdl_build_web \
    -DSDL_STATIC=ON \
    -DSDL_SHARED=OFF \
    -DSDL_TESTS=ON \
    -DSDL_EXAMPLES=ON \
    -DCMAKE_LIBRARY_OUTPUT_DIRECTORY=build-artifacts \
    -DCMAKE_RUNTIME_OUTPUT_DIRECTORY=build-artifacts \
    -DCMAKE_ARCHIVE_OUTPUT_DIRECTORY=build-artifacts && \
emmake cmake --build sdl_build_web