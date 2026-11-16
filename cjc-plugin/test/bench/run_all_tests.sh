#!/bin/bash

set -e

./run_test.sh binarytrees 20 17
./run_test.sh binarytrees_single 20 17
./run_test.sh nbody 20 1000000
./run_test.sh fasta 20 1000000
./run_test.sh gameoflive 5