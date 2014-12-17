all:
	arm-linux-gnueabi-as -g startup.s -o startup.o
	arm-linux-gnueabi-gcc -c -g test.c -o test.o
	arm-linux-gnueabi-ld -Map test.map -T test.ld test.o startup.o -o test.elf
	arm-linux-gnueabi-objcopy -O binary test.elf test.bin


clean:
	rm *.o
	rm *.elf
	rm *.map
	rm *.bin
