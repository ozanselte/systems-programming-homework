#!/bin/bash

rm -f file3 file4
make clean
make all
./programA -i file1 -o file3 -t 10 &
./programA -i file2 -o file3 -t 20 &
./programB -i file3 -o file4 -t 5 &
./programB -i file3 -o file4 -t 25 &