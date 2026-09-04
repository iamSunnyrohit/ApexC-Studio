#include "apexc_api.h"
#include <stdio.h>
#include <string.h>

extern void init_lexer(const char *src);
extern void *parse_program(void);
extern void generate_assembly_to_buffer(void *ast, char *buf, int max_len);

int apexc_compile_to_string(const char *source, char *out_buf, int max_len) {
    if (!source || !out_buf || max_len <= 0) return -1;

    // TODO: Route your lexer/parser/codegen to write into out_buf
    FILE *memstream = fmemopen(out_buf, max_len, "w");
    if (!memstream) return -1;

    fclose(memstream);
    return 0;
}