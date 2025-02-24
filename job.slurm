#!/bin/bash
#SBATCH --job-name=batch_jobs
#SBATCH --output=output_%x_%j.txt   # Save stdout with job name + JOBID
#SBATCH --error=error_%x_%j.txt     # Save stderr with job name + JOBID
#SBATCH --nodes=1
#SBATCH --ntasks=64
#SBATCH --time=02:47:00

# Define folder where istances are located
INSTANCE_DIR="$(pwd)/inputs"

# Check existance of folder
if [ ! -d "$INSTANCE_DIR" ]; then
    echo "Error: Instance directory $INSTANCE_DIR not found!"
    exit 1
fi

# Loop over all files in the folder
for instance in "$INSTANCE_DIR"/*.col; do
    # Verify presence of file in the folder
    if [ ! -e "$instance" ]; then
        echo "No .col files found in $INSTANCE_DIR"
        exit 1
    fi

    # Extract file name
    instance_name=$(basename "$instance" .col)

    # Execute the program
    echo "Running on instance: $instance"
    srun ./build/graph-coloring "$instance" 10 > "output_${instance_name}.txt" 2> "error_${instance_name}.txt"
done
