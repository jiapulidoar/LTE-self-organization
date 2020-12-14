#!/bin/bash
for i in $(eval echo {0..$1})
do
    gnuplot nodes$i.plt
	
done
