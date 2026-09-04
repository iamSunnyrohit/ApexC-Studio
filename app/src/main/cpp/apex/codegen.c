#include "codegen.h"
#include <stdio.h>
#include <stdlib.h>

static int label_counter = 0;

static void emit_num(int val, FILE *out) {
    if (val >= 0 && val <= 65535) {
        fprintf(out, "    mov w0, #%d\n", val);
    } else if (val < 0 && val >= -65536) {
        fprintf(out, "    mov w0, #%d\n", val);
    } else {
        unsigned int uval = (unsigned int)val;
        fprintf(out, "    mov w0, #%u\n", uval & 0xFFFF);
        if ((uval >> 16) & 0xFFFF) {
            fprintf(out, "    movk w0, #%u, lsl #16\n", (uval >> 16) & 0xFFFF);
        }
    }
}

static void codegen_expr(ASTNode *node, FILE *out) {
    if (!node) return;

    switch (node->type) {
        case AST_NUM:
            emit_num(node->num.value, out);
            break;

        case AST_VAR_REF:
            fprintf(out, "    ldr w0, [x29, #-%d]\n", node->var_ref.offset);
            break;

        case AST_FUNC_CALL:
            for (int i = 0; i < node->func_call.arg_count; i++) {
                codegen_expr(node->func_call.args[i], out);
                fprintf(out, "    str w0, [sp, #-16]!\n");
            }
            for (int i = node->func_call.arg_count - 1; i >= 0; i--) {
                if (i < 8) {
                    fprintf(out, "    ldr w%d, [sp], #16\n", i);
                } else {
                    // Extra args handling if needed, though AAPCS64 first 8 are in w0-w7
                    fprintf(out, "    ldr w0, [sp], #16\n");
                }
            }
            fprintf(out, "    bl %s\n", node->func_call.name);
            break;

        case AST_BINARY_OP:
            codegen_expr(node->binary.left, out);
            fprintf(out, "    str w0, [sp, #-16]!\n");
            codegen_expr(node->binary.right, out);
            fprintf(out, "    ldr w1, [sp], #16\n");

            switch (node->binary.op) {
                case TOKEN_PLUS:
                    fprintf(out, "    add w0, w1, w0\n");
                    break;
                case TOKEN_MINUS:
                    fprintf(out, "    sub w0, w1, w0\n");
                    break;
                case TOKEN_STAR:
                    fprintf(out, "    mul w0, w1, w0\n");
                    break;
                case TOKEN_SLASH:
                    fprintf(out, "    sdiv w0, w1, w0\n");
                    break;
                case TOKEN_EQ:
                    fprintf(out, "    cmp w1, w0\n");
                    fprintf(out, "    cset w0, eq\n");
                    break;
                case TOKEN_NE:
                    fprintf(out, "    cmp w1, w0\n");
                    fprintf(out, "    cset w0, ne\n");
                    break;
                case TOKEN_LT:
                    fprintf(out, "    cmp w1, w0\n");
                    fprintf(out, "    cset w0, lt\n");
                    break;
                case TOKEN_LE:
                    fprintf(out, "    cmp w1, w0\n");
                    fprintf(out, "    cset w0, le\n");
                    break;
                case TOKEN_GT:
                    fprintf(out, "    cmp w1, w0\n");
                    fprintf(out, "    cset w0, gt\n");
                    break;
                case TOKEN_GE:
                    fprintf(out, "    cmp w1, w0\n");
                    fprintf(out, "    cset w0, ge\n");
                    break;
                default:
                    fprintf(stderr, "Codegen Error: Unknown operator token %d\n", node->binary.op);
                    break;
            }
            break;

        default:
            fprintf(stderr, "Codegen Error: Unexpected node type in expression\n");
            break;
    }
}

static void codegen_stmt(ASTNode *node, FILE *out) {
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
                fprintf(out, "    str w0, [x29, #-%d]\n", node->var_decl.offset);
            }
            break;

        case AST_ASSIGN:
            codegen_expr(node->assign.expr, out);
            fprintf(out, "    str w0, [x29, #-%d]\n", node->assign.offset);
            break;

        case AST_RETURN:
            codegen_expr(node->return_stmt.expr, out);
            fprintf(out, "    mov sp, x29\n");
            fprintf(out, "    ldp x29, x30, [sp], #16\n");
            fprintf(out, "    ret\n");
            break;

        case AST_IF: {
            int label_id = ++label_counter;
            codegen_expr(node->if_stmt.condition, out);
            fprintf(out, "    cmp w0, #0\n");
            if (node->if_stmt.else_branch) {
                fprintf(out, "    b.eq .Lelse_%d\n", label_id);
                codegen_stmt(node->if_stmt.then_branch, out);
                fprintf(out, "    b .Lend_%d\n", label_id);
                fprintf(out, ".Lelse_%d:\n", label_id);
                codegen_stmt(node->if_stmt.else_branch, out);
                fprintf(out, ".Lend_%d:\n", label_id);
            } else {
                fprintf(out, "    b.eq .Lend_%d\n", label_id);
                codegen_stmt(node->if_stmt.then_branch, out);
                fprintf(out, ".Lend_%d:\n", label_id);
            }
            break;
        }

        case AST_WHILE: {
            int label_id = ++label_counter;
            fprintf(out, ".Lcond_%d:\n", label_id);
            codegen_expr(node->while_stmt.condition, out);
            fprintf(out, "    cmp w0, #0\n");
            fprintf(out, "    b.eq .Lend_%d\n", label_id);
            codegen_stmt(node->while_stmt.body, out);
            fprintf(out, "    b .Lcond_%d\n", label_id);
            fprintf(out, ".Lend_%d:\n", label_id);
            break;
        }

        case AST_FUNC_CALL:
            codegen_expr(node, out);
            break;

        default:
            fprintf(stderr, "Codegen Error: Unknown statement type\n");
            break;
    }
}

static void codegen_function(ASTNode *node, FILE *out) {
    if (!node || node->type != AST_FUNCTION) return;

    const char *name = node->function.name;
    fprintf(out, "    .global %s\n", name);
    fprintf(out, "    .p2align 2\n");
    fprintf(out, "%s:\n", name);

    int total_bytes = node->function.symbol_table.total_stack_bytes;
    int aligned_size = (total_bytes + 15) & ~15;

    // Prologue
    fprintf(out, "    stp x29, x30, [sp, #-16]!\n");
    fprintf(out, "    mov x29, sp\n");
    if (aligned_size > 0) {
        fprintf(out, "    sub sp, sp, #%d\n", aligned_size);
    }

    // Copy register parameters (w0..w7) to stack slots
    for (int i = 0; i < node->function.param_count && i < 8; i++) {
        int offset = node->function.symbol_table.symbols[i].offset;
        fprintf(out, "    str w%d, [x29, #-%d]\n", i, offset);
    }

    // Body
    for (int i = 0; i < node->function.body_count; i++) {
        codegen_stmt(node->function.body[i], out);
    }

    // Default Epilogue
    fprintf(out, "    mov w0, #0\n");
    fprintf(out, "    mov sp, x29\n");
    fprintf(out, "    ldp x29, x30, [sp], #16\n");
    fprintf(out, "    ret\n");
}

void codegen_generate(ASTNode *program, FILE *out) {
    if (!program || program->type != AST_PROGRAM) return;

    for (int i = 0; i < program->program.function_count; i++) {
        codegen_function(program->program.functions[i], out);
    }
}
