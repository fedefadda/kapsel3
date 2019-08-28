#!/bin/sh
#$ -S /bin/sh
#$ -cwd
#$ -o std.out -e std.err
#$ -q tuna0.q
#$ -v LD_LIBRARY_PATH=/opt/hdf5/1.10.5/lib/:/opt/fftw/3.3.7/lib
#$ -pe openmp 6
#$ -v KMP_AFFINITY=verbose,scatter,1
#$ -v MKL_NUM_THREADS=1
#$ -v MKL_DOMAIN_NUM_THREADS="MKL_ALL=1, MKL_FFT=6"
#$ -v MKL_DYNAMIC=FALSE
#$ -v OMP_NUM_THREADS=6
#$ -v OMP_DYNAMIC=FALSE
#$ -v OMP_SCHEDULE="static"
# run
./kapsel -Iinput.udf -Ooutput.udf -Ddefine.udf -Rrestart.udf
