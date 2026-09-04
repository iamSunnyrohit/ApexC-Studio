#ifndef APEXC_CODEGEN_H
#define APEXC_CODEGEN_H

#include "parser.h"
#include <stdio.h>

void codegen_generate(const ASTNode *program, FILE *out);

#endif // APEXC_CODEGEN_H