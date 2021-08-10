cmake_minimum_required(VERSION 3.13.4)

project(my-debugger)

# add_compile_options(-std=c++14)

include_directories(external/linenoise/ include/)
link_directories(src/)

add_executable(${PROJECT_NAME} src/main.cpp 
                               src/debugger.cpp 
                               src/breakpoint.cpp 
                               src/helper.cpp
                               external/linenoise/linenoise.c)



add_executable(hello examples/hello.cpp)
set_target_properties(hello
	PROPERTIES COMPILE_OPTIONS "-g")

add_executable(simpleadd examples/simpleadd.cpp)
set_target_properties(simpleadd
	PROPERTIES COMPILE_OPTIONS "-g")

add_executable(cerrs examples/cerrs.cpp)
set_target_properties(cerrs
	PROPERTIES COMPILE_OPTIONS "-g")

add_executable(couts examples/couts.cpp)
set_target_properties(couts
	PROPERTIES COMPILE_OPTIONS "-g")

add_executable(function examples/function.cpp)
set_target_properties(function
	PROPERTIES COMPILE_OPTIONS "-g")

