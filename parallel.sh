#!/bin/bash
#SBATCH -N1 -n16 --mem-per-cpu=1000m -t00:30:00
export OMP_NUM_THREADS=16
cd /home/dhganey/430/p1/parallel
#make
make clean
module load intel/14.02
icpc Main.cpp Permutation.cpp -o pj01 -fopenmp

for file in /home/dhganey/430/p1/correct/*.txt
do
	(time ./pj01 < $file) &>> "time.txt"
	echo $file >> "time.txt"
done
echo "done with correct"
for file in /home/dhganey/430/p1/incorrect/*.txt
do
	(time ./pj01 < $file) &>> "time.txt"
	echo $file >> "time.txt"
done
echo "Done with incorrect"