#!/bin/bash

# Acquire name of this breakdown
NAME=${1,-}

function print_usage_then_die
{
    echo "$0 <name_of_this_breakdown>"
    exit 1
}

[ -z "$NAME" ] && print_usage_then_die

# Generate svg
perf script | ./FlameGraph/stackcollapse-perf.pl | \
    ./FlameGraph/flamegraph.pl > perf.data.svg

# Rename and clean unused files
mv perf.data perf_$NAME.data
mv perf.data.svg perf_$NAME.data.svg

