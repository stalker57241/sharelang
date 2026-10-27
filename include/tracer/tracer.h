#pragma once

#include <stddef.h>

typedef struct tracer_stack stack_t;
typedef struct tracer_arg arg_t;

typedef struct tracer_arg_ctl {
    size_t argcount;
    arg_t* args;
} arg_ctl;

extern stack_t* root_stack;
// LIFECYCLE

int tracerInit();
void tracerTerminate();

// STACK
stack_t* tracerSpawnStack(stack_t* parent);
size_t tracerStackGetVarCount(stack_t* stack);
size_t tracerStackGetOffset(stack_t* stack, const char* const varname);
char* tracerStackGetTypename(stack_t* stack, const char* const varname);
stack_t* tracerPopStack(stack_t* stack);
int tracerStackPushVar(stack_t* stack, char* typename, char* varname, int offset);
int tracerStackPopVar(stack_t* stack, char** typename, char** pvarname, int* poffset);

stack_t* tracerPushStack();

// ARG
void tracerPushArg(arg_t* ctl, char* typename, char* varname, int offset);
void tracerPopArg(arg_t* ctl, char** typename, char** varname, int* offset);

size_t tracerVarGetOffset(arg_t* ctl, const char* const varname);
char* tracerVarGetTypename(arg_t* ctl, const char* const varname);