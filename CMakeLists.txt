cmake_minimum_required(VERSION 3.13.4)
project(my-debugger)

add_compile_options(-std=c++14)

include_directories(external/linenoise/ include/)
add_executable(${PROJECT_NAME} src/main.cpp src/debugger.cpp external/linenoise/linenoise.c)

add_executable(hello-world examples/hello-world.cpp)
set_target_properties(hello-world
                      PROPERTIES COMPILE_FLAG "-g -O0")


