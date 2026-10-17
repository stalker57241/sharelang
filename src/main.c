#include "lexer.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

static struct {
    bool needhelp:1;
    const char* filename;
} options = {0};

void printhelp(const char* progname) {
    printf("Usage:\n");
    // printf("%s -h", progname);
    printf("\t%s <options> filename\n", progname);
    printf("\t%s [-h|--help]\n", progname);
}

int main(const int argc, const char* const argv[]) {
    enum {
        STATE_FILENAME = 0,
    } state = STATE_FILENAME;
    for (int i = 1; i < argc; i++) {
        switch (state) {
        case STATE_FILENAME:
            if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
                printf("Needhelp\n");
                options.needhelp = true;
            }
            else {
                printf("argv[%d] = %s\n", i, argv[i]);
                options.filename = argv[i];
            }
            break;
        default:
            printf("State: %d\n", state);
        }
    }
    if (options.needhelp || options.filename == NULL) {
        printf("options.filename: %s\n", options.filename == NULL ? "(null)" : options.filename);
        printhelp(argv[0]);
        return options.filename == NULL ? 1 : 0;
    }
    if (!lexerInit(options.filename)) {
        return 1;
    }
    while (lexerHasTokens()) {
        Token token = lexerNext();
        printf("token: '%s'\n", token.text);
        free(token.text);
    }
    lexerTerminate();
    return 0;
}