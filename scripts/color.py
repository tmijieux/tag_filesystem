#!/usr/bin/env python
import sys
if "-h" in sys.argv[1:] or "--help" in sys.argv[1:]:
    print("usage: {0} [-t|--terse]".format(sys.argv[0]))
    print("the --terse option format the color in an array")
    sys.exit(0)

terse = "-t" in sys.argv[1:] or "--terse" in sys.argv[1:]
write = sys.stdout.write
for i in range(2 if terse else 10):
    for j in range(30, 38):
        for k in range(40, 48):
            if terse:
                write("\33[%d;%d;%dm%d;%d;%d\33[m " % (i, j, k, i, j, k))
            else:
                write("%d;%d;%d: \33[%d;%d;%dm Hello, World! \33[m \n" %
                      (i, j, k, i, j, k,))
        write("\n")
