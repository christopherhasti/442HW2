// Purpose: Handles low-level OS interaction and provides mathematical primitives for power-of-two calculations.
//
// Memory Acquisition:
// - mmalloc: The exclusive interface for requesting memory from the OS via mmap. It uses PROT_READ|PROT_WRITE and MAP_ANONYMOUS to provide a private, zero-initialized memory region.
// - mmfree: Releases the mmap'd region back to the kernel.
// Math Helpers:
// - size2e: Converts a byte size into the smallest exponent e such that 2^e >= size.
// - e2size: Computes 2^e using bit-shifting.
// Bitwise Primitives: Provides the raw logic for toggling and testing bits within a byte array.

#include <sys/mman.h>
#include <stdlib.h>
#include "utils.h"

extern void *mmalloc(size_t size) {
    void *p = mmap(0, size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    return (p == MAP_FAILED) ? (void *)-1 : p;
}

extern void mmfree(void *p, size_t size) {
    munmap(p, size);
}

extern size_t e2size(int e) {
    return (size_t)1 << e;
}

extern int size2e(size_t size) {
    int e = 0;
    while (e2size(e) < size) e++;
    return e;
}

extern size_t divup(size_t n, size_t d) { return (n + d - 1) / d; }
extern size_t bits2bytes(size_t bits) { return divup(bits, bitsperbyte); }

extern void bitset(void *p, int bit) { ((char*)p)[bit/8] |= (1 << (bit%8)); }
extern void bitclr(void *p, int bit) { ((char*)p)[bit/8] &= ~(1 << (bit%8)); }
extern void bitinv(void *p, int bit) { ((char*)p)[bit/8] ^= (1 << (bit%8)); }
extern int bittst(void *p, int bit) { return (((char*)p)[bit/8] >> (bit%8)) & 1; }