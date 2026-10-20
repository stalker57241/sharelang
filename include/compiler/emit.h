#pragma once


#include <stdio.h>

void emit_syscall(FILE* file);
void emit_mov(FILE* file, char* const left, char* const right);
void emit_label(FILE* file, char* const label);
void emit_global(FILE* file, char* const label);

// platform-specific
// Stack: exit code on top
void emit_exit(FILE* file);

// stack
void emit_push(FILE* file, char* const what);
void emit_pop(FILE* file, char* const where);

void emit_add(FILE* file, const char* const who, const char* const what); 
void emit_sub(FILE* file, const char* const who, const char* const what);

void emit_empty(FILE* file);
 
void emit_mul(FILE* file, const char* const who, const char* const what);
void emit_div(FILE* file, const char* const who, const char* const what);
void emit_mod(FILE* file, const char* const who, const char* const what);