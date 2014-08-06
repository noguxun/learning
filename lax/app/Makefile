all:
	mkdir -p ./__output
	gcc -c -fPIC laxcore.c -o ./__output/laxcore.o
	gcc -shared -o ./__output/liblaxcore.so ./__output/laxcore.o

clean:
	rm -rf ./__output

