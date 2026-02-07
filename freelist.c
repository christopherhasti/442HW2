#include <stdlib.h>
#include <stdio.h>
#include "freelist.h"
#include "bbm.h"
#include "bm.h"
#include "utils.h"

typedef struct {
    void **heads;    // Heads of free lists
    BBM *bbms;       // Buddy bitmaps for merging
    BM *is_alloc;    // Track allocation starts/sizes
} FL;

extern FreeList freelistcreate(size_t size, int l, int u) {
    FL *f = malloc(sizeof(FL));
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
    FL *fl = (FL *)f;
    for (int i = l; i <= u; i++) {
        bbmdelete(fl->bbms[i]);
        bmdelete(fl->is_alloc[i]);
    }
    free(fl->heads); free(fl->bbms); free(fl->is_alloc); free(fl);
}

extern void *freelistalloc(FreeList f, void *base, int e, int l) {
    FL *fl = (FL *)f;
    int k = e;
    while (k <= 31 && !fl->heads[k]) k++; 
    if (k > 31 || !fl->heads[k]) return NULL;

    void *block = fl->heads[k];
    fl->heads[k] = *(void **)block;

    while (k > e) {
        k--;
        void *buddy = (char *)block + e2size(k);
        *(void **)buddy = fl->heads[k];
        fl->heads[k] = buddy;
        // Buddy System: Toggle bit for pair at order k
        if (bbmtst(fl->bbms[k], base, block, k)) bbmclr(fl->bbms[k], base, block, k);
        else bbmset(fl->bbms[k], base, block, k);
    }
    bmset(fl->is_alloc[e], ((char *)block - (char *)base) >> e);
    return block;
}

extern void freelistfree(FreeList f, void *base, void *mem, int e, int l) {
    FL *fl = (FL *)f;
    bmclr(fl->is_alloc[e], ((char *)mem - (char *)base) >> e);
    
    void *curr = mem;
    int k = e;
    
    // Merge buddies until reaching max size u or finding an allocated buddy
    while (k < 31 && fl->bbms[k]) {
        int was_buddy_free = bbmtst(fl->bbms[k], base, curr, k);
        if (was_buddy_free) {
            bbmclr(fl->bbms[k], base, curr, k);
            void *buddy = baddrinv(base, curr, k);
            
            // Remove buddy from free list
            void **p = &fl->heads[k];
            while (*p && *p != buddy) p = (void **)*p;
            if (*p) *p = *(void **)buddy;
            
            if (buddy < curr) curr = buddy;
            k++;
        } else {
            bbmset(fl->bbms[k], base, curr, k);
            break;
        }
    }
    *(void **)curr = fl->heads[k];
    fl->heads[k] = curr;
}

extern int freelistsize(FreeList f, void *base, void *mem, int l, int u) {
    FL *fl = (FL *)f;
    for (int i = l; i <= u; i++) {
        if (bmtst(fl->is_alloc[i], ((char *)mem - (char *)base) >> i)) return i;
    }
    return -1;
}

extern void freelistprint(FreeList f, int l, int u) {
    FL *fl = (FL *)f;
    for (int i = l; i <= u; i++) {
        printf("  Order %2d: ", i);
        void *curr = fl->heads[i];
        while (curr) { printf("%p ", curr); curr = *(void **)curr; }
        printf("\n");
    }
}