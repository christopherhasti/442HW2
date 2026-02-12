// Purpose: Provides a dynamic, heap-allocated bitmap structure for tracking arbitrary bit-level states.
//
// Logic:
// - Allocates a memory block where the first few bytes store metadata (the total bit count) followed by the raw bit data.
// - bmcreate: Uses mmalloc to obtain memory and initializes all bits to zero.
// - bmset / bmclr / bmtst: Provides safe access to individual bits. It includes bounds checking (ok function) to ensure indices do not exceed the bitmap size.
// - bmdelete: Frees the memory acquired during creation.

#include <stdlib.h>
#include <string.h>

#include "bm.h"
#include "utils.h"

static size_t bmbits(BM b) { size_t *bits=b; return *--bits; }

static size_t bmbytes(BM b) { return bits2bytes(bmbits(b)); }

static void ok(BM b, size_t i) {
  if (i<bmbits(b))
    return;
  fprintf(stderr,"bitmap index out of range\n");
  exit(1);
}         

extern BM bmcreate(size_t bits) {
  size_t bytes=bits2bytes(bits);
  size_t *p=mmalloc(sizeof(size_t)+bytes);
  if ((long)p==-1)
    return 0;
  *p=bits;
  BM b=++p;
  memset(b,0,bytes);
  return b;
}

extern void bmdelete(BM b) {
  size_t *p=b;
  p--;
  mmfree(p,sizeof(size_t)+bits2bytes(*p));
}

extern void bmset(BM b, size_t i) {
  ok(b,i); bitset(b+i/bitsperbyte,i%bitsperbyte);
}

extern void bmclr(BM b, size_t i) {
  ok(b,i); bitclr(b+i/bitsperbyte,i%bitsperbyte);
}

extern int bmtst(BM b, size_t i) {
  ok(b,i); return bittst(b+i/bitsperbyte,i%bitsperbyte);
}

extern void bmprt(BM b) {
  for (int byte=bmbytes(b)-1; byte>=0; byte--)
    printf("%02x%s",((char *)b)[byte],(byte ? " " : "\n"));
}
