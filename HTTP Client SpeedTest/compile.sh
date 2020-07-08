#!/bin/bash
echo "Compiling SpeedTest.c"
#gcc antman.c SpeedTest.c -O3 -Wall -Werror -o speedTest -lpthread 
gcc antman.c SpeedTest.c commands.c -O3 -Wall -o speedTest -lpthread 

