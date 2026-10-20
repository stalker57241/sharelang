#include "lexer.h"
#include "compiler.h"
#include "emit.h"
#include "assembler.h"

#include <linux/limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char* STACK_REGISTER = "rbx";
static const char* MATH_REGISTER = "rcx";

static char* OUTNAME = NULL;
static char* ASMNAME = NULL;

static bool error = false;

static Token (*peekn)(int n) = NULL;
static Token (*next)() = NULL;
static void (*prev)() = NULL;

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
void compilerSetPeeknFn(Token (*fn)(int)){
    peekn = fn;
}
void compilerSetNextFn(Token (*fn)()){
    next = fn;
}
void compilerSetPrevFn(void (*fn)()){
    prev = fn;
}

// ------------------
// COMPILING
// ------------------
static bool compileStatements();
static bool compileStatement();
static bool compileReturnStatement();
static bool compileExpression();
static bool compileAddition();
static bool compileMultiplication();
static bool compileValue();
// + | -
// * | / | %
// value | varname | (expr)

// SUM = MUL | SUM + MUL | SUM - MUL
// MUL = VALUE | MUL * VALUE | MUL / VALUE | MUL % VALUE
// VALUE  = digit | '(' expr ')'

static bool compileStatements() {
    emit_global(assemblycode, "_start");
    emit_label(assemblycode, "_start");
    Token nextn = peekn(0);
    while (nextn.type != TOKEN_TYPE_EOF) {
        compileStatement();
        nextn = next();
        if (nextn.type == TOKEN_TYPE_SEPARATOR && strcmp(nextn.text, ";") == 0) {
            continue;
        }
        else {
            return false;
        }
    }
    return true;
}
static bool compileStatement() {
    
    if(compileReturnStatement() && !hasErrors()) return true;

    return false;
}
static bool compileReturnStatement() {
    Token current = next();
    if (current.text == NULL) {
        error = true;
        return false;
    }
    if (strcmp(current.text, "return") != 0) {
        prev();
        return false;
    }
    if (!compileExpression()) {
        error = true;
        return false;
    }
    // printf("compileReturnStatement: result.text = %s\n", result.text);
    emit_exit(assemblycode);
    return true;
}
static bool compileExpression() {
    return compileAddition();
}
static bool compileAddition() {
    printf("%u\tcompileAddition\n", __LINE__);
    if (!compileMultiplication()) return false;
    while (true) {

        Token nxt = peekn(0);
        printf("nxt(peekn): %s\n", nxt.text);
        struct {
            bool add:1;
            bool sub:1;
        } state = {0};
        if (!(
            nxt.type == TOKEN_TYPE_OPERATOR && (
                (state.add = (strcmp(nxt.text, "+") == 0)) ||
                (state.sub = (strcmp(nxt.text, "-") == 0))
            )
        )) {
            printf("compileAddition: no sign +/-\n");
            break;
        }
        if (state.add == state.sub) break;
        next();
        if (!compileAddition()) return false;
        emit_pop(assemblycode, MATH_REGISTER);
        emit_pop(assemblycode, STACK_REGISTER);
        if (state.add) emit_add(assemblycode, STACK_REGISTER, MATH_REGISTER);
        else if (state.sub) emit_sub(assemblycode, STACK_REGISTER, MATH_REGISTER);
        else return false;
        emit_push(assemblycode, STACK_REGISTER);
        emit_empty(assemblycode);
    }
    return true;
}
static bool compileMultiplication() {
    printf("%u\tcompileMultiplication\n", __LINE__);
    if (!compileValue()) return false;
    while (true) {

        Token nxt = peekn(0);
        struct {
            bool mul:1;
            bool div:1;
            bool mod:1;
        } state = {0};
        if (!(
            nxt.type == TOKEN_TYPE_OPERATOR && (
            (state.mul = (strcmp(nxt.text, "*") == 0)) ||
            (state.div = (strcmp(nxt.text, "/") == 0)) ||
            (state.mod = (strcmp(nxt.text, "%") == 0))
        ))) {
            printf("compileMultiplication: no sign *|/|%%\n");
            break;
        }
        next();
        if (!compileMultiplication()) return false;
        emit_pop(assemblycode, MATH_REGISTER);
        emit_pop(assemblycode, STACK_REGISTER);
        if(state.mul == true) emit_mul(assemblycode, STACK_REGISTER, MATH_REGISTER);
        else if (state.div == true) emit_div(assemblycode, STACK_REGISTER, MATH_REGISTER);
        else if (state.mod == true) emit_mod(assemblycode, STACK_REGISTER, MATH_REGISTER);
        else return false;
        emit_push(assemblycode, STACK_REGISTER);
        emit_empty(assemblycode);
    }
    return true;
}
static bool compileValue() {
    printf("%u\tcompileValue\n", __LINE__);
    Token token = peekn(0);
    if ((token.type == TOKEN_TYPE_OPEN_BRACKET && strcmp(token.text, "(") == 0)) {
        printf("compileValue:bracket\n");
        next();
        if (!compileExpression()) {
            return false;
        }
        printf("compileValue expr exit\n");
        token = next();
        if (!(token.type == TOKEN_TYPE_CLOSE_BRACKET && strcmp(token.text, ")") == 0)) {
            return false;
        }
    }
    else {
        Token current = next();
        char* value = NULL;
        switch (current.type) {
        case TOKEN_TYPE_LITERAL: {error = true; return false;}
        case TOKEN_TYPE_DIGIT: value = current.text; break;
        default: {error = true; return false;}
        }
        emit_mov(assemblycode, (char*) STACK_REGISTER, value);
        emit_push(assemblycode, (char*) STACK_REGISTER);
        emit_empty(assemblycode);
        return true;        
    }
    return true;
}
// static bool compileDigit() {
//     return true;
// }
bool compile() {
    if (peekn == NULL || next == NULL || prev == NULL) {
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
    fflush(assemblycode);
    system(command);
    printf("OUTNAME: %s\n", OUTNAME);
    printf("Executed: %s\n", command);
    free(command);
    return true;
}

size_t compilerGetLastRow() {
    return peekn(-1).row;
}
size_t compilerGetLastColumn() {
    return peekn(-1).column;
}