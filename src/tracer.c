#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#include "tracer.h"

stack_t* root_stack = NULL;
arg_ctl* args;

int tracerInit() {
    root_stack = tracerPushStack();
    args = calloc(1, sizeof(struct tracer_arg_ctl));
    return true;
}
void tracerTerminate() {
    while ((root_stack = tracerPopStack(root_stack)));
    return;
}
