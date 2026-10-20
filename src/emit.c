#include "emit.h"


void emit_syscall(FILE* file) {
    fprintf(file, "\tsyscall");
    printf("emit:\tsyscall");
}

void emit_mov(FILE* file, char* const left, char* const right) {
    fprintf(file, "\tmov %s, %s\n", left, right);
    printf("emit:\tmov %s, %s\n", left, right);
}

void emit_label(FILE* file, char* const label) {
    fprintf(file, "%s:\n", label);
    printf("emit(label): %s:\n", label);
}

void emit_global(FILE* file, char* const label) {
    fprintf(file, "global %s\n", label);
    printf("emit: global %s\n", label);
}

void emit_push(FILE* file, char* const what) {
    fprintf(file, "\tpush %s\n", what);
    printf("emit:\tpush %s\n", what);
}
void emit_pop(FILE* file, char* const where) {
    fprintf(file, "\tpop %s\n", where);
    printf("emit:\tpop %s\n", where);
}
void emit_add(FILE* file, const char* const who, const char* const what) {
    fprintf(file, "\tadd %s, %s\n", who, what);
    printf("emit:\tadd %s, %s\n", who, what);
}
void emit_sub(FILE* file, const char* const who, const char* const what) {
    fprintf(file, "\tsub %s, %s\n", who, what);
    printf("emit:\tsub %s, %s\n", who, what);
}
void emit_empty(FILE* file) {
    fprintf(file, "\n");
}
 
void emit_mul(FILE* file, const char* const who, const char* const what) {
    fprintf(file, "\timul %s, %s\n", who, what);
    printf("emit:\timul %s, %s\n", who, what);
}
 
void emit_div(FILE* file, const char* const who, const char* const what) {
    // fprintf(file, "\tadd %s, %s\n", who, what);
    printf("emit:\tidiv %s, %s\n", who, what);
}
 
void emit_mod(FILE* file, const char* const who, const char* const what) {
    // fprintf(file, "\tadd %s, %s\n", who, what);
    printf("emit:\tadd %s, %s\n", who, what);
}
void emit_exit(FILE* file) {
    emit_mov(file, "rax", "60");
    emit_pop(file, "rdi");
    emit_syscall(file);
}