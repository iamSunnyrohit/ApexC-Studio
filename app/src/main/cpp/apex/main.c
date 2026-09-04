#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "parser.h"
#include "codegen.h"

static void print_usage(const char *prog_name) {
    fprintf(stderr, "ApexC C Compiler (ARM64 Apple Silicon Target)\n");
    fprintf(stderr, "Usage: %s [options] <input.c>\n", prog_name);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -S              Compile only; emit ARM64 assembly file\n");
    fprintf(stderr, "  -o <file>       Place output into <file>\n");
    fprintf(stderr, "  --dump-ast      Pretty-print the parsed AST hierarchy to terminal\n");
}

static char *read_file(const char *filename) {
    FILE *f = fopen(filename, "rb");
    if (!f) {
        perror("Error opening input file");
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *buf = malloc(sz + 1);
    if (!buf) {
        fclose(f);
        fprintf(stderr, "Error: Memory allocation failed\n");
        return NULL;
    }
    size_t read_sz = fread(buf, 1, sz, f);
    buf[read_sz] = '\0';
    fclose(f);
    return buf;
}

static bool ends_with(const char *str, const char *suffix) {
    if (!str || !suffix) return false;
    size_t lenstr = strlen(str);
    size_t lensuffix = strlen(suffix);
    if (lensuffix > lenstr) return false;
    return strncmp(str + lenstr - lensuffix, suffix, lensuffix) == 0;
}

int main(int argc, char **argv) {
    const char *input_file = NULL;
    const char *output_file = NULL;
    bool emit_asm_only = false;
    bool dump_ast_flag = false;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-S") == 0) {
            emit_asm_only = true;
        } else if (strcmp(argv[i], "--dump-ast") == 0) {
            dump_ast_flag = true;
        } else if (strcmp(argv[i], "-o") == 0) {
            if (i + 1 < argc) {
                output_file = argv[++i];
            } else {
                fprintf(stderr, "Error: -o option requires an argument\n");
                print_usage(argv[0]);
                return 1;
            }
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "Error: Unknown option '%s'\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        } else {
            if (!input_file) {
                input_file = argv[i];
            } else {
                fprintf(stderr, "Error: Multiple input files provided\n");
                print_usage(argv[0]);
                return 1;
            }
        }
    }

    if (!input_file) {
        fprintf(stderr, "Error: No input file specified\n");
        print_usage(argv[0]);
        return 1;
    }

    char *source = read_file(input_file);
    if (!source) {
        return 1;
    }

    Parser parser = parser_init(source);
    ASTNode *ast = parse_program(&parser);

    if (parser.has_error || !ast) {
        fprintf(stderr, "Compilation failed due to syntax errors.\n");
        free(source);
        if (ast) ast_free(ast);
        return 1;
    }

    if (dump_ast_flag) {
        ast_dump(ast, 0);
        ast_free(ast);
        free(source);
        return 0;
    }

    // Determine output assembly destination
    bool is_asm_output = emit_asm_only || (output_file && ends_with(output_file, ".s"));
    const char *asm_file_path = is_asm_output ? (output_file ? output_file : "output.s") : "/tmp/apexc_tmp.s";

    FILE *out = stdout;
    if (output_file || !is_asm_output) {
        out = fopen(asm_file_path, "w");
        if (!out) {
            perror("Error opening output file");
            free(source);
            ast_free(ast);
            return 1;
        }
    }

    codegen_generate(ast, out);

    if (out != stdout) {
        fclose(out);
    }

    // If compiling directly to executable binary
    if (!is_asm_output && output_file) {
        char cmd[1024];
        snprintf(cmd, sizeof(cmd), "clang -arch arm64 %s -o %s", asm_file_path, output_file);
        int res = system(cmd);
        remove(asm_file_path);
        if (res != 0) {
            fprintf(stderr, "Error: Linking failed\n");
            ast_free(ast);
            free(source);
            return 1;
        }
    }

    ast_free(ast);
    free(source);
    return 0;
}
