#include "emit.h"


void emit_syscall(FILE* file) {
    fprintf(file, "\tsyscall");
}

void emit_mov(FILE* file, char* const left, char* const right) {
    fprintf(file, "\tmov %s, %s", left, right);
}

void emit_label(FILE* file, char* const label) {
    fprintf(file, "%s:\n", label);
}

void emit_global(FILE* file, char* const label) {
    fprintf(file, "global %s\n", label);
}
