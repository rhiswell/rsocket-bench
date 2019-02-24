
# -f    Do not spawn chilren for each test, run serially.
# -D    Do not daemonize.
# -L name,family Use name to pick listen addr and family for family.
# -4    Do IPv4.
LD_PRELOAD=/usr/lib/rsocket/librspreload.so netserver -D -f -L 192.168.10.141 -4

