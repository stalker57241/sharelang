#include "emit.h"
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

void emit_section(FILE* file, enum SectionType type) {
    switch (type) {
        case SECTION_TYPE_TEXT:
            fprintf(file, "section .text\n");
            break;
        case SECTION_TYPE_RODATA:
            fprintf(file, "section .rodata\n");
            break;
        case SECTION_TYPE_DATA:
            fprintf(file, "section .data\n");
            break;
        case SECTION_TYPE_BSS:
            fprintf(file, "section .bss\n");
            break;
        }
}

void emit_syscall(FILE* file) {
    fprintf(file, "\tsyscall");
    printf("emit:\tsyscall");
}

void emit_mov(FILE* file, char* left, char* right) {
    fprintf(file, "\tmov %s, %s\n", left, right);
    printf("emit:\tmov %s, %s\n", left, right);
}

void emit_label(FILE* file, char* label) {
    fprintf(file, "%s:\n", label);
    printf("emit(label): %s:\n", label);
}

void emit_global(FILE* file, char* label) {
    fprintf(file, "global %s\n", label);
    printf("emit: global %s\n", label);
}

void emit_push(FILE* file, char* what) {
    fprintf(file, "\tpush %s\n", what);
    printf("emit:\tpush %s\n", what);
}
void emit_pop(FILE* file, char* where) {
    fprintf(file, "\tpop %s\n", where);
    printf("emit:\tpop %s\n", where);
}
void emit_add(FILE* file, char* who, char* what) {
    fprintf(file, "\tadd %s, %s\n", who, what);
    printf("emit:\tadd %s, %s\n", who, what);
}
void emit_sub(FILE* file, char* who, char* what) {
    fprintf(file, "\tsub %s, %s\n", who, what);
    printf("emit:\tsub %s, %s\n", who, what);
}
void emit_empty(FILE* file) {
    fprintf(file, "\n");
}
void emit_comment(FILE* file, const char* const fmt, ...) {
    if (debug) {

        va_list args;
        fprintf(file, "; ");
        va_start(args, fmt);
        vfprintf(file, fmt, args);
        va_end(args);
        fprintf(file, "\n");
    }
}
void emit_mul(FILE* file, char* who, char* what) {
    fprintf(file, "\timul %s, %s\n", who, what);
    printf("emit:\timul %s, %s\n", who, what);
}
 
void emit_div(FILE* file, char* who, char* what) {
    // fprintf(file, "\tadd %s, %s\n", who, what);
	emit_mov(file, "rax", who);
	fprintf(file, "\tcqo\n");
	printf("\tcqo\n");
	fprintf(file, "\tidiv %s\n", what);
    printf("emit:\tidiv rcx\n");
	emit_mov(file, who, "rax");
}
 
void emit_mod(FILE* file, char* who, char* what) {
    // fprintf(file, "\tadd %s, %s\n", who, what);
	emit_mov(file, "rax", who);
	fprintf(file, "\tcqo\n");
	printf("\tcqo\n");
	fprintf(file, "\tidiv %s\n", what);
    printf("emit:\tidiv rcx\n");
	emit_mov(file, who, "rdx");
}

void emit_neg(FILE* file, char* who) {
    fprintf(file, "\tneg %s\n", who);
}

void emit_exit(FILE* file) {
    emit_mov(file, "rax", "60");
    emit_pop(file, "rdi");
    emit_syscall(file);
}
void emit_res(FILE* file, char* name, int size, unsigned int count) {
    char sz_char = 'b';
    // b - 1
    // w - 2
    // d - 4
    // q - 8
    switch (size) {
    case 1: sz_char = 'b'; break;
    case 2: sz_char = 'w'; break;
    case 4: sz_char = 'd'; break;
    case 8: sz_char = 'q'; break;
    }
    fprintf(file, "%s res%c %u\n", name, sz_char, count);
}
char* wrap_addr(char* text, int size) {
    size_t requiredsize = 0;
    switch (size){
    case 1: requiredsize = snprintf(NULL, 0, "byte [%s]", text); break;
    case 2: requiredsize = snprintf(NULL, 0, "word [%s]", text); break;
    case 4: requiredsize = snprintf(NULL, 0, "dword [%s]", text); break;
    case 8: requiredsize = snprintf(NULL, 0, "qword [%s]", text); break;
    default: requiredsize = snprintf(NULL, 0, "[%s]", text); break;
    }
    char* buffer = calloc(requiredsize + 1, sizeof(char));
    snprintf(buffer, requiredsize + 1, "[%s]", text);
    switch (size){
    case 1: snprintf(buffer, requiredsize + 1, "byte [%s]", text); break;
    case 2: snprintf(buffer, requiredsize + 1, "word [%s]", text); break;
    case 4: snprintf(buffer, requiredsize + 1, "dword [%s]", text); break;
    case 8: snprintf(buffer, requiredsize + 1, "qword [%s]", text); break;
    default: snprintf(buffer, requiredsize + 1, "[%s]", text); break;
    }
    buffer[requiredsize] = '\0';
    return buffer;
}