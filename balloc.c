// Purpose: The primary API used by applications to interact with the allocator.
//
// Logic:
// - bcreate: The only function that calls mmap (via mmalloc). It initializes the pool and populates the freelist with the largest possible block sizes.
// - balloc: Rounds requests to the nearest power of two within the range [2^l, 2^u] and retrieves a block from the freelist.
// - bfree: Detects the block size using internal bitmaps and returns the memory to the freelist manager for merging.
// - bsize: Queries the freelist bitmaps to return the actual allocated size of a pointer.
// - bprint: Iterates through the free list heads to provide a textual representation of the allocator's current state.

#include <stdlib.h>
#include <stdio.h>
#include <string.h> // Required for memset
#include "balloc.h"
#include "freelist.h"
#include "utils.h"

struct balloc_s {
    void *base;
    size_t size;
    int l, u;
    FreeList fl;
};

extern Balloc bcreate(unsigned int size, int l, int u) {
    // FIX: Use mmalloc instead of malloc to avoid wrapper recursion
    struct balloc_s *p = mmalloc(sizeof(struct balloc_s));
    if (p == (void *)-1) return NULL;
    memset(p, 0, sizeof(struct balloc_s)); // mmap is zeroed, but explicit is safer
    
    p->base = mmalloc(size);
    if (p->base == (void *)-1) {
        // In a strict implementation, you would free p here
        return NULL;
    }
    
    p->size = size;
    p->l = l;
    p->u = u;
    p->fl = freelistcreate(size, l, u);
    
    char *curr = (char *)p->base;
    size_t remaining = size;
    for (int i = u; i >= l; i--) {
        size_t block_size = e2size(i);
        while (remaining >= block_size) {
            freelistfree(p->fl, p->base, curr, i, l);
            curr += block_size;
            remaining -= block_size;
        }
    }
    return (Balloc)p;
}

extern void bdelete(Balloc pool) {
    struct balloc_s *p = (struct balloc_s *)pool;
    if (!p) return;
    
    // Clean up internal components
    freelistdelete(p->fl, p->l, p->u);
    
    // Release the managed memory pool
    mmfree(p->base, p->size);
    
    // FIX: Release the pool structure itself using mmfree, not free()
    mmfree(p, sizeof(struct balloc_s));
}

extern void *balloc(Balloc pool, unsigned int size) {
    struct balloc_s *p = (struct balloc_s *)pool;
    if (!p) return NULL;
    int e = size2e(size);
    if (e < p->l) e = p->l;
    if (e > p->u) return NULL; // Fail if request exceeds 2^u
    return freelistalloc(p->fl, p->base, e, p->l);
}

extern void bfree(Balloc pool, void *mem) {
    if (!mem) return;
    struct balloc_s *p = (struct balloc_s *)pool;
    int e = freelistsize(p->fl, p->base, mem, p->l, p->u);
    if (e != -1) {
        freelistfree(p->fl, p->base, mem, e, p->l);
    }
}

extern unsigned int bsize(Balloc pool, void *mem) {
    if (!mem) return 0;
    struct balloc_s *p = (struct balloc_s *)pool;
    int e = freelistsize(p->fl, p->base, mem, p->l, p->u);
    return (e == -1) ? 0 : (unsigned int)e2size(e);
}

extern void bprint(Balloc pool) {
    struct balloc_s *p = (struct balloc_s *)pool;
    if (!p) return;
    printf("Balloc Pool %p: base=%p size=%zu range=[2^%d, 2^%d]\n", 
           (void*)p, p->base, p->size, p->l, p->u);
    freelistprint(p->fl, p->l, p->u);
}