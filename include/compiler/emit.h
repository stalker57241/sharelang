#pragma once


#include <stdio.h>
enum SectionType {
    SECTION_TYPE_TEXT,
    SECTION_TYPE_DATA,
    SECTION_TYPE_RODATA,
    SECTION_TYPE_BSS
};

enum Size {
    BYTE = 'b',
    WORD = 'w',
    DWORD = 'd',
    QWORD = 'q'
};

void emit_section(FILE* file, const char* name, enum SectionType type);

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

void emit_exit(FILE* file);

void emit_res(FILE* file, const char* varname, enum Size size, unsigned int bytecount);
