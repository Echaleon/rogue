#include <stdlib.h>

#include "stack.h"
#include "helpers.h"

#include "Settings/misc-settings.h"

// See stack.h
Stack_T *new_stack() {
    Stack_T *s;
    s = safe_malloc(sizeof(Stack_T));
    s->capacity = DEFAULT_STACK_SIZE;
    s->top = -1;
    s->array = safe_malloc(DEFAULT_STACK_SIZE * sizeof(void *));
    return s;
}

// See stack.h
void stack_push(Stack_T *s, void *data) {

    // If our stack is at capacity, grow it by DEFAULT_STACK_SIZE capacity
    if (s->top == s->capacity - 1) {
        s->capacity += DEFAULT_STACK_SIZE;
        s->array = safe_realloc(s->array, (s->capacity) * sizeof(void *));
    }

    // Put the data on the top of the stack
    s->top++;
    s->array[s->top] = data;
}

// See stack.h
void *stack_pop(Stack_T *s) {

    // Make sure our stack is not empty
    if (s->top == -1) {
        return NULL;
    }

    // Return the data and increment our stack
    s->top--;
    return s->array[s->top + 1];
}

// See stack.h
void *stack_peek(const Stack_T *s) {

    // Make sure our stack is not empty
    if (s->top == -1) {
        return NULL;
    }

    return s->array[s->top];
}

// See stack.h
void cleanup_stack(Stack_T *s) {
    while (s->top != -1) {
        free(stack_pop(s));
    }
    free(s->array);
    free(s);
}


