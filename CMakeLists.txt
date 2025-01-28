cmake_minimum_required(VERSION 3.16)
project(imageEditor)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set paths
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/../libs/imgui)
set(SDL3_DIR ${CMAKE_SOURCE_DIR}/../libs/SDL)
set(SDL3_LIB_DIR ${CMAKE_SOURCE_DIR}/../sdl_build)
set(SDL3_WEB_LIB_DIR ${CMAKE_SOURCE_DIR}/../sdl_build_web)
set(PFD_DIR ${CMAKE_SOURCE_DIR}/../libs/pfd)
set(STB_DIR ${CMAKE_SOURCE_DIR}/../libs/stb)

# Add source files
set(SOURCES
    main.cpp
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdl3.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

# Common include directories
set(COMMON_INCLUDE_DIRS
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${STB_DIR}
    ${SDL3_DIR}/include
)

if(EMSCRIPTEN)
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
    set(COMPILE_FLAGS "-s USE_SDL=3")
    set(LINK_FLAGS "-s USE_SDL=3 -s USE_WEBGL2=1 -s FULL_ES3=1 -s WASM=1 -s ALLOW_MEMORY_GROWTH=1")
    set(EXPORTED_FUNCTIONS "['_main','_malloc','_free','_processImageData']")
    set(EXPORTED_RUNTIME "['ccall','cwrap']")
    
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMPILE_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${LINK_FLAGS} -s EXPORTED_FUNCTIONS=${EXPORTED_FUNCTIONS} -s EXPORTED_RUNTIME_METHODS=${EXPORTED_RUNTIME}")

    # Set output directory for Emscripten build
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/web)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/web)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/web)

    add_executable(${PROJECT_NAME}_web ${SOURCES})
    target_include_directories(${PROJECT_NAME}_web PRIVATE ${COMMON_INCLUDE_DIRS})
    target_compile_definitions(${PROJECT_NAME}_web PRIVATE IMGUI_IMPL_OPENGL_ES3)
    
    target_link_options(${PROJECT_NAME}_web PRIVATE
        "SHELL:--shell-file ${CMAKE_SOURCE_DIR}/emscripten/shell_minimal.html"
        "SHELL:${LINK_FLAGS}"
        "SHELL:-s NO_EXIT_RUNTIME=0"
        "SHELL:-s ASSERTIONS=1"
    )

    target_link_libraries(${PROJECT_NAME}_web PRIVATE ${SDL3_WEB_LIB_DIR}/build-artifacts/web/libSDL3.a)
else()
    add_executable(${PROJECT_NAME} ${SOURCES})
    target_include_directories(${PROJECT_NAME} PRIVATE ${COMMON_INCLUDE_DIRS} ${SDL3_DIR}/include ${PFD_DIR})
    
    find_package(OpenGL REQUIRED)
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wformat)

    if(UNIX)
        target_compile_definitions(${PROJECT_NAME} PRIVATE LINUX)
        target_link_libraries(${PROJECT_NAME} PRIVATE dl)
    endif()

    target_link_libraries(${PROJECT_NAME} PRIVATE
        OpenGL::GL
        ${SDL3_LIB_DIR}/build-artifacts/static/libSDL3.a
    )
endif()