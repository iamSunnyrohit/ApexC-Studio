#ifndef CODEGEN_H
#define CODEGEN_H

#include <stdio.h>
#include "parser.h"

void codegen_generate(ASTNode *program, FILE *out);

#endif // CODEGEN_H
