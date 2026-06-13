cmake_minimum_required(VERSION 3.20)
project(voxelgame)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE SOURCES "src/*.cpp")
add_executable(voxelgame ${SOURCES})



target_include_directories(voxelgame PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(voxelgame PRIVATE
    ${CMAKE_SOURCE_DIR}/lib/raylib.lib
    opengl32
    gdi32
    winmm
)

