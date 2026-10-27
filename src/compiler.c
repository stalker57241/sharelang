#define ROOT

#include "lexer.h"
#include "compiler.h"
#include "emit.h"
#include "assembler.h"
#include "tracer.h"

#include <linux/limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char* STACK_EREGISTER = "ebx";
static char* STACK_RREGISTER = "rbx";
static char* MATH_EREGISTER = "ecx";
static char* MATH_RREGISTER = "rcx";

static char* OUTNAME = NULL;
static char* ASMNAME = NULL;

static struct State {
    bool error:1;
    bool hasbss:1;
} state = {false, false};

static Token (*peekn)(int n) = NULL;
static Token (*next)() = NULL;
static void (*prev)() = NULL;

static char* lastvarname = NULL;
static int lasttypesize = 0;

static FILE* globaldatacode = NULL;

struct codeblock {
    FILE* prefix;
    FILE* suffix;
};
static struct function {
    struct codeblock* block;
    FILE* assemblycode;
}* function = NULL;

static FILE* resultcode = NULL;

static bool hasErrors() {
    return state.error;
}
static bool hasBSS() {
    return state.hasbss;
}
static int generateAsmFile() {
    if (ASMNAME == NULL) {
        resultcode = tmpfile();
    } else {
        resultcode = fopen(ASMNAME, "w");
    }
    if (resultcode == NULL) {
        fprintf(stderr, "Failed to create assembly file\n");
        return false;
    }
    return true;
}
void compilerSetDebug(bool value) {
    debug = value;
}
static char* getAsmName() {
    char buffer[PATH_MAX] = {0};
    if (ASMNAME == NULL)
        sprintf(buffer, "/dev/fd/%d", fileno(resultcode));
    else
        sprintf(buffer, "%s", ASMNAME);

    return strdup(buffer);
}
int compilerInit(const char* outname) {
    if (outname == NULL) return false;
    OUTNAME = strdup(outname);
    globaldatacode = tmpfile();
    function = malloc(sizeof(struct function));
    function->assemblycode = tmpfile();
    tracerInit();
    return generateAsmFile();
}
void compilerSetAssemblyFile(const char* asmname) {
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

    fclose(globaldatacode);
    fclose(function->assemblycode);

    fclose(resultcode);

    free(function);
    tracerTerminate();
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
// Stmt
static bool compileVarDeclaration();
static bool compileVarDefinition();
static bool compileReturnStatement();

// Expr
static bool compileExpression();
static bool compileAddition();
static bool compileMultiplication();
static bool compileNegate();
static bool compileValue();
// + | -
// * | / | %
// value | varname | (expr)

// SUM = MUL | SUM + MUL | SUM - MUL
// MUL = VALUE | MUL * VALUE | MUL / VALUE | MUL % VALUE
// VALUE  = digit | '(' expr ')'

static bool compileStatements() {
    emit_section(globaldatacode, SECTION_TYPE_BSS);

    emit_section(function->assemblycode, SECTION_TYPE_TEXT);
    emit_global(function->assemblycode, "_start");
    emit_label(function->assemblycode, "_start");
    
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
    if(compileVarDeclaration()  && !hasErrors()) return true;
    if(compileReturnStatement() && !hasErrors()) return true;

    return false;
}
static bool compileVarDeclaration() {
    if (!compileVarDefinition()) return false;

    Token operator = peekn(0);
    if (operator.type == TOKEN_TYPE_OPERATOR && strcmp(operator.text, "=") == 0) {
        next();
        compileExpression();
        char* buffer = wrap_addr(lastvarname, 0);
        emit_pop(function->assemblycode, STACK_RREGISTER);
        emit_mov(function->assemblycode, buffer, STACK_EREGISTER);
        emit_empty(function->assemblycode);
        free(buffer);
        return true;
    }
    else if (operator.type == TOKEN_TYPE_SEPARATOR && strcmp(operator.text, ";") == 0) return true;
    state.error = true;
    return false;
}
static bool compileVarDefinition() {
    Token type = peekn(0);
    if (type.type != TOKEN_TYPE_LITERAL || strcmp(type.text, "int") != 0) { 
        return false; // not int
    }
    type = next();
    Token name = next();
    const int size = sizeof(int);
    emit_res(globaldatacode, name.text, size, 1);
    state.hasbss = 1;
    lastvarname = name.text;
    lasttypesize = 4;
    printf("compileVarDefinition\n");
    return true;
}
static bool compileReturnStatement() {
    Token current = next();
    if (current.text == NULL) {
        state.error = true;
        return false;
    }
    if (strcmp(current.text, "return") != 0) {
        prev();
        return false;
    }
    if (
        peekn(0).type == TOKEN_TYPE_DIGIT &&
        peekn(1).type == TOKEN_TYPE_SEPARATOR && strcmp(peekn(1).text, ";") == 0
    ) {
        emit_mov(function->assemblycode, "rax", "60");
        emit_mov(function->assemblycode, "rdi", peekn(0).text);
        emit_syscall(function->assemblycode);
        return true;
    }
    else if (!compileExpression()) {
        state.error = true;
        return false;
    }
    // printf("compileReturnStatement: result.text = %s\n", result.text);
    emit_exit(function->assemblycode);
    return true;
}
static bool evalExpression();
static bool evalExpressionResult(long long* result, int* level);
static bool evalAddition(long long* result, int* level);
static bool evalMultiplication(long long* result, int* level);
static bool evalNegate(long long* result, int* level);
static bool evalValue(long long* result, int* level);

static bool evalExpression() {
    // 1. Сохраняем позицию в лексере перед попыткой эвалюации
    // (Замените на вашу структуру/индекс, например: int save_pos = lexer->position;)
    int saved_token_index = 0; 
    emit_comment(function->assemblycode, __FUNCTION__);

    long long compile_time_value = 0;
    if (evalExpressionResult(&compile_time_value, &saved_token_index)) {
        // Успех! Выражение полностью посчитано. Генерируем всего 1 инструкцию вместо дерева.
        char buffer[32];
        sprintf(buffer, "%lld", compile_time_value);
        
        emit_comment(function->assemblycode, __FUNCTION__);
        emit_mov(function->assemblycode, STACK_RREGISTER, buffer);
        emit_push(function->assemblycode, STACK_RREGISTER);
        emit_empty(function->assemblycode);
        return true;
    }

    // 2. Если не посчиталось (вернуло false), ОТКАТЫВАЕМ позицию токенов назад
    // (Замените на вашу функцию отката, например: restore_lexer_position(save_pos);)
    while (saved_token_index > 0) {
        prev();
        saved_token_index--;
    }
    
    return false;
}
static Token evalNext(int* level) {
    (*level)++;
    // Token token = next();
    // emit_comment("%ld:%ld", token.row, token.column);
    return next();
}
static bool evalExpressionResult(long long *result, int* level) {
    emit_comment(function->assemblycode, __FUNCTION__);
    return evalAddition(result, level);
}
static bool evalAddition(long long* result, int* level) {
    long long left_val;
    emit_comment(function->assemblycode, __FUNCTION__);
    if (!evalMultiplication(&left_val, level)) return false;

    while (true) {
        Token nxt = peekn(0);
        bool is_add = (nxt.type == TOKEN_TYPE_OPERATOR && strcmp(nxt.text, "+") == 0);
        bool is_sub = (nxt.type == TOKEN_TYPE_OPERATOR && strcmp(nxt.text, "-") == 0);

        if (!is_add && !is_sub) break;

        evalNext(level);
        long long right_val;
        // Важно: для правильной ассоциативности вызываем evalMultiplication, а не evalAddition
        if (!evalMultiplication(&right_val, level)) return false; 

        if (is_add) left_val += right_val;
        else left_val -= right_val;
    }
    *result = left_val;
    emit_comment(function->assemblycode, "%s: left_val: %lld [%d]\n", __FUNCTION__, left_val, *level);
    return true;
}
static bool evalMultiplication(long long* result, int* level) {
    long long left_val;
    if (!evalNegate(&left_val, level)) return false;

    while (true) {
        Token nxt = peekn(0);
        bool is_mul = (nxt.type == TOKEN_TYPE_OPERATOR && strcmp(nxt.text, "*") == 0);
        bool is_div = (nxt.type == TOKEN_TYPE_OPERATOR && strcmp(nxt.text, "/") == 0);
        bool is_mod = (nxt.type == TOKEN_TYPE_OPERATOR && strcmp(nxt.text, "%") == 0);

        if (!is_mul && !is_div && !is_mod) break;

        evalNext(level);
        long long right_val;
        if (!evalNegate(&right_val, level)) return false;

        if (is_mul) {
            left_val *= right_val;
        } else {
            if (right_val == 0) {
                // Защита от деления на ноль при компиляции
                state.error = true; 
                return false; 
            }
            if (is_div) left_val /= right_val;
            else left_val %= right_val;
        }
    }
    *result = left_val;
    emit_comment(function->assemblycode, "%s: left_val: %lld [%d]\n", __FUNCTION__, left_val, *level);
    return true;
}
static bool evalNegate(long long* result, int* level) {
    Token token = peekn(0);
    bool isneg = false;
    if (
        token.type == TOKEN_TYPE_OPERATOR &&
        (strcmp(token.text, "+") == 0 ||
        (isneg = strcmp(token.text, "-") == 0)
        )
    ) {
        evalNext(level);
        long long value;
        emit_comment(function->assemblycode, __FUNCTION__);
        if (!evalValue(&value, level)) return false;
        if (isneg) *result = -value;
        else *result = value;
        return true;
    }
    return evalValue(result, level);
}
static bool evalValue(long long* result, int* level) {
    Token token = peekn(0);
    
    if (token.type == TOKEN_TYPE_OPEN_BRACKET && strcmp(token.text, "(") == 0) {
        evalNext(level);
        if (!evalExpressionResult(result, level)) return false;
        
        token = evalNext(level);
        if (!(token.type == TOKEN_TYPE_CLOSE_BRACKET && strcmp(token.text, ")") == 0)) {
            return false;
        }
        return true;
    }
    
    if (token.type == TOKEN_TYPE_DIGIT) {
        evalNext(level);
        *result = strtoll(token.text, NULL, 10);
        emit_comment(function->assemblycode, "%s: *result: %lld [%d]\n", __FUNCTION__, *result, *level);
        return true;
    }

    return false; 
}
static bool compileExpression() {
    if (evalExpression()) return true;
    return compileAddition();
}
static bool compileAddition() {
    emit_comment(function->assemblycode, __FUNCTION__);
    if (!compileMultiplication()) return false;
    while (true) {

        Token nxt = peekn(0);
        printf("nxt(peekn): %s\n", nxt.text);
        struct {
            bool add:1;
            bool sub:1;
        } opstate = {0};
        if (!(
            nxt.type == TOKEN_TYPE_OPERATOR && (
                (opstate.add = (strcmp(nxt.text, "+") == 0)) ||
                (opstate.sub = (strcmp(nxt.text, "-") == 0))
            )
        )) {
            printf("compileAddition: no sign +/-\n");
            break;
        }
        if (!opstate.add && !opstate.sub) break;
        next();
        if (!compileMultiplication()) return false;

        emit_comment(function->assemblycode, __FUNCTION__);
        emit_pop(function->assemblycode, MATH_RREGISTER);
        emit_pop(function->assemblycode, STACK_RREGISTER);
        if (opstate.add) emit_add(function->assemblycode, STACK_RREGISTER, MATH_RREGISTER);
        else if (opstate.sub) emit_sub(function->assemblycode, STACK_RREGISTER, MATH_RREGISTER);
        else {
            state.error = true;
            return false;
        }
        emit_push(function->assemblycode, STACK_RREGISTER);
        emit_empty(function->assemblycode);
    }
    return true;
}
static bool compileMultiplication() {
    emit_comment(function->assemblycode, __FUNCTION__);
    if (!compileNegate()) return false;
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
		printf("oper: %s\n", nxt.text);
        next();
        if (!compileNegate()) return false;
        emit_comment(function->assemblycode, __FUNCTION__);
        emit_pop(function->assemblycode, MATH_RREGISTER);
        emit_pop(function->assemblycode, STACK_RREGISTER);
        if(state.mul == true) emit_mul(function->assemblycode, STACK_RREGISTER, MATH_RREGISTER);
        else if (state.div == true) emit_div(function->assemblycode, STACK_RREGISTER, MATH_RREGISTER);
        else if (state.mod == true) emit_mod(function->assemblycode, STACK_RREGISTER, MATH_RREGISTER);
        else return false;
        emit_push(function->assemblycode, STACK_RREGISTER);
        emit_empty(function->assemblycode);
    }
    return true;
}

