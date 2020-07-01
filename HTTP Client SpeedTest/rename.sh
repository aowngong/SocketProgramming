#!/bin/bash
echo "Checking Memory"
valgrind --tool=memcheck --log-file=vallog --leak-check=full --verbose ./speedTest 119.46.114.58 8080



