#pragma once


#include <stdio.h>

#ifndef ROOT
extern int debug;
#else
int debug;
#endif
enum SectionType {
    SECTION_TYPE_TEXT,
    SECTION_TYPE_RODATA,
    SECTION_TYPE_DATA,
    SECTION_TYPE_BSS,
};

char* wrap_addr(char* text, int size);

void emit_section(FILE* file, enum SectionType type);
void emit_syscall(FILE* file);
void emit_mov(FILE* file, char* left, char* right);
void emit_label(FILE* file, char* label);
void emit_global(FILE* file, char* label);

// platform-specific
// Stack: exit code on top
void emit_exit(FILE* file);

// HELPER
void emit_comment(FILE* file, const char* const fmt, ...);
void emit_empty(FILE* file);

// stack
void emit_push(FILE* file, char* what);
void emit_pop(FILE* file, char* where);

// ops
void emit_add(FILE* file, char* who, char* what); 
void emit_sub(FILE* file, char* who, char* what);

void emit_mul(FILE* file, char* who, char* what);
void emit_div(FILE* file, char* who, char* what);
void emit_mod(FILE* file, char* who, char* what);

void emit_neg(FILE* file, char* who);

// mem
void emit_res(FILE* file, char* name, int size, unsigned int count);
