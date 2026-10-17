#include "lexer.h"
#include "compiler.h"
#include "emit.h"
#include "assembler.h"

#include <linux/limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static char* OUTNAME = NULL;
static char* ASMNAME = NULL;

static bool error = false;

static Token (*peekn)(int n) = NULL;
static Token (*next)() = NULL;

static FILE* assemblycode = NULL;

static bool hasErrors() {
    return error;
}

static int generateAsmFile() {
    if (ASMNAME == NULL) {
        assemblycode = tmpfile();
    } else {
        assemblycode = fopen(ASMNAME, "w");
    }
    if (assemblycode == NULL) {
        fprintf(stderr, "Failed to create assembly file\n");
        return false;
    }
    return true;
}
static char* getAsmName() {
    char buffer[PATH_MAX] = {0};
    sprintf(buffer, "/dev/fd/%d", fileno(assemblycode));

    return strdup(buffer);
}
int compilerInit(const char* outname) {
    if (outname == NULL) return false;
    OUTNAME = strdup(outname);
    return generateAsmFile();
    // return true;
}
void compilerSetAsemblerPath(const char* asmname) {
    ASMNAME = strdup(asmname);
}

void compilerTerminate() {
    if (OUTNAME != NULL) {
        free(OUTNAME);
        OUTNAME = NULL;
    }
    if (ASMNAME != NULL) {
        free(ASMNAME);
        ASMNAME = NULL;
    }
    peekn = NULL;
    next = NULL;
    fclose(assemblycode);
}
static bool compileStatements();
static bool compileStatement();
static bool compileReturnStatement();

static bool compileStatements() {
    emit_global(assemblycode, "_start");
    emit_label(assemblycode, "_start");
    Token nextn = peekn(0);
    while (nextn.type != TOKEN_TYPE_EOF) {
        compileStatement();
        nextn = next();
    }
    return true;
}
static bool compileStatement() {
    bool state = false;
    #define TRY_COMPILE(what) if (what && !hasErrors()) return true
    
    TRY_COMPILE(compileReturnStatement());
    
    #undef TRY_COMPILE

    return false;
}
static bool compileReturnStatement() {
    Token current = peekn(0);
    if (current.text == NULL) {
        return false;
    }
    if (strcmp(current.text, "return") != 0) {
        return false;
    }
    next();
    Token result = peekn(1);
    if (result.type != TOKEN_TYPE_DIGIT) return false;
    emit_mov(assemblycode, "rax", "60");
    emit_mov(assemblycode, "rdi", result.text);
    emit_syscall(assemblycode);
    return true;
}
// static bool compileDigit() {
//     return true;
// }
bool compile() {
    if (peekn == NULL || next == NULL) {
        fprintf(stderr, "No peekn and/or next\n");
        return false;
    }
    if (OUTNAME == NULL) {
        fprintf(stderr, "No outname specified\n");
        return false;
    }
    compileStatements();
    return true;
}
bool assemble() {
    size_t length = snprintf(NULL, 0, "nasm -f elf64 %s -o %s", getAsmName(), OUTNAME);
    printf("assemble: length = %zu\n", length);
    char* command = calloc(length + 1, sizeof(char));
    size_t nextlength = snprintf(command, length + 1, "nasm -f elf64 %s -o %s", getAsmName(), OUTNAME);
    printf("assemble: nextlength = %zu\n", nextlength);
    system(command);
    printf("OUTNAME: %s\n", OUTNAME);
    printf("Executed: %s\n", command);
    free(command);
    return true;
}
void compilerSetPeeknFn(Token (*fn)(int)){
    peekn = fn;
}
void compilerSetNextFn(Token (*fn)()){
    next = fn;
}
