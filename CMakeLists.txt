cmake_minimum_required(VERSION 3.21)

# define the project name
project(sim8051)

set(VERSION_MAJOR 0)
set(VERSION_MINOR 2)

set(CMAKE_CXX_STANDARD 17)

# static builds for windows
if (MSVC)
	set(SFML_STATIC_LIBRARIES TRUE)
	add_compile_definitions(IMGUI_SFML_SHARED_LIB=0 SFML_STATIC=1)
endif()

add_definitions(-DCMAKE_PROJECT_ROOT="${CMAKE_CURRENT_SOURCE_DIR}")

# packages
find_package(SFML 3.0 COMPONENTS Graphics REQUIRED)

# subdirectories
set(EXE_NAME ${PROJECT_NAME})
add_subdirectory(sim8051/src)
