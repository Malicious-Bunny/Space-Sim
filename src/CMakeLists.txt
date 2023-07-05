add_executable (main main.cc)

target_link_libraries (main glad -lglfw -lGL -lX11 -lpthread -lXrandr -lXi -ldl)
