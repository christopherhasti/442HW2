#include <stdlib.h>
#include <stdio.h>
#include "balloc.h"
#include "freelist.h"
#include "utils.h"

struct balloc_struct {
    void *base;
    size_t size;
    int l, u;
    FreeList fl;
};

typedef struct balloc_struct *Pool;

extern Balloc bcreate(unsigned int size, int l, int u) {
    Pool p = malloc(sizeof(struct balloc_struct));
    if (!p) return NULL;

    p->size = size;
    p->l = l;
    p->u = u;
    p->base = mmalloc(size); // Obtained via mmap
    
    if (p->base == (void *)-1) {
        free(p);
        return NULL;
    }
    
    p->fl = freelistcreate(size, l, u);
    
    // Initial memory setup: Split the pool into valid buddy blocks.
    // We must ensure 'curr' stays within the allocated 'size'.
    void *curr = p->base;
    size_t remaining = size;
    for (int i = u; i >= l; i--) {
        size_t bsize = e2size(i);
        while (remaining >= bsize) {
            // Add block to free list without triggering buddy merges 
            // that might look outside our allocated mmap range.
            freelistfree(p->fl, p->base, curr, i, l);
            curr = (char*)curr + bsize;
            remaining -= bsize;
        }
    }
    return (Balloc)p;
}

extern void bdelete(Balloc pool) {
    Pool p = (Pool)pool;
    if (!p) return;
    freelistdelete(p->fl, p->l, p->u);
    mmfree(p->base, p->size);
    free(p);
}

extern void *balloc(Balloc pool, unsigned int size) {
    Pool p = (Pool)pool;
    int e = size2e(size);
    if (e < p->l) e = p->l;
    if (e > p->u) return NULL;
    return freelistalloc(p->fl, p->base, e, p->l);
}

extern void bfree(Balloc pool, void *mem) {
    if (!mem) return;
    Pool p = (Pool)pool;
    int e = freelistsize(p->fl, p->base, mem, p->l, p->u);
    if (e != -1) {
        freelistfree(p->fl, p->base, mem, e, p->l);
    }
}

extern unsigned int bsize(Balloc pool, void *mem) {
    Pool p = (Pool)pool;
    int e = freelistsize(p->fl, p->base, mem, p->l, p->u);
    return (e == -1) ? 0 : e2size(e);
}

extern void bprint(Balloc pool) {
    Pool p = (Pool)pool;
    printf("Allocator Pool %p (Base: %p, Size: %zu)\n", (void*)p, p->base, p->size);
    freelistprint(p->fl, p->l, p->u);
}