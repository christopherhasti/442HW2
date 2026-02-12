#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "deq.h"
#include "error.h"

/**
 * IMPLEMENTATION STRATEGY: Symmetric Doubly-Linked List
 * * To satisfy the requirement of "pairwise symmetric operations" without 
 * code duplication, we use an array of pointers `np[Ends]` in each node.
 * * - `Ends` is an enum {Head, Tail}.
 * - `np[e]` (where e is Head or Tail) represents the "OUTWARD" link 
 * (towards the `e` end of the list).
 * - `np[1-e]` represents the "INWARD" link (towards the center or other end).
 * * This allows us to write generic helper functions (put, get, etc.) that 
 * take an `End` parameter. Calling `put(r, Head, ...)` performs the exact 
 * symmetric logic to `put(r, Tail, ...)` simply by swapping the indices.
 */

// indices and size of array of node pointers
typedef enum { Head, Tail, Ends } End;

typedef struct Node {
  struct Node *np[Ends]; // np[Head] points to Head-ward neighbor, np[Tail] to Tail-ward
  Data data;
} *Node;

typedef struct {
  Node ht[Ends]; // ht[Head] points to the Head node, ht[Tail] points to the Tail node
  int len;
} *Rep;

static Rep rep(Deq q) {
  if (!q)
    ERROR("zero pointer");
  return (Rep)q;
}

/**
 * put: Add data 'd' to the end 'e'.
 * logic: Allocates node, links its "inward" pointer to old end, 
 * updates old end's "outward" pointer to new node.
 */
static void put(Rep r, End e, Data d) {
  Node n = (Node)malloc(sizeof(*n));
  if (!n)
    ERROR("malloc() failed");
  n->data = d;
  n->np[e] = 0;          // New node is at the edge, so outward is 0
  n->np[1 - e] = r->ht[e]; // Points "inward" to current end

  if (r->len == 0) {
    r->ht[Head] = n;
    r->ht[Tail] = n;
  } else {
    r->ht[e]->np[e] = n; // Old end points "outward" to new node
    r->ht[e] = n;        // Update head/tail pointer
  }
  r->len++;
}

/**
 * ith: Retrieve the i-th element starting from end 'e'.
 * logic: Traverses "inward" 'i' times.
 */
static Data ith(Rep r, End e, int i) {
  if (i < 0 || i >= r->len)
    ERROR("index out of bounds");
  Node curr = r->ht[e];
  while (i > 0) {
    curr = curr->np[1 - e]; // Move "inward"
    i--;
  }
  return curr->data;
}

/**
 * get: Remove and return data from end 'e'.
 * logic: removing the last node updates both Head/Tail to 0.
 * Otherwise, updates the specific end pointer to move "inward".
 */
static Data get(Rep r, End e) {
  if (r->len == 0)
    ERROR("get from empty deque");
  Node n = r->ht[e];
  Data d = n->data;

  if (r->len == 1) {
    r->ht[Head] = 0;
    r->ht[Tail] = 0;
  } else {
    r->ht[e] = n->np[1 - e]; // Move end pointer "inward"
    r->ht[e]->np[e] = 0;     // New end has no "outward" neighbor
  }
  free(n);
  r->len--;
  return d;
}

/**
 * rem: Remove the first occurrence of 'd' starting search from end 'e'.
 * logic: Traverses "inward". If found, relinks neighbors to bypass current node.
 */
static Data rem(Rep r, End e, Data d) {
  Node curr = r->ht[e];
  while (curr) {
    if (curr->data == d) { // Found it (pointer comparison)
      // Unlink
      Node prev = curr->np[e];     // Outward neighbor
      Node next = curr->np[1 - e]; // Inward neighbor

      if (prev)
        prev->np[1 - e] = next;
      else
        r->ht[e] = next; // Was the end node

      if (next)
        next->np[e] = prev;
      else
        r->ht[1 - e] = prev; // Was the other end node (list became empty or singleton)

      Data ret = curr->data;
      free(curr);
      r->len--;
      return ret;
    }
    curr = curr->np[1 - e]; // Continue inward
  }
  return 0; // Not found
}

extern Deq deq_new() {
  Rep r = (Rep)malloc(sizeof(*r));
  if (!r)
    ERROR("malloc() failed");
  r->ht[Head] = 0;
  r->ht[Tail] = 0;
  r->len = 0;
  return r;
}

extern int deq_len(Deq q) { return rep(q)->len; }

extern void deq_head_put(Deq q, Data d) { put(rep(q), Head, d); }
extern Data deq_head_get(Deq q) { return get(rep(q), Head); }
extern Data deq_head_ith(Deq q, int i) { return ith(rep(q), Head, i); }
extern Data deq_head_rem(Deq q, Data d) { return rem(rep(q), Head, d); }

extern void deq_tail_put(Deq q, Data d) { put(rep(q), Tail, d); }
extern Data deq_tail_get(Deq q) { return get(rep(q), Tail); }
extern Data deq_tail_ith(Deq q, int i) { return ith(rep(q), Tail, i); }
extern Data deq_tail_rem(Deq q, Data d) { return rem(rep(q), Tail, d); }

extern void deq_map(Deq q, DeqMapF f) {
  // Map always traverses Head -> Tail
  for (Node n = rep(q)->ht[Head]; n; n = n->np[Tail])
    f(n->data);
}

extern void deq_del(Deq q, DeqMapF f) {
  if (f)
    deq_map(q, f);
  Node curr = rep(q)->ht[Head];
  while (curr) {
    Node next = curr->np[Tail];
    free(curr);
    curr = next;
  }
  free(q);
}

extern Str deq_str(Deq q, DeqStrF f) {
  char *s = strdup("");
  for (Node n = rep(q)->ht[Head]; n; n = n->np[Tail]) {
    char *d = f ? f(n->data) : n->data;
    char *t;
    asprintf(&t, "%s%s%s", s, (*s ? " " : ""), d);
    free(s);
    s = t;
    if (f)
      free(d);
  }
  return s;
}