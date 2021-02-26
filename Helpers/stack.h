#ifndef ROGUE_STACK_H
#define ROGUE_STACK_H

// Contains an array based stack that will dynamically grow as it needs to. By using a void pointer, the stack can store
// any data, but it must be casted and the programmer is responsible for keeping track of what type of data is stored.
typedef struct Stack_S {
    int top;
    int capacity;
    void **array;
} Stack_T;

// Returns a pointer to a new stack with DEFAULT_STACK_CAPACITY
Stack_T *new_stack();

// Push an element onto a stack
void stack_push(Stack_T *s, void *data);

// Get an element from the stack
void *stack_pop(Stack_T *s);

// Just look at the top of the stack without removing it
void *stack_peek(const Stack_T *s);

// Frees any data still in the stack, then cleans up the stack and itself.
void cleanup_stack(Stack_T *s);

#endif //ROGUE_STACK_H
