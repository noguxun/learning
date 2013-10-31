import sys
import os

if len(sys.argv) != 2:
    sys.exit("param not correct\n")

value = int(sys.argv[1], 0)

shift = 31
while(1):
    print(" {0:2d}".format(shift), end="")
    if shift % 4 == 0:
        print(" |", end="")
    if shift == 0:
        break;
    shift -= 1

print("");

shift = 31
while(1):
    tmp = (value >> shift) & 0x1;
    print("  " + str(tmp), end="" );
    if shift % 4 == 0:
        print(" |", end="")
    if shift == 0:
    	break
    shift -= 1

print("\n")
