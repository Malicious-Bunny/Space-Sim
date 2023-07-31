cmake_minimum_required(VERSION 3.12)

project(SpaceSim VERSION 0.0)

set (CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories(include/)

file(GLOB_RECURSE PROJ_SRC "src/*cpp")
add_executable(SpaceSim ${PROJ_SRC})
