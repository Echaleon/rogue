#ifndef ROGUE_PAIRING_HEAP_H
#define ROGUE_PAIRING_HEAP_H

#include <stdbool.h>

// This is an implementation of a pairing heap -- while theoretically slower than a Fibonacci, rank pairing or Brodal
// heap, it is often much faster in practice due to better time constants. There are two ways to use it: on its own or
// as an instructive data structure, where a wrapper class contains a child heap node. The first way will dynamically
// allocate its own nodes, while the latter while require the caller to maintain its own allocations. The first way is
// more flexible, but the second will be faster, as it can be made to make far fewer calls to malloc().
//
// Either way, a heap can only be used one way or the other at a time, and the heap will kill the program if an illegal
// operation is committed. This only comes into play in the two insert methods. A heap will keep track if it's dynamic
// or intrusive, and the corresponding insert method must be used.

// Have to declare since the struct contains heap nodes
typedef struct Heap_Node_S Heap_Node_T;

// Struct that actually makes up the heap. Maintains a key, which is used in comparisons with the minimum in the heap
// always being at the top, as well as maintaining pointers to the data it represents, and other parts of the heap
struct Heap_Node_S {
    int key;
    void *data;
    Heap_Node_T *prev, *next, *child;
};

// Stores our actual heap. Size is always accurate to the number of nodes in the heap, and intrusive represents if nodes
// are preallocated or should allocated by the heap itself.
typedef struct Heap_S {
    int size;
    bool intrusive;
    Heap_Node_T *root;
} Heap_T;

// Returns a new, empty heap
Heap_T *new_heap(bool intrusive);

// Insert a new node into the heap, which will be dynamically allocated internally, with key, and a pointer to the data.
Heap_Node_T *heap_dynamic_insert(Heap_T *h, int key, void *data);

// Insert a new node into the heap, passing in an uninitialized node that's already allocated. This is useful for pools
// where the memory is already allocated, like in the Dijkstra methods, or for using it as an intrusive structure. Most
// likely data should point to its parent struct, which makes it easier to get back out... otherwise we need really ugly
// macros and offsets.
void heap_intrusive_insert(Heap_T *h, Heap_Node_T *n, int key, void *data);

// Deletes a node from the tree. This is an expensive operation compared to the others.
void heap_delete(Heap_T *h, Heap_Node_T *n);

// Returns the minimum node
Heap_Node_T *heap_min(const Heap_T *h);

// Removes the minimum node from the heap
Heap_Node_T *heap_remove_min(Heap_T *h);

// Change the key for a node
void heap_decrease_key(Heap_T *h, Heap_Node_T *n, int key);

// Frees all the nodes in a heap, if they still exist and if the heap is dynamic, but does not free the data
// pointed to by any nodes. Otherwise it will make sure every node only points to NULL
void cleanup_heap(Heap_T *h);

#endif //ROGUE_PAIRING_HEAP_H
