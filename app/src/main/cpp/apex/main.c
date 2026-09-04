#include <stdio.h>
#include <stdlib.h>
#include "lexer.h"
#include "parser.h"
#include "eval.h"
#include "codegen.h"
#include "builtins.h"

int main(int argc, char **argv) {
    const char *test_code =
            "#include <stdio.h>\n"
            "#include <math.h>\n\n"
            "int main() {\n"
            "    int val = 64;\n"
            "    int r = sqrt(val);\n"
            "    printf(\"sqrt(%d) = %d\\n\", val, r);\n"
            "    return r;\n"
            "}\n";

    printf("=== APEXC EVALUATION ===\n");
    builtins_reset_stdout();
    Parser p1 = parser_init(test_code);
    ASTNode *prog1 = parse_program(&p1);
    if (prog1) {
        int code = eval_program(prog1);
        if (strlen(g_runtime_stdout) > 0) {
            printf("%s", g_runtime_stdout);
        }
        printf("Return Code: %d\n\n", code);
        ast_free(prog1);
    }

    printf("=== APEXC ARM64 ASSEMBLY ===\n");
    Parser p2 = parser_init(test_code);
    ASTNode *prog2 = parse_program(&p2);
    if (prog2) {
        codegen_generate(prog2, stdout);
        ast_free(prog2);
    }

    return 0;
}