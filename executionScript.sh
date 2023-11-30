module load mpich-3.2
mpicc -g -Wall -o birch_clustering ./src/main.c ./src/*/*.c -lm