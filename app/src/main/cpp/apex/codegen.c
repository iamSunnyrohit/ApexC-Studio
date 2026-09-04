#include "codegen.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct StringEntry {
    int id;
    char *text;
    struct StringEntry *next;
} StringEntry;

static StringEntry *g_strings = NULL;
static int g_label_counter = 0;

static void register_string(int id, const char *text) {
    StringEntry *entry = malloc(sizeof(StringEntry));
    entry->id = id;
    entry->text = strdup(text);
    entry->next = g_strings;
    g_strings = entry;
}

static void free_strings(void) {
    StringEntry *curr = g_strings;
    while (curr) {
        StringEntry *tmp = curr->next;
        free(curr->text);
        free(curr);
        curr = tmp;
    }
    g_strings = NULL;
}

static void codegen_expr(const ASTNode *node, FILE *out);
static void codegen_stmt(const ASTNode *node, FILE *out);

static void codegen_expr(const ASTNode *node, FILE *out) {
    if (!node) return;

    switch (node->type) {
        case AST_NUM:
            fprintf(out, "    mov  w0, #%d\n", node->num.value);
            break;

        case AST_STRING:
            register_string(node->str.label_id, node->str.value);
            fprintf(out, "    adrp x0, .LC%d\n", node->str.label_id);
            fprintf(out, "    add  x0, x0, :lo12:.LC%d\n", node->str.label_id);
            break;

        case AST_VAR_REF:
            fprintf(out, "    ldr  w0, [sp, #%d]\n", node->var_ref.offset);
            break;

        case AST_BINARY_OP:
            // Evaluate left operand -> w0, push to stack
            codegen_expr(node->binary.left, out);
            fprintf(out, "    str  w0, [sp, #-16]!\n");

            // Evaluate right operand -> w0
            codegen_expr(node->binary.right, out);
            fprintf(out, "    mov  w1, w0\n");

            // Pop left operand -> w0
            fprintf(out, "    ldr  w0, [sp], #16\n");

            switch (node->binary.op) {
                case TOKEN_PLUS:
                    fprintf(out, "    add  w0, w0, w1\n");
                    break;
                case TOKEN_MINUS:
                    fprintf(out, "    sub  w0, w0, w1\n");
                    break;
                case TOKEN_STAR:
                    fprintf(out, "    mul  w0, w0, w1\n");
                    break;
                case TOKEN_SLASH:
                    fprintf(out, "    sdiv w0, w0, w1\n");
                    break;
                case TOKEN_EQ:
                    fprintf(out, "    cmp  w0, w1\n");
                    fprintf(out, "    cset w0, eq\n");
                    break;
                case TOKEN_NE:
                    fprintf(out, "    cmp  w0, w1\n");
                    fprintf(out, "    cset w0, ne\n");
                    break;
                case TOKEN_LT:
                    fprintf(out, "    cmp  w0, w1\n");
                    fprintf(out, "    cset w0, lt\n");
                    break;
                case TOKEN_LE:
                    fprintf(out, "    cmp  w0, w1\n");
                    fprintf(out, "    cset w0, le\n");
                    break;
                case TOKEN_GT:
                    fprintf(out, "    cmp  w0, w1\n");
                    fprintf(out, "    cset w0, gt\n");
                    break;
                case TOKEN_GE:
                    fprintf(out, "    cmp  w0, w1\n");
                    fprintf(out, "    cset w0, ge\n");
                    break;
                default:
                    break;
            }
            break;

        case AST_FUNC_CALL: {
            int arg_count = node->func_call.arg_count;

            // Evaluate arguments and preserve them on the stack in reverse order
            for (int i = arg_count - 1; i >= 0; i--) {
                ASTNode *arg = node->func_call.args[i];
                codegen_expr(arg, out);
                if (arg->type == AST_STRING) {
                    // String literal is a 64-bit pointer
                    fprintf(out, "    str  x0, [sp, #-16]!\n");
                } else {
                    fprintf(out, "    str  w0, [sp, #-16]!\n");
                }
            }

            // Pop into AAPCS64 parameter registers (x0/w0 to x7/w7)
            for (int i = 0; i < arg_count && i < 8; i++) {
                ASTNode *arg = node->func_call.args[i];
                if (arg->type == AST_STRING) {
                    fprintf(out, "    ldr  x%d, [sp], #16\n", i);
                } else {
                    fprintf(out, "    ldr  w%d, [sp], #16\n", i);
                }
            }

            // Branch with link
            fprintf(out, "    bl   %s\n", node->func_call.name);
            break;
        }

        default:
            break;
    }
}

