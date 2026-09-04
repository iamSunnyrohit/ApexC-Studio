#include "apexc_api.h"
#include "lexer.h"
#include "parser.h"
#include "eval.h"
#include "codegen.h"
#include "builtins.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *apexc_eval(const char *source) {
    if (!source) return strdup("Error: Empty input source");

    builtins_reset_stdout();

    Parser parser = parser_init(source);
    ASTNode *program = parse_program(&parser);

    if (!program) {
        return strdup("Syntax Error: Failed to parse C program");
    }

    int ret_code = eval_program(program);
    ast_free(program);

    char buf[MAX_TERM_OUTPUT + 256];
    if (strlen(g_runtime_stdout) > 0) {
        snprintf(buf, sizeof(buf), "%s\n[Program exited with code: %d]", g_runtime_stdout, ret_code);
    } else {
        snprintf(buf, sizeof(buf), "[Program exited with code: %d]", ret_code);
    }

    return strdup(buf);
}

char *apexc_compile_asm(const char *source) {
    if (!source) return strdup("// Error: Empty input source");

    char *asm_buffer = NULL;
    size_t asm_size = 0;
    FILE *mem_out = open_memstream(&asm_buffer, &asm_size);

    if (!mem_out) {
        return strdup("// Internal Error: open_memstream failed");
    }

    Parser parser = parser_init(source);
    ASTNode *program = parse_program(&parser);

    if (!program) {
        fprintf(mem_out, "// ApexC Error: Syntax parsing failed\n");
    } else {
        codegen_generate(program, mem_out);
        ast_free(program);
    }

    fflush(mem_out);
    fclose(mem_out);

    char *result = asm_buffer ? strdup(asm_buffer) : strdup("// No assembly generated");
    if (asm_buffer) free(asm_buffer);
    return result;
}

void apexc_free_string(char *str) {
    if (str) free(str);
}