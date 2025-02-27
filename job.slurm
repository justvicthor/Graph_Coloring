#!/bin/bash
#SBATCH --job-name=batch_jobs
#SBATCH --output=output_%x_%j.txt   # Save stdout with job name + JOBID
#SBATCH --error=error_%x_%j.txt     # Save stderr with job name + JOBID
#SBATCH --nodes=2
#SBATCH --ntasks=128
#SBATCH --time=00:30:00

# Define the folder where instances are located
INSTANCE_DIR="$(pwd)/inputs"

# Check if the folder exists
if [ ! -d "$INSTANCE_DIR" ]; then
    echo "Error: Instance directory $INSTANCE_DIR not found!"
    exit 1
fi

###################################################################
# Mode 1: Run the program on a single instance
# (uncomment this section and comment out the following section)
###################################################################
# if [ "$#" -ne 1 ]; then
#     echo "Usage: $0 instance_name_without_extension"
#     exit 1
# fi

# instance="$INSTANCE_DIR/$1.col"
# if [ ! -f "$instance" ]; then
#     echo "Error: Instance file $instance not found!"
#     exit 1
# fi

# instance_name=$(basename "$instance" .col)

# echo "Running on instance: $instance"
# srun ./build/graph-coloring "$instance" 180 > "output_${instance_name}.txt" 2> "error_${instance_name}.txt"
# exit 0

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