cmake_minimum_required(VERSION 3.20)

set(CMAKE_TOOLCHAIN_FILE "" CACHE STRING "" FORCE)
set(VCPKG_MANIFEST_MODE OFF CACHE BOOL "" FORCE)

project(pixeltaps LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "build this as x64")
endif()

set(minhook ${CMAKE_SOURCE_DIR}/external/minhook)

file(GLOB_RECURSE minhooksources ${minhook}/src/*.c)

add_library(pixeltaps SHARED
    src/main.cpp
    src/hooks.cpp
    src/pixeltap.cpp
    src/subtick.cpp
    src/game.cpp
    ${minhooksources}
)

target_include_directories(pixeltaps PRIVATE src ${minhook}/include)
target_compile_definitions(pixeltaps PRIVATE NOMINMAX)

set_target_properties(pixeltaps PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/dlls)
