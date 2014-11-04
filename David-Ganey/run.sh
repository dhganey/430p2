#!/bin/bash
#start by compiling the program
make

#first, generate pthreads outputs for all the inputs
for file in ./inputs/*.cc
do
    filename=`basename $file`
    filename=${filename%.*}
    addition="_construct.cc"
    filename=$filename$addition
    ./pj02 < $file > outputs/$filename
done

#then, compile and run those outputs
for file in ./outputs/*.cc
do
    filename=`basename $file`
    filename=${filename%.*}
    addition="_executable"
    filename=$filename$addition
    g++ $file -lpthread -o ./outputs/$filename
    echo "compiled"
    ./outputs/$filename
done