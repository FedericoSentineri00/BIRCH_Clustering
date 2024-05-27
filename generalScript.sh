#!/bin/bash
#PBS -l select=1:ncpus=4:mem=2gb -l place=pack:excl
# set max execution time
#PBS -l walltime=3:00:00
# imposta la coda di esecuzione
#PBS -q short_cpuQ

module load mpich-3.2
mpirun.actual -n 4 ./PARALLEL_BIRCH_CLUSTERING/birch_clustering 100 7.0 1 ./PARALLEL_BIRCH_CLUSTERING/ArtificialDataset_1000000-15-20.csv , 1
mpirun.actual -n 4 ./PARALLEL_BIRCH_CLUSTERING/birch_clustering 100 7.0 1 ./PARALLEL_BIRCH_CLUSTERING/ArtificialDataset_2500000-15-20.csv , 1
mpirun.actual -n 4 ./PARALLEL_BIRCH_CLUSTERING/birch_clustering 100 7.0 1 ./PARALLEL_BIRCH_CLUSTERING/ArtificialDataset_5000000-15-20.csv , 1
mpirun.actual -n 4 ./PARALLEL_BIRCH_CLUSTERING/birch_clustering 100 7.0 1 ./PARALLEL_BIRCH_CLUSTERING/ArtificialDataset_7500000-15-20.csv , 1