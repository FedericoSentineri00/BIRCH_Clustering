#!/bin/bash
#PBS -l select=1:ncpus=2:mem=2gb
# set max execution time
#PBS -l walltime=0:03:00
# imposta la coda di esecuzione
#PBS -q short_cpuQ

module load mpich-3.2
mpirun.actual -n 1 ./BIRCH_CLUSTERING/birch_code/birch_clustering