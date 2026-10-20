#pragma once

#define __need_size_t

#include <stdbool.h>
#include <stddef.h>

typedef enum token_type {
    TOKEN_TYPE_EOF = 0,
    TOKEN_TYPE_LITERAL,
    TOKEN_TYPE_DIGIT,
    TOKEN_TYPE_STRING,
    TOKEN_TYPE_OPERATOR,
    TOKEN_TYPE_SEPARATOR,
    TOKEN_TYPE_OPEN_BRACKET,
    TOKEN_TYPE_CLOSE_BRACKET,
    TOKEN_TYPE_MAX,
} TokenType;

typedef struct token {
    char* text;
    TokenType type;
    size_t row, column;
} Token;
int lexerInit(const char* filename);
void lexerTerminate();

bool lexerHasTokens();
Token lexerNext();