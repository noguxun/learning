
# document
#  http://shrib.com/baremetal

#qemu-system-arm -M versatilepb -m 128M -nographic -kernel test.bin -serial telnet:localhost:1235,server
qemu-system-arm -M versatilepb -m 128M -nographic -kernel test.bin
