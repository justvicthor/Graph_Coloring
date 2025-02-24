cmake_minimum_required(VERSION 3.10)
project(graph-coloring)

# Use C++17 (or another standard if needed)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Include directories
include_directories(${PROJECT_SOURCE_DIR}/include)

find_package(MPI REQUIRED)

# Add the executable and source files
add_executable(graph-coloring
        src/main.cpp
        src/solution.cpp
        src/graph.cpp
        src/maxclique.cpp
)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.0)
    target_link_libraries(graph-coloring PRIVATE stdc++fs)
endif()

target_link_libraries(graph-coloring PRIVATE MPI::MPI_CXX pthread)
