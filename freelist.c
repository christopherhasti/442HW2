#include <stdlib.h>
#include <stdio.h>
#include "freelist.h"
#include "bbm.h"
#include "bm.h"
#include "utils.h"

typedef struct freelist_s {
    void **heads;    // Heads of free lists for each order
    BBM *bbms;       // Buddy bitmaps
    BM *is_alloc;    // Track if a block is currently allocated
} *FL;

extern FreeList freelistcreate(size_t size, int l, int u) {
    FL f = malloc(sizeof(struct freelist_s));
    f->heads = calloc(u + 1, sizeof(void *));
    f->bbms = calloc(u + 1, sizeof(BBM));
    f->is_alloc = calloc(u + 1, sizeof(BM));
    
    for (int i = l; i <= u; i++) {
        f->bbms[i] = bbmcreate(size, i);
        f->is_alloc[i] = bmcreate(divup(size, e2size(i)));
    }
    return (FreeList)f;
}

extern void freelistdelete(FreeList f, int l, int u) {
    FL fl = (FL)f;
    for (int i = l; i <= u; i++) {
        bbmdelete(fl->bbms[i]);
        bmdelete(fl->is_alloc[i]);
    }
    free(fl->heads);
    free(fl->bbms);
    free(fl->is_alloc);
    free(fl);
}

extern void *freelistalloc(FreeList f, void *base, int e, int l) {
    FL fl = (FL)f;
    int k = e;
    while (k <= 31 && fl->heads[k] == NULL) k++;
    if (k > 31 || !fl->heads[k]) return NULL;

    void *block = fl->heads[k];
    fl->heads[k] = *(void **)block;

    // Split blocks if we found a larger one
    while (k > e) {
        k--;
        void *buddy = block + e2size(k);
        *(void **)buddy = fl->heads[k];
        fl->heads[k] = buddy;
        // Toggling buddy bit: use bbmset/clr to indicate one is now used
        if (bbmtst(fl->bbms[k], base, block, k)) bbmclr(fl->bbms[k], base, block, k);
        else bbmset(fl->bbms[k], base, block, k);
    }
    
    bmset(fl->is_alloc[e], ((char*)block - (char*)base) >> e);
    return block;
}

extern void freelistfree(FreeList f, void *base, void *mem, int e, int l) {
    FL fl = (FL)f;
    // Mark as not allocated
    bmclr(fl->is_alloc[e], ((char*)mem - (char*)base) >> e);

    void *curr = mem;
    int k = e;

    // Merge buddies until we can't anymore
    while (k < 31 && fl->bbms[k]) {
        // baddrinv calculates the buddy address by flipping the k-th bit
        void *buddy = baddrinv(base, curr, k);
        
        // SAFETY CHECK: Ensure buddy is within our pool's range
        // If the pool size isn't a power of two, the 'calculated' buddy might be invalid.
        // We use the buddy bit (bbmtst) to see if the buddy is free.
        if (!bbmtst(fl->bbms[k], base, curr, k)) {
            // Buddy bit is 0, meaning the buddy is NOT available for merging.
            // So we mark our current block's availability by flipping the bit to 1.
            bbmset(fl->bbms[k], base, curr, k);
            break;
        }

        // If we reach here, buddy is free. Clear the bit and merge.
        bbmclr(fl->bbms[k], base, curr, k);
        
        // Remove buddy from its current free list
        void **p = &fl->heads[k];
        while (*p && *p != buddy) p = (void **)*p;
        if (*p) {
            *p = *(void **)buddy;
        } else {
            // If buddy isn't in the list but bit was set, something is wrong.
            // Flip bit back and stop merging to prevent segfault.
            bbmset(fl->bbms[k], base, curr, k);
            break;
        }
        
        if (buddy < curr) curr = buddy;
        k++;
    }
    
    // Add the (potentially merged) block to the free list
    *(void **)curr = fl->heads[k];
    fl->heads[k] = curr;
}

extern int freelistsize(FreeList f, void *base, void *mem, int l, int u) {
    FL fl = (FL)f;
    for (int i = l; i <= u; i++) {
        if (bmtst(fl->is_alloc[i], ((char*)mem - (char*)base) >> i)) return i;
    }
    return -1;
}

extern void freelistprint(FreeList f, int l, int u) {
    FL fl = (FL)f;
    for (int i = l; i <= u; i++) {
        printf("Order %d: ", i);
        void *curr = fl->heads[i];
        while (curr) {
            printf("[%p] ", curr);
            curr = *(void **)curr;
        }
        printf("\n");
    }
}