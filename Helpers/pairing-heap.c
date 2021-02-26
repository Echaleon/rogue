#include <stdlib.h>

#include "pairing-heap.h"
#include "helpers.h"
#include "../Settings/exit-codes.h"

// Helper to merge two trees together.
static Heap_Node_T *merge(Heap_Node_T *a, Heap_Node_T *b) {
    Heap_Node_T *n;

    // Make sure to just return if either node is empty
    if (a == NULL || b == NULL) {
        return a == NULL ? b : a;
    }

    // Swap so we can reuse code
    if (a->key > b->key) {
        n = a;
        a = b;
        b = n;
    }

    // b is smaller than a, so a must stay the root. Set b's parent to be a
    b->prev = a;

    // Integrate b into the rest of a
    b->next = a->child;

    // If child is not an orphan, makes the new child be the right sibling of the old root
    if (a->child != NULL) {
        a->child->prev = b;
    }

    // Set the new child to be b
    a->child = b;

    // a is the new root at the top. Only has a child in b.
    a->prev = NULL;
    a->next = NULL;

    return a;
}

// Helper to splice a tree out of another.
static void remove_siblings(Heap_Node_T *n) {

    // Check if it's the first node in a child list
    if (n->prev->child == n) {

        // Since it's the first node, we need to set the parent node to have a new child node
        n->prev->child = n->next;

        // As long as the node wasn't an orphan, we need to splice out the next node's link to it
        if (n->next != NULL) {
            n->next->prev = n->prev;
        }

    } else {

        // Break the previous node's link to it
        n->prev->next = n->next;

        // Break the next node's link, only if this node is not the last one in the list
        if (n->next != NULL) {
            n->next->prev = n->prev;
        }
    }

    // Break left and right links for this node
    n->prev = NULL;
    n->next = NULL;
}

// Helper to make sure our heap satisfies the constraints to be a pairing heap. After a particularly destructive
// operation, this helps collapse the heap to one that is far far more efficient.
static Heap_Node_T *two_pass_merge(Heap_Node_T *root) {
    Heap_Node_T *node, *next, *list;

    // Check if the node is null or its child is null
    if (root == NULL || root->child == NULL) {
        return NULL;
    }

    // Initialize all of our variables
    node = root->child;
    root->child = NULL;
    list = NULL;
    next = node->next;

    // First run through with the merge
    while (next != NULL) {
        Heap_Node_T *tmp;

        // Store the next node
        tmp = next->next;

        // Merge the nodes together
        node = merge(node, next);

        // Insert our node into the head of the list
        node->next = list;
        list = node;

        // Set our node to be the next
        node = tmp;

        // Iterate to the next node if we can
        next = node == NULL ? NULL : node->next;
    }

    // Deal with the child being an orphan by inserting it to the head of our list for the second pass
    if (node != NULL) {
        node->next = list;
        list = node;
    }

    // Make a second pass with the merge
    while (list->next != NULL) {
        Heap_Node_T *tmp;

        // Store the next node
        tmp = list->next->next;

        // Merge our list together
        list = merge(list, list->next);

        // Set up our list to be the next
        list->next = tmp;
    }

    // List is now the root node
    list->prev = NULL;
    list->next = NULL;

    return list;
}

// See pairing-heap.h
Heap_T *new_heap(bool intrusive) {
    Heap_T *h;
    h = safe_malloc(sizeof(Heap_T));
    h->size = 0;
    h->intrusive = intrusive;
    h->root = NULL;
    return h;
}

// See pairing-heap.h
Heap_Node_T *heap_dynamic_insert(Heap_T *h, int key, void *data) {
    Heap_Node_T *n;

    // Check to make sure the caller is using the function right.
    if (h->intrusive) {
        kill(INVALID_STATE, "FATAL ERROR! HEAP IS INTRUSIVE AND DYNAMIC INSERT CANNOT BE USED!");
    }

    // Allocate space for our new node
    n = safe_malloc(sizeof(Heap_Node_T));

    // Set our node parameters
    n->key = key;
    n->data = data;
    n->child = NULL;
    n->next = NULL;
    n->prev = NULL;

    // Merge the nodes together
    h->root = merge(h->root, n);
    h->size++;

    return n;
}

// See pairing-heap.h
void heap_intrusive_insert(Heap_T *h, Heap_Node_T *n, int key, void *data) {

    // Check to make sure the caller is using the function right.
    if (!h->intrusive) {
        kill(INVALID_STATE, "FATAL ERROR! HEAP IS DYNAMIC AND INTRUSIVE INSERT CANNOT BE USED!");
    }

    // Set our node parameters
    n->key = key;
    n->data = data;
    n->child = NULL;
    n->next = NULL;
    n->prev = NULL;

    // Merge the nodes together
    h->root = merge(h->root, n);
    h->size++;
}

// See pairing-heap.h
void heap_delete(Heap_T *h, Heap_Node_T *n) {
    Heap_Node_T *tmp;

    // Check if it's the root node so we can use the faster heap_move_min()
    if (n->prev == NULL) {
        heap_remove_min(h);
        return;
    }

    // Isolate our node
    remove_siblings(n);

    // Expensive merges to reset our heap to be valid
    tmp = two_pass_merge(n);
    h->root = merge(h->root, tmp);
    h->size--;
}

// See pairing-heap.h
Heap_Node_T *heap_min(const Heap_T *h) {
    return h->size == 0 ? NULL : h->root;
}

// See pairing-heap.h
Heap_Node_T *heap_remove_min(Heap_T *h) {
    Heap_Node_T *tmp;

    // Check if our heap size is zero
    if (h->size == 0) {
        return NULL;
    }

    // Store our root node, since two_pass_merge() will remove it
    tmp = h->root;

    // Actually remove the root node from the heap
    h->root = two_pass_merge(tmp);
    h->size--;

    return tmp;
}

// See pairing-heap.h
void heap_decrease_key(Heap_T *h, Heap_Node_T *n, int key) {

    n->key = key;

    // Check if node is not root
    if (n->prev != NULL) {
        remove_siblings(n);
        h->root = merge(h->root, n);
    }
}

// See pairing-heap.h
void cleanup_heap(Heap_T *h) {
    while (h->size > 0) {
        if (!h->intrusive) {
            free(heap_remove_min(h));
        } else {
            heap_remove_min(h);
        }
    }
    free(h);
}
