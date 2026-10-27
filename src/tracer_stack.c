#include "tracer.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

struct tracer_stack {
    stack_t* parent;
    size_t varcount;
    struct tracer_offset {
        struct tracer_offset* next;
        char* typename;
        char* varname;
        int offset;
    }* offsets;
};


static struct tracer_offset* tracerStackPopOffset(
    struct tracer_offset* var,
    char** ptypename, 
    char** pvarname,
    int* offset
) {
    if (var == NULL) return NULL;
    struct tracer_offset* next = var->next;
    if (offset) *offset = var->offset;
    if (ptypename) *ptypename = var->typename;
    else free(var->typename); 
    if (pvarname) *pvarname = var->varname; 
    else free(var->varname);
    free(var);
    return next;
}

stack_t* tracerSpawnStack(stack_t* parent) {
    stack_t* stack = calloc(1, sizeof(struct tracer_stack));
    stack->varcount = 0;
    stack->offsets = NULL;
    return stack;
}
size_t tracerStackGetVarCount(stack_t* stack) {
    if (!stack) return 0;
    return stack->varcount;
}
size_t tracerStackGetOffset(stack_t* stack, const char* const varname) {
    for (struct tracer_offset* var = stack->offsets; var; var = var->next) {
        if (!strcmp(var->varname, varname)) return var->offset;
    }
    return 0;
}
// Returns copy. Take care of it
char* tracerStackGetTypename(stack_t* stack, const char* const varname) {
    for (struct tracer_offset* var = stack->offsets; var; var = var->next) {
        if (!strcmp(var->varname, varname)) return strdup(var->typename);
    }
    return NULL;
}
stack_t* tracerPopStack(stack_t* stack) {
    while (stack->offsets) {
        stack->offsets = tracerStackPopOffset(stack->offsets, NULL, NULL, NULL);
    }
    stack_t* parent = stack->parent;
    free(stack);
    return parent;
}
// varname dups here
int tracerStackPushVar(stack_t* stack, char* typename, char* varname, int offset) {
    if (!stack) return false;
    struct tracer_offset* nextvar = malloc(sizeof(struct tracer_offset));
    if (!nextvar) return false;
    stack->varcount++;
    nextvar->offset = offset;
    nextvar->varname = strdup(varname);
    nextvar->typename = strdup(typename);
    nextvar->next = stack->offsets;
    stack->offsets = nextvar;
    return true;
}
int tracerStackPopVar(stack_t* stack, char** typename, char** varname, int* offset) {
    if (!stack) return 1;
    stack->offsets = tracerStackPopOffset(stack->offsets, typename, varname, offset);
    return 0;
}
stack_t* tracerPushStack() {
    return root_stack = tracerSpawnStack(root_stack);
}