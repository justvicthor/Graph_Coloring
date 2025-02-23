# Graph-coloring
HPC Challenge - Team 5

## Overview
This program performs graph coloring using MPI for parallel processing. It requires an input file containing the graph data and outputs the results to specified directories.

## Directory Structure
Ensure the following directory structure in the root directory before running the program:
```
Graph-coloring/
│-- inputs/       # Directory for input files
│-- out/          # Directory for output solutions
│-- out_opt/      # Directory for optimal output solutions
│
│-- include/      # Header files
│-- src/          # Implementation files
│
│-- CMakeLists.txt
```

## Compilation
To compile the program, navigate to the root folder `Graph-coloring` and use CMake with the following steps:
```sh
mkdir build
cd build
cmake ..
make
```

## Running the Program
To execute the program, use the `mpirun` command:
```sh
mpirun -n <number_of_processes> ./graph-coloring ../inputs/<input_file_name> <time_limit_in_seconds>
```
### Arguments:
- `<number_of_processes>`: Number of parallel processes to run
- `<input_file_name>`: Name of the input file located in the `inputs/` directory
- `<time_limit_in_seconds>`: The maximum execution time in seconds

## Example Usage
```sh
mpirun -n 4 ./graph-coloring ../inputs/anna.col 60
```
This command runs the program using 4 processes on the input file `anna.col` with a time limit of 60 seconds.

## Output
The results will be stored in the `out/` and `out_opt/` directories for suboptimal and optimal solutions, respectively.

## Requirements
- C++ compiler with MPI support
- CMake
- OpenMPI or another MPI implementation

## Notes
- Ensure MPI is properly installed and configured before running the program.
- The input file must be placed in the `inputs/` directory.
- The `build/` directory is only for compilation and can be deleted if necessary.
