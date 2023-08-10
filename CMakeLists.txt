cmake_minimum_required(VERSION 3.12)

project(SpaceSim VERSION 0.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE Debug)

include_directories(external/)
include_directories(external/glm/)
include_directories(external/imgui/)

IF(WIN32)
    set(Vulkan_LIBRARY "$ENV{VK_SDK_PATH}/Lib")
    set(Vulkan_INCLUDE_DIR "$ENV{VK_SDK_PATH}/Include")
    link_directories(lib)
ENDIF()

if (DEFINED VULKAN_SDK_PATH)
    set(Vulkan_INCLUDE_DIRS "${VULKAN_SDK_PATH}/Include")
    	set(Vulkan_LIBRARIES "${VULKAN_SDK_PATH}/Lib")
    	set(Vulkan_FOUND "True")
else()
    find_package(Vulkan REQUIRED)
    message(STATUS "Found Vulkan: $ENV{VULKAN_SDK}")
endif()
if (NOT Vulkan_FOUND)
    message(FATAL_ERROR "Could not find Vulkan library!")
else()
    message(STATUS "Using vulkan lib at: ${Vulkan_LIBRARIES}")
endif()

add_subdirectory(external/glm)
add_subdirectory(src/)

set_target_properties(SpaceSim PROPERTIES FOLDER "src")

set(SRC_SOURCE_GROUP "src/*.cpp")
set(SRC_HEADER_GROUP "src/*.h")

source_group(src FILES ${SRC_SOURCE_GROUP})
source_group(src FILES ${SRC_HEADER_GROUP})

