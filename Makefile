CC=clang

CFLAGS= -O2 -c -pipe -Wall -Wno-multichar -Wno-unused-function -Wno-pointer-to-int-cast -Wno-int-conversion -Wno-int-to-pointer-cast -std=c99

OBJECTS= main.o helper.o image3.o util.o kcache.o aes.o

reimagine: $(OBJECTS)
	$(CC) $(OBJECTS) -o reimagine

%.o: %.c
	$(CC) $(CFLAGS) $<

clean:
	rm -f *.o reimagine
