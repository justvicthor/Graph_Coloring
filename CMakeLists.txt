cmake_minimum_required(VERSION 3.10)
project(graph-coloring)

# Use C++17 (or another standard if needed)
set(CMAKE_CXX_STANDARD 17)

# Include directories
include_directories(${PROJECT_SOURCE_DIR}/include)

find_package(MPI REQUIRED)

# Add the executable and source files
add_executable(graph-coloring
        src/main.cpp
        src/solution.cpp
        src/graph.cpp
)

target_link_libraries(graph-coloring PRIVATE MPI::MPI_CXX pthread)
