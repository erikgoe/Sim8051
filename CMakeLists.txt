cmake_minimum_required(VERSION 3.6)

# define the project name
project(sim8051)

set(VERSION_MAJOR 0)
set(VERSION_MINOR 1)

set(CMAKE_CXX_STANDARD 17)

# static builds for windows
if (MSVC)
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
	set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} /MT")
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO  "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /MTd")
endif()

add_definitions(-DCMAKE_PROJECT_ROOT="${CMAKE_CURRENT_SOURCE_DIR}")

# packages
find_package(SFML 2.5 COMPONENTS graphics REQUIRED)

# subdirectories
set(EXE_NAME ${PROJECT_NAME})
add_subdirectory(sim8051/src)
