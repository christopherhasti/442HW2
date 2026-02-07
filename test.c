#include <stdio.h>
#include <assert.h>
#include "balloc.h"

int main() {
    printf("Starting Buddy System Tests...\n");
    
    Balloc pool = bcreate(65536, 4, 12); // 64KB, 16B min, 4KB max
    assert(pool != NULL);

    // Test Allocation and Size
    void *p1 = balloc(pool, 10); // Should get 2^4 = 16 bytes
    assert(bsize(pool, p1) == 16);
    
    void *p2 = balloc(pool, 4000); // Should get 2^12 = 4096 bytes
    assert(bsize(pool, p2) == 4096);

    // Test Boundary Failure
    void *p3 = balloc(pool, 5000); // Exceeds 2^u
    assert(p3 == NULL);

    // Test Free and Re-allocation
    bfree(pool, p1);
    void *p4 = balloc(pool, 16);
    assert(p4 == p1); // Should reuse same block

    bprint(pool);
    bdelete(pool);
    
    printf("All tests passed!\n");
    return 0;
}