CC=clang
CFLAGS= -O2 -c -pipe -Wall -Wno-multichar -Wno-unused-function -std=c99

LDFLAGS= -lssl -lcrypto

all: opensn0w reimagine

opensn0w:
	cd opensn0w-X/src && make

reimagine: main.o helper.o opensn0w-X/src/image3.o opensn0w-X/src/util.o opensn0w-X/src/ibootsup.o opensn0w-X/src/patch.o opensn0w-X/src/kcache.o opensn0w-X/src/macho_loader.o
	$(CC) $(LDFLAGS) helper.o main.o opensn0w-X/src/image3.o opensn0w-X/src/util.o opensn0w-X/src/ibootsup.o opensn0w-X/src/patch.o opensn0w-X/src/kcache.o opensn0w-X/src/macho_loader.o -o reimagine

main.o: main.c
	$(CC) $(CFLAGS) main.c

helper.o: helper.c
	$(CC) $(CFLAGS) helper.c

clean:
	rm -f *o reimagine
	cd opensn0w-X/src && make clean
