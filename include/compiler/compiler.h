#pragma once

#include "lexer.h"

int compilerInit(const char* filename);
void compilerTerminate();
void compilerSetAssemblyFile(const char* filename);
void compilerSetDebug(bool value);

void compilerSetPeeknFn(Token (*fn)(int offset));
void compilerSetNextFn(Token (*fn)());
void compilerSetPrevFn(void (*fn)());

bool compile();
size_t compilerGetLastRow();
size_t compilerGetLastColumn();