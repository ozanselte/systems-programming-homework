#!/bin/bash

NODECNT=6301
CLNTCNT=250

for i in $(seq 1 $CLNTCNT); do
    ./client -a 127.0.0.1 -p 10000 -s $((RANDOM%NODECNT)) -d $((RANDOM%NODECNT)) &
done