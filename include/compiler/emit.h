#pragma once


#include <stdio.h>

void emit_syscall(FILE* file);
void emit_mov(FILE* file, char* const left, char* const right);
void emit_label(FILE* file, char* const label);
void emit_global(FILE* file, char* const label);
