#!/bin/bash
# Project: Merge-splitting sort
# Author:  Nikola Valesova, xvales02
# Date:    8. 4. 2018


# check number of arguments
if [ $# -ne 2 ]; then
    echo "ERROR: Invalid number of arguments!"
    exit 1
fi

# compile source files
mpic++ --prefix /usr/local/share/OpenMPI -o mss mss.cpp

# create file with random numbers
dd if=/dev/random bs=1 count=$1 of=numbers 2>/dev/null

# execute
mpirun --prefix /usr/local/share/OpenMPI -np $2 mss

# clean
rm -f mss numbers
