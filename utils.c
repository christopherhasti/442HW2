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