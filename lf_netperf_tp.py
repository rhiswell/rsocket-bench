#!/usr/local/env python3

import matplotlib.pyplot as plt

FILES = ('netperf_tp.csv', )
LABELS = ('RSOCKET_TCP', )


def extract_XY(fpath):
    lines = open(fpath).readlines()[2:]
    X = [float(line.split(',')[0]) for line in lines]
    Y = [float(line.split(',')[1]) for line in lines]
    return X, Y


for i, f in enumerate(FILES):
    X, Y = extract_XY(f)
    #markers_on = [0, 1]
    #plt.plot(X, Y, '-bx', label=LABELS[i], markevery=markers_on)
    plt.plot(X, Y, '-bx', label=LABELS[i])
plt.xscale('log', basex=2)
plt.yscale('linear')
plt.legend()
plt.xlabel('Message size (Bytes)')
plt.ylabel('Tput (Gbps)')
plt.show()
