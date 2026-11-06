#ifndef __HELPER_H
#define __HELPER_H

#include <stdint.h>
#include <stddef.h>

#define add_ptr2(x, y) ((uintptr_t)((uintptr_t)x + (uintptr_t)y))

int offset(int a, int b);
void hexToInts(const char* hex, unsigned int** buffer, size_t* bytes);
void hexdump(void *mem, unsigned int len);

#endif
