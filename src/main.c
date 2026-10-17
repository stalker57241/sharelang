#include "lexer.h"
#include "compiler.h"
#include "assembler.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

static struct {
    bool needhelp:1;
    const char* filename;
    const char* asmname;
    const char* outname;
} options = {0};

void printhelp(const char* progname) {
    printf("Usage:\n");
    // printf("%s -h", progname);
    printf("\t%s <options> filename\n", progname);
    printf("\t%s [-h|--help]\n", progname);
}

static Token* tokens = NULL;
static size_t size = 0, capacity = 1, cursor = 0;
static Token next() {
    if (cursor + 1 < size) return tokens[cursor++];
    return (Token) {NULL, TOKEN_TYPE_EOF, -1, -1};
}
static void prev() {
    if (cursor > 0) cursor--;
}
static Token peekn(int offset) {
    if (cursor + offset < size) return tokens[cursor + offset];
    return (Token) {NULL, TOKEN_TYPE_EOF, -1, -1};
}

int main(const int argc, const char* const argv[]) {
    enum {
        STATE_FILENAME = 0,
        STATE_OUTNAME,
        STATE_ASMNAME,
    } state = STATE_FILENAME;
    for (int i = 1; i < argc; i++) {
        switch (state) {
        case STATE_FILENAME:
            if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
                printf("Needhelp\n");
                options.needhelp = true;
            }
            else if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--out") == 0) state = STATE_OUTNAME;
            else if (strcmp(argv[i], "-a") == 0 || strcmp(argv[i], "--asm") == 0) state = STATE_ASMNAME;
            else {
                printf("argv[%d] = %s\n", i, argv[i]);
                options.filename = argv[i];
            }
            break;
        case STATE_OUTNAME:
            options.outname = argv[i];
            state = STATE_FILENAME;
            break;
        case STATE_ASMNAME:
            options.asmname = argv[i];
            state = STATE_FILENAME;
            break;
        default:
            printf("State: %d\n", state);
        }
    }
    printf("filename: %s\n", options.filename == NULL ? "(null)" : options.filename);
    printf("outname: %s\n", options.outname == NULL ? "(null)" : options.outname);
    printf("asmname: %s\n", options.asmname == NULL ? "(null)" : options.asmname);
    if (options.needhelp || options.filename == NULL) {
        printf("options.filename: %s\n", options.filename == NULL ? "(null)" : options.filename);
        printhelp(argv[0]);
        return options.filename == NULL ? 1 : 0;
    }
    if (!lexerInit(options.filename)) {
        return 1;
    }
    tokens = realloc(tokens, capacity * sizeof(Token));
    while (lexerHasTokens()) {
        Token token = lexerNext();
        tokens[size++] = token;
        if (size == capacity) {
            capacity <<= 1;
            tokens = realloc(tokens, capacity * sizeof(token));
        }
        // free(token.text);
    }
    lexerTerminate();
    if (options.outname == NULL) {
        char* filename = strdup(options.filename);
        size_t lastpoint = 0;
        for (size_t i = 0; i < strlen(filename); i++) {
            if (filename[i] == '.') lastpoint = i;
        }
        char* filenamenoext = calloc(lastpoint + 1, sizeof(char));
        strncpy(filenamenoext, filename, lastpoint);
        options.outname = strcat(filenamenoext, ".o");
        printf("-o: %s\n", options.outname);
        free(filenamenoext);
        free(filename);
    }
    if (options.asmname != NULL)
        compilerSetAsemblerPath(options.asmname);
    if (!compilerInit(options.outname)) {
        return 1;
    }
    compilerSetNextFn(next);
    compilerSetPrevFn(prev);
    compilerSetPeeknFn(peekn);
    
    if (!compile()) {
        compilerTerminate();
        return 1;
    }
    assemble();
    compilerTerminate();
    for (size_t idx = 0; idx < size; idx++) {
        printf("token: '%s'\n", tokens[idx].text);
        free(tokens[idx].text);
    }
    free(tokens);
    return 0;
}