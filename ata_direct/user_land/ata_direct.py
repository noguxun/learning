import sys, os, fcntl

fd = open("/dev/ata_lax","w")

loop_c = 100

while True:
    fcntl.ioctl(fd, 0xff, 0x2b)
    loop_c -= 1
    if loop_c <= 0:
        break;

fd.close()
