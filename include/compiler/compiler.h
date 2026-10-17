#pragma once

#include "lexer.h"

int compilerInit(const char* filename);
void compilerTerminate();
void compilerSetAsemblerPath(const char* filename);

void compilerSetPeeknFn(Token (*fn)(int offset));
void compilerSetNextFn(Token (*fn)());

bool compile();