
RSOCKET="/usr/local/lib/rsocket/librspreload.so"
NETPERF="netperf -t TCP_STREAM -f g -H 192.168.10.141 -T 2 -l 10 --"
PERF="perf record -F 99 -e cycles --call-graph dwarf --"

tsks=()
tsks[${#tsks[@]}]="netperf_tcpstrm_1KB"
tsks[${#tsks[@]}]="netperf_tcpstrm_64KB"
tsks[${#tsks[@]}]="netperf_tcpstrm_1MB"

declare -A msg_size
msg_size+=( ["netperf_tcpstrm_1KB"]="$((2**10))" )
msg_size+=( ["netperf_tcpstrm_64KB"]="$((2**16))" )
msg_size+=( ["netperf_tcpstrm_1MB"]="$((2**20))" )

for tsk in ${tsks[@]}; do
    echo -e "\n\$LD_PRELOAD=$RSOCKET $PERF $NETPERF -m ${msg_size[$tsk]}"
    LD_PRELOAD=$RSOCKET $PERF $NETPERF -m ${msg_size[$tsk]}
    ./gen_fg.sh $tsk
done

