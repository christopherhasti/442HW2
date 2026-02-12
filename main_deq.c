#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "deq.h"

int main() {
    printf("Testing Deq with Buddy Allocator Wrapper...\n");

    Deq q = deq_new();
    
    // Add some data
    deq_head_put(q, "First");
    deq_tail_put(q, "Last");
    deq_head_put(q, "NewHead");

    printf("Deque Length: %d\n", deq_len(q));
    assert(deq_len(q) == 3);

    // Verify data
    printf("Head: %s\n", (char *)deq_head_get(q)); // Should be NewHead
    printf("Tail: %s\n", (char *)deq_tail_get(q)); // Should be Last

    deq_del(q, NULL);
    
    printf("Deq test passed successfully using Buddy Allocator!\n");
    return 0;
}