static bool compileNegate() {
    emit_comment(function->assemblycode, __FUNCTION__);
    Token token = peekn(0);
    bool isneg = false;
    if (
        token.type == TOKEN_TYPE_OPERATOR &&
        (strcmp(token.text, "+") == 0 ||
        (isneg = strcmp(token.text, "-") == 0)
        )
    ) {
        next();
        if (!compileValue()) return false;
        emit_pop(function->assemblycode, STACK_RREGISTER);
        emit_neg(function->assemblycode, STACK_RREGISTER);
        emit_push(function->assemblycode, STACK_RREGISTER);
        emit_empty(function->assemblycode);
        return true;
    }
    return compileValue();
}
static bool compileBracketedExpression() {
    Token token = peekn(0);
    if (!(token.type == TOKEN_TYPE_OPEN_BRACKET && strcmp(token.text, "(") == 0)) return false;
    
    emit_comment(function->assemblycode, __FUNCTION__);
    printf("compileValue:bracket\n");
    next();
    if (!compileExpression()) {
        return false;
    }
    printf("compileValue expr exit\n");
    token = next();
    if (!(token.type == TOKEN_TYPE_CLOSE_BRACKET && strcmp(token.text, ")") == 0)) {
        state.error = true;
        return false;
    }
    return true;
    
}
static bool compileValue() {
    Token token = peekn(0);
    if (compileBracketedExpression() && !hasErrors()) return true;
    else if (hasErrors()) return false;
    Token current = next();
    char* value = NULL;
    bool freevalue = false;
    switch (current.type) {
    case TOKEN_TYPE_LITERAL: {
        value = wrap_addr(current.text, 0);
        freevalue = true;
        break;
    }
    case TOKEN_TYPE_DIGIT:{
        value = current.text;
        break;
    } 
    default: {state.error = true; return false;}
    }
    emit_comment(function->assemblycode, __FUNCTION__);
    emit_mov(function->assemblycode, STACK_RREGISTER, value);
    emit_push(function->assemblycode, STACK_RREGISTER);
    emit_empty(function->assemblycode);
    if (freevalue) free(value);
    return true;        
}

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
    
    fflush(globaldatacode);
    fflush(function->assemblycode);
    // fprintf(resultcode, "; .intel_syntax noprefix\n");
    size_t datalen = ftell(globaldatacode);
    size_t assemblylen = ftell(function->assemblycode);
    
    if (state.hasbss) {

        char* databuffer = calloc(datalen + 1, sizeof(char));
        databuffer[datalen] = '\0';
        rewind(globaldatacode);
        fread(databuffer, datalen, sizeof(char), globaldatacode);
        fwrite(databuffer, datalen, sizeof(char), resultcode);
        free(databuffer);
        fprintf(resultcode, "\n");
    }
    
    char* assemblybuffer = calloc(assemblylen + 1, sizeof(char));
    assemblybuffer[assemblylen] = '\0';
    rewind(function->assemblycode);
    fread(assemblybuffer, assemblylen, sizeof(char), function->assemblycode);
    fwrite(assemblybuffer, assemblylen, sizeof(char), resultcode);
    free(assemblybuffer);

    return true;
}
bool assemble() {
    size_t length = snprintf(NULL, 0, "nasm -f elf64 %s -o %s", getAsmName(), OUTNAME);
    printf("assemble: length = %zu\n", length);
    char* command = calloc(length + 1, sizeof(char));
    size_t nextlength = snprintf(command, length + 1, "nasm -f elf64 %s -o %s", getAsmName(), OUTNAME);
    printf("assemble: nextlength = %zu\n", nextlength);
    fflush(resultcode);
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