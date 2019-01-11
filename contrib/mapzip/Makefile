all: mapzip

clean:
	rm mapzip mapzip.o

mapzip: mapzip.o
	gcc -o mapzip mapzip.o -lz

mapzip.o: mapzip.c
	gcc -g -c mapzip.c
