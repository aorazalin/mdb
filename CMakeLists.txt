cmake_minimum_required(VERSION 3.13.4)

project(mdb)

# add_compile_options(-std=c++14)

include_directories(external/linenoise/
										external/libelfin/
										include/
										external/libelfin/dwarf
										external/libelfin/elf)

link_directories(src/)

add_executable(${PROJECT_NAME} src/main.cc 
                               src/debugger.cc 
                               src/breakpoint.cc 
                               src/helper.cc
                               external/linenoise/linenoise.c)


add_executable(add examples/add.cc)
set_target_properties(add
	PROPERTIES COMPILE_FLAGS "-g -O0 -fno-omit-frame-pointer -gdwarf-2")

add_executable(couts examples/couts.cc)
set_target_properties(couts
	PROPERTIES COMPILE_FLAGS "-g -O0 -fno-omit-frame-pointer -gdwarf-2")

add_executable(function examples/function.cc)
set_target_properties(function
	PROPERTIES COMPILE_FLAGS  "-g -O0 -fno-omit-frame-pointer -gdwarf-2")

add_executable(nested examples/nested.cc)
set_target_properties(nested
	PROPERTIES COMPILE_FLAGS "-g -O0 -fno-omit-frame-pointer -gdwarf-2")

add_executable(variables examples/variables.cc)
set_target_properties(variables
	PROPERTIES COMPILE_FLAGS "-g -O0 -fno-omit-frame-pointer -gdwarf-2")

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



