#!/bin/bash
#PBS -l select=1:ncpus=2:mem=2gb
# set max execution time
#PBS -l walltime=0:01:00
# imposta la coda di esecuzione
#PBS -q short_cpuQ

module load mpich-3.2
mpirun.actual -n 2 ./BIRCH_CLUSTERING/birch_clustering 100 0.8 1 ./BIRCH_CLUSTERING/IRIS.csv , 1 ./BIRCH_CLUSTERING/output