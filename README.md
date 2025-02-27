# Graph-coloring
**EuMaster4HPC Student Challenge - Team 5**

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
|-- job.slurm 
```

## Compilation
To compile the program, navigate to the root folder `Graph-coloring` and use CMake with the following steps:
```sh
module load openmpi/4.1.2.1
mkdir build
cd build
cmake ..
make
```

## Running the Program
There are **two modes** for running our program.

### Preliminaries: choice of resources allocation
Before dealing with the two modalities our code offers to be run with, it is necessary to first decide how many resources have to be allocated to execute the code.

Once located in the root folder, open file `job.slurm`. First lines of this file will show what follows:
```sh
#!/bin/bash
#SBATCH --job-name=batch_jobs
#SBATCH --output=output_%x_%j.txt   # Save stdout with job name + JOBID
#SBATCH --error=error_%x_%j.txt     # Save stderr with job name + JOBID
#SBATCH --nodes=2
#SBATCH --ntasks=128
#SBATCH --time=00:30:00             # -> 30min = 1800sec
```

According to one's needs, it is possible to modify values related to:
- `--nodes ‎  `: Number of nodes to run the program on
- `--ntasks`: Number of parallel processes to run
- `--time ‎ ‎ `: Total amount of time for the entire sbatch

### Mode 1: job.slurm for a single instance
In order to make `sbatch` work with a single instance, we adapted `job.slurm` file based on the following command:
```sh
srun -N <number_of_nodes> -n <number_of_processes> ./graph-coloring ../inputs/<input_file_name> <time_limit_in_seconds>
```
whose **arguments** are:
- `<number_of_nodes>`: Number of nodes to run the program on
- `<number_of_processes>`: Number of parallel processes to run
- `<input_file_name>`: Name of the input file located in the `inputs/` directory
- `<time_limit_in_seconds>`: The maximum execution time in seconds (for a single instance)

\
Once opened the `job.slurm` file, un-comment all code related to **Mode 1** execution.

A correct execution of this step should make part of `job.slurm` content appear as follows:
```sh
###################################################################
# Mode 1: Run the program on a single instance
# (uncomment this section and comment out the following section)
###################################################################
if [ "$#" -ne 1 ]; then
    echo "Usage: $0 instance_name_without_extension"
    exit 1
fi

instance="$INSTANCE_DIR/$1.col"
if [ ! -f "$instance" ]; then
    echo "Error: Instance file $instance not found!"
    exit 1
fi

instance_name=$(basename "$instance" .col)

echo "Running on instance: $instance"
srun ./build/graph-coloring "$instance" 1800 > "output_${instance_name}.txt" 2> "error_${instance_name}.txt"
exit 0
```
In the `srun` command at penultimate row, you may want to change the value associated to the `<time_limit_in_seconds>`; in particular, since we are dealing with a single instance, it would be ideal to make `#SBATCH --time` and `<time_limit_in_seconds>` coincide.

Comment now out code related to **Mode 2**:
```sh
###################################################################
# Mode 2: Run the program on all instances
# (leave this section active if you want to run on all instances)
###################################################################
#for instance in "$INSTANCE_DIR"/*.col; do
# Verify the file exists
#if [ ! -e "$instance" ]; then
#echo "No .col files found in $INSTANCE_DIR"
#exit 1
#fi

    # Extract the instance name
#    instance_name=$(basename "$instance" .col)

    # Run the program
#    echo "Running on instance: $instance"
#    srun ./build/graph-coloring "$instance" 180 > "output_${instance_name}.txt" 2> "error_${instance_name}.txt"
#done
```

You can now execute the code on a single instance (e.g., `anna.col`) by typing:
```sh
sbatch job.slurm anna
```

Alternatively, it is still possible to directly invoke the `srun` command on the single instance (here it is needed to be located in `buld/`):
```sh
srun -N <number_of_nodes> -n <number_of_processes> ./graph-coloring ../inputs/<input_file_name> <time_limit_in_seconds>
```

### Mode 2: job.slurm for ALL instances
> ⚠️ **Note:** **Mode 2** is currently the default mode. 

Analogously to what done for executing code with **Mode 1**, un-comment code related to **Mode 2** execution.

A correct execution of this step should make part of `job.slurm` content appear as follows:
```sh
###################################################################
# Mode 2: Run the program on all instances
# (leave this section active if you want to run on all instances)
###################################################################
for instance in "$INSTANCE_DIR"/*.col; do
# Verify the file exists
if [ ! -e "$instance" ]; then
echo "No .col files found in $INSTANCE_DIR"
exit 1
fi

    # Extract the instance name
    instance_name=$(basename "$instance" .col)

    # Run the program
    echo "Running on instance: $instance"
    srun ./build/graph-coloring "$instance" 180 > "output_${instance_name}.txt" 2> "error_${instance_name}.txt"
done
```
Comment now out code related to **Mode 1**:
```sh
###################################################################
# Mode 1: Run the program on a single instance
# (uncomment this section and comment out the following section)
###################################################################
#if [ "$#" -ne 1 ]; then
#    echo "Usage: $0 instance_name_without_extension"
#    exit 1
#fi

#instance="$INSTANCE_DIR/$1.col"
#if [ ! -f "$instance" ]; then
#    echo "Error: Instance file $instance not found!"
#    exit 1
#fi

#instance_name=$(basename "$instance" .col)

#echo "Running on instance: $instance"
#srun ./build/graph-coloring "$instance" 180 > "output_${instance_name}.txt" 2> "error_${instance_name}.txt"
#exit 0
```

Remember that, in the `srun` command at penultimate row, you might want to adapt the value associated to the `<time_limit_in_seconds>`, which represents the amount of time a single instance must be run within.
Since we are dealing with ALL instances in the folder, it would be ideal to give a big value to `#SBATCH --time` and a smaller value to `<time_limit_in_seconds>`. 

You can now execute the code on ALL instances by typing:
```sh
sbatch job.slurm
```


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
