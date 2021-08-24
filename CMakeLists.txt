cmake_minimum_required(VERSION 3.13.4)

project(my-debugger)

# add_compile_options(-std=c++14)

include_directories(external/linenoise/
										external/libelfin/
										include/
										external/libelfin/dwarf
										external/libelfin/elf)

link_directories(src/)

add_executable(${PROJECT_NAME} src/main.cpp 
                               src/debugger.cpp 
                               src/breakpoint.cpp 
                               src/helper.cpp
                               external/linenoise/linenoise.c)

														 # set_target_properties(${PROJECT_NAME} # tobecommented
														 #	PROPERTIES COMPILE_OPTIONS "-g")

add_executable(hello examples/hello.cpp)
set_target_properties(hello
	PROPERTIES COMPILE_FLAGS "-g -O0")

add_executable(simpleadd examples/simpleadd.cpp)
set_target_properties(simpleadd
	PROPERTIES COMPILE_FLAGS "-g -O0")

add_executable(cerrs examples/cerrs.cpp)
set_target_properties(cerrs
	PROPERTIES COMPILE_FLAGS "-g -O0")

add_executable(couts examples/couts.cpp)
set_target_properties(couts
	PROPERTIES COMPILE_FLAGS "-g -O0")

add_executable(function examples/function.cpp)
set_target_properties(function
	PROPERTIES COMPILE_FLAGS  "-g -O0")

add_executable(nested-functions examples/nested-functions.cpp)
set_target_properties(nested-functions
	PROPERTIES COMPILE_FLAGS "-g -O0")

# run make in libelfin
add_custom_target(
	libelfin
	COMMAND make
	WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/external/libelfin
) 

target_link_libraries(${PROJECT_NAME}
										  ${PROJECT_SOURCE_DIR}/external/libelfin/dwarf/libdwarf++.so
											${PROJECT_SOURCE_DIR}/external/libelfin/elf/libelf++.so)

add_dependencies(${PROJECT_NAME} libelfin)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/external/cwalk)

target_link_libraries(${PROJECT_NAME} cwalk)



