CPPFLAGS=-I../include -std=c99
CFLAGS= -O2 -pipe -Wno-pointer-to-int-cast -Wno-unused-result -Wno-multichar -Wno-implicit-function-declaration -Wno-int-conversion -Wno-address-of-packed-member -Wno-int-to-pointer-cast -Wno-discarded-qualifiers -Wno-unused-function -D__target_arm__
TOOLS=iboot_patcher kernel_patcher image3_loader
IBOOT_PATCHER_OBJECTS=ibootsup.o patch.o util.o iboot_patcher.o
KERNEL_PATCHER_OBJECTS=patch.o util.o kcache.o macho_loader.o kernel_patcher.o
IMAGE3_PATCHER_OBJECTS=image3.o image3_mapper.o util.o

all: $(TOOLS)

image3_loader: $(IMAGE3_PATCHER_OBJECTS)
	$(CC) $(CFLAGS) $(IMAGE3_PATCHER_OBJECTS) -o $@

iboot_patcher: $(IBOOT_PATCHER_OBJECTS)
	$(CC) $(CFLAGS) $(IBOOT_PATCHER_OBJECTS) -o $@

kernel_patcher: $(KERNEL_PATCHER_OBJECTS)
	$(CC) $(CFLAGS) $(KERNEL_PATCHER_OBJECTS) -o $@

%.o: %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

clean:
	rm -f $(TOOLS) *.o
