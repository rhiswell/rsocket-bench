
# -f G|M|K|g|m|k    Set the output units.
# -T lcpu,rcpu      Request netperf/netserver be bound to local/remote cpu.
# -l testlen        Specify test duration (>0 secs) (<0 bytes/trans).
NETPERF="netperf -t TCP_STREAM -f g -H 192.168.10.141 -T 2 -l 10"
OUTPUT="netperf_tp.csv"

printf "LD_PRELOAD=/usr/lib/rsocket/librspreload.so $NETPERF "'-- -m $msg_size'"\n" \
    | tee $OUTPUT
printf "msg_size (bytes), throughput (Gbps)\n" | tee -a $OUTPUT
for e in $(seq 8 20); do
    msg_size=$((2**e))  # bytes
    LD_PRELOAD=/usr/lib/rsocket/librspreload.so $NETPERF -- -m $msg_size | \
        tail -n1 | awk '{ printf "%s, %s\n", $3, $5 }' | tee -a $OUTPUT
done

