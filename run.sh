clear
rm -rf build/
mkdir build
cd build/
cmake ..
make
mpirun -n 8 ./graph-coloring ../inputs/anna.col 20