static void codegen_stmt(const ASTNode *node, FILE *out) {
    if (!node) return;

    switch (node->type) {
        case AST_BLOCK:
            for (int i = 0; i < node->block.stmt_count; i++) {
                codegen_stmt(node->block.stmts[i], out);
            }
            break;

        case AST_VAR_DECL:
            if (node->var_decl.init) {
                codegen_expr(node->var_decl.init, out);
                fprintf(out, "    str  w0, [sp, #%d]\n", node->var_decl.offset);
            }
            break;

        case AST_ASSIGN:
            codegen_expr(node->assign.expr, out);
            fprintf(out, "    str  w0, [sp, #%d]\n", node->assign.offset);
            break;

        case AST_RETURN:
            if (node->return_stmt.expr) {
                codegen_expr(node->return_stmt.expr, out);
            }
            // Branch to function epilogue
            fprintf(out, "    b    .Lfunc_end\n");
            break;

        case AST_IF: {
            int lbl_else = ++g_label_counter;
            int lbl_end = ++g_label_counter;

            codegen_expr(node->if_stmt.condition, out);
            fprintf(out, "    cmp  w0, #0\n");
            fprintf(out, "    b.eq .L%d\n", node->if_stmt.else_branch ? lbl_else : lbl_end);

            codegen_stmt(node->if_stmt.then_branch, out);

            if (node->if_stmt.else_branch) {
                fprintf(out, "    b    .L%d\n", lbl_end);
                fprintf(out, ".L%d:\n", lbl_else);
                codegen_stmt(node->if_stmt.else_branch, out);
            }

            fprintf(out, ".L%d:\n", lbl_end);
            break;
        }

        case AST_WHILE: {
            int lbl_loop = ++g_label_counter;
            int lbl_end = ++g_label_counter;

            fprintf(out, ".L%d:\n", lbl_loop);
            codegen_expr(node->while_stmt.condition, out);
            fprintf(out, "    cmp  w0, #0\n");
            fprintf(out, "    b.eq .L%d\n", lbl_end);

            codegen_stmt(node->while_stmt.body, out);
            fprintf(out, "    b    .L%d\n", lbl_loop);

            fprintf(out, ".L%d:\n", lbl_end);
            break;
        }

        default:
            // Standalone expression statement (e.g. printf(...);)
            codegen_expr(node, out);
            break;
    }
}

static void codegen_function(const ASTNode *fn, FILE *out) {
    const char *name = fn->function.name;
    int locals_bytes = fn->function.symbol_table.total_stack_bytes;

    // AAPCS64 mandates 16-byte stack alignment
    int stack_size = 16 + locals_bytes;
    if (stack_size % 16 != 0) {
        stack_size = ((stack_size + 15) / 16) * 16;
    }

    fprintf(out, "    .globl %s\n", name);
    fprintf(out, "    .type  %s, @function\n", name);
    fprintf(out, "%s:\n", name);

    // Prologue: Save frame pointer and link register
    fprintf(out, "    stp  x29, x30, [sp, #-%d]!\n", stack_size);
    fprintf(out, "    mov  x29, sp\n");

    // Store incoming argument registers into local stack slots
    for (int i = 0; i < fn->function.param_count && i < 8; i++) {
        fprintf(out, "    str  w%d, [sp, #%d]\n", i, (i + 1) * 8);
    }

    // Function Body
    for (int i = 0; i < fn->function.body_count; i++) {
        codegen_stmt(fn->function.body[i], out);
    }

    // Epilogue
    fprintf(out, ".Lfunc_end:\n");
    fprintf(out, "    ldp  x29, x30, [sp], #%d\n", stack_size);
    fprintf(out, "    ret\n\n");
}

void codegen_generate(const ASTNode *program, FILE *out) {
    if (!program || program->type != AST_PROGRAM) return;

    free_strings();
    g_label_counter = 0;

    fprintf(out, "    .arch armv8-a\n");
    fprintf(out, "    .text\n\n");

    // Emit code for each function
    for (int i = 0; i < program->program.function_count; i++) {
        codegen_function(program->program.functions[i], out);
    }

    // Emit read-only data section for accumulated string literals
    if (g_strings) {
        fprintf(out, "    .section .rodata\n");
        StringEntry *curr = g_strings;
        while (curr) {
            fprintf(out, ".LC%d:\n", curr->id);
            fprintf(out, "    .string \"%s\"\n", curr->text);
            curr = curr->next;
        }
        fprintf(out, "\n");
    }

    free_strings();
}