#!/usr/local/bin/bash

if [ "$#" -lt 2 ]; then
    echo "USAGE:"
    echo "$0 num_processes_per_prog prog1 prog2 [prog3 ... prog5]"
    echo "Will output score for prog1"
    exit 1
fi

NUM_PROCESS=$1
PRG_NAME=$2
TOTAL=$((($#-1)*$NUM_PROCESS))

sudo su arena -c "./arena $*"

sleep 10
ps -U arena > match_result.txt
sudo pkill -KILL -U arena

X=$(grep $PRG_NAME match_result.txt | wc -l)
if [ $X -gt $(($TOTAL)) ]; then
	X=$TOTAL
fi
MATCH_SCORE=$(echo "scale=2; $X*100/($TOTAL)" | bc -l)

echo "GAME: $2 with maximum number of processes $TOTAL" 
echo "-------------------------" 
echo $MATCH_SCORE 
