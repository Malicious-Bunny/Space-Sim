
file(GLOB_RECURSE PROJ_SRC "*.cpp" "vulkan/*.cpp" "models/*.cpp"
"../external/imgui/imgui.cpp"
"../external/imgui/imgui_demo.cpp"
"../external/imgui/imgui_draw.cpp"
"../external/imgui/imgui_widgets.cpp"
"../external/imgui/imgui_tables.cpp"
"../external/imgui/backends/*.cpp"
"*.h"
"vulkan/*.h"
)

add_executable(SpaceSim ${PROJ_SRC})

IF(WIN32)

    file(GLOB VK_SOURCE_GROUP "vulkan/*.cpp" "vulkan/*.h")
    source_group(vulkan FILES ${VK_SOURCE_GROUP})

    target_link_libraries(SpaceSim glfw3 vulkan-1)
    include_directories("$ENV{VK_SDK_PATH}/Include/")
    include_directories(external/glfw/)

    file(GLOB SRC_SOURCE_GROUP "./*.cpp" "./*.h")
    source_group(src FILES ${SRC_SOURCE_GROUP})

ELSE()

    target_link_libraries(SpaceSim glfw ${Vulkan_LIBRARIES})    

ENDIF()