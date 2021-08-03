cmake_minimum_required(VERSION 3.13.4)

project(my-debugger)

add_compile_options(-std=c++14)

include_directories(external/linenoise/ include/)
link_directories(src/)

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  # Using Clang
	message("--- Using CLANG compiler")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	message("--- Using GNU compiler")
  # Using GCC
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
  # Using Intel C++
	message("--- Using Intel compiler")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  # Using Visual Studio C++
	message("--- Using MSVC compiler")
endif()

add_executable(${PROJECT_NAME} src/main.cpp 
                               src/debugger.cpp 
                               src/breakpoint.cpp 
                               src/helper.cpp
                               external/linenoise/linenoise.c)



add_executable(hello-world examples/hello-world.cpp)
set_target_properties(hello-world
	PROPERTIES COMPILE_OPTIONS "-g")

add_executable(simpleadd examples/simpleadd.cpp)
set_target_properties(hello-world
	PROPERTIES COMPILE_OPTIONS "-g")
