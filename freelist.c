// Purpose: Manages the lists of available memory blocks and implements the splitting and merging logic.
//
// Logic:
// - Data Structures: Maintains an array of list heads, one for each possible power-of-two order from l to u.
// - Management Data: Management pointers (linked list pointers) are stored at the beginning of free blocks themselves, ensuring no extra memory is wasted in allocated blocks.
// - Splitting (Allocation): If a requested order is empty, the module searches higher orders for a block. When a larger block is found, it is recursively split into "buddies" until the requested size is reached.
// - Merging (Deallocation): When a block is freed, the module uses buddy bitmaps to check if the adjacent buddy is also free. If it is, the buddies are coalesced into a single larger block, and the process repeats for the next higher order.

#include <stdlib.h>
#include <stdio.h>
#include "freelist.h"
#include "bbm.h"
#include "bm.h"
#include "utils.h"

// Added l and u to store the boundaries of this specific allocator
typedef struct freelist_s {
    void **heads;    // Heads of free lists for each order
    BBM *bbms;       // Buddy bitmaps
    BM *is_alloc;    // Track if a block is currently allocated
    int l, u;        // Bounds
} *FL;

extern FreeList freelistcreate(size_t size, int l, int u) {
    FL f = malloc(sizeof(struct freelist_s));
    f->l = l;
    f->u = u;
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
    while (k <= fl->u && fl->heads[k] == NULL) k++;
    if (k > fl->u || !fl->heads[k]) return NULL;

    void *block = fl->heads[k];
    fl->heads[k] = *(void **)block;

    // Split blocks if we found a larger one
    while (k > e) {
        k--;
        void *buddy = (char*)block + e2size(k);
        *(void **)buddy = fl->heads[k];
        fl->heads[k] = buddy;
        
        // Buddy System logic: Toggling buddy bit
        if (bbmtst(fl->bbms[k], base, block, k)) bbmclr(fl->bbms[k], base, block, k);
        else bbmset(fl->bbms[k], base, block, k);
    }
    
    bmset(fl->is_alloc[e], ((char*)block - (char*)base) >> e);
    return block;
}

extern void freelistfree(FreeList f, void *base, void *mem, int e, int l) {
    FL fl = (FL)f;
    bmclr(fl->is_alloc[e], ((char*)mem - (char*)base) >> e);

    void *curr = mem;
    int k = e;

    // Merge buddies until we can't anymore (limited by fl->u)
    while (k < fl->u && fl->bbms[k]) {
        void *buddy = baddrinv(base, curr, k);
        
        // Toggling bit: If bit was 0, it means the buddy is allocated.
        // We set bit to 1 and stop merging. If bit was 1, buddy is free; we merge.
        if (!bbmtst(fl->bbms[k], base, curr, k)) {
            bbmset(fl->bbms[k], base, curr, k);
            break;
        }

        // Buddy is free. Clear the bit and remove buddy from its current free list.
        bbmclr(fl->bbms[k], base, curr, k);
        
        void **p = &fl->heads[k];
        while (*p && *p != buddy) p = (void **)*p;
        if (*p) {
            *p = *(void **)buddy;
        } else {
            // Safety: Bit indicated free but buddy not found in list
            bbmset(fl->bbms[k], base, curr, k);
            break;
        }
        
        if (buddy < curr) curr = buddy;
        k++;
    }
    
    // Add the merged block to the free list
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
        printf("Order %2d: ", i);
        void *curr = fl->heads[i];
        while (curr) {
            printf("[%p] ", curr);
            curr = *(void **)curr;
        }
        printf("\n");
    }
}