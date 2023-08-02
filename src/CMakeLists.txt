
file(GLOB_RECURSE PROJ_SRC "*.cpp" "vulkan/*.cpp" "models/*.cpp"
"../external/imgui/imgui.cpp"
"../external/imgui/imgui_demo.cpp"
"../external/imgui/imgui_draw.cpp"
"../external/imgui/imgui_widgets.cpp"
"../external/imgui/imgui_tables.cpp"
"../external/imgui/backends/*.cpp"
)

add_executable(SpaceSim ${PROJ_SRC})
target_link_libraries(SpaceSim glm glfw ${Vulkan_LIBRARIES})