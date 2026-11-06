CC=clang

BREW := $(shell command -v brew 2>/dev/null || echo /opt/homebrew/bin/brew)
export PKG_CONFIG_PATH := $(shell $(BREW) --prefix openssl@3 2>/dev/null)/lib/pkgconfig:$(PKG_CONFIG_PATH)

CFLAGS= -O2 -c -pipe -Wall -Wno-multichar -Wno-unused-function -std=c99 $(shell pkg-config --cflags openssl)

LDFLAGS= $(shell pkg-config --libs openssl)

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
