#!/bin/bash

FG=$(pwd)/../FlameGraph
PERF="perf record -F 99 -e cycles --call-graph dwarf -- ./a.out 1 20 1000000000" # <nr_thread> <nr_loop> <nr_sample>

# Generate FG
$PERF && perf script | $FG/stackcollapse-perf.pl | \
    $FG/flamegraph.pl > perf.data.svg
