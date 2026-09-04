#include "eval.h"
#include "builtins.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct VarScope {
    char name[64];
    int value;
    struct VarScope *next;
} VarScope;

typedef struct CallFrame {
    VarScope *vars;
    struct CallFrame *next;
} CallFrame;

static const ASTNode *g_program_ast = NULL;
static CallFrame *g_call_stack = NULL;
static int g_has_returned = 0;
static int g_return_value = 0;

static void push_frame(void) {
    CallFrame *frame = calloc(1, sizeof(CallFrame));
    frame->next = g_call_stack;
    g_call_stack = frame;
}

static void pop_frame(void) {
    if (!g_call_stack) return;
    CallFrame *top = g_call_stack;
    g_call_stack = top->next;

    VarScope *v = top->vars;
    while (v) {
        VarScope *next = v->next;
        free(v);
        v = next;
    }
    free(top);
}

static void set_var(const char *name, int value) {
    if (!g_call_stack) return;
    for (VarScope *v = g_call_stack->vars; v; v = v->next) {
        if (strcmp(v->name, name) == 0) {
            v->value = value;
            return;
        }
    }
    VarScope *new_var = calloc(1, sizeof(VarScope));
    strncpy(new_var->name, name, 63);
    new_var->value = value;
    new_var->next = g_call_stack->vars;
    g_call_stack->vars = new_var;
}

static int get_var(const char *name, int *out_val) {
    if (!g_call_stack) return 0;
    for (VarScope *v = g_call_stack->vars; v; v = v->next) {
        if (strcmp(v->name, name) == 0) {
            *out_val = v->value;
            return 1;
        }
    }
    return 0;
}

static const ASTNode *find_function(const char *name) {
    if (!g_program_ast || g_program_ast->type != AST_PROGRAM) return NULL;
    for (int i = 0; i < g_program_ast->program.function_count; i++) {
        const ASTNode *fn = g_program_ast->program.functions[i];
        if (strcmp(fn->function.name, name) == 0) {
            return fn;
        }
    }
    return NULL;
}

static int eval_expr(const ASTNode *node);
static void eval_stmt(const ASTNode *node);

static int eval_expr(const ASTNode *node) {
    if (!node || g_has_returned) return 0;

    switch (node->type) {
        case AST_NUM:
            return node->num.value;

        case AST_STRING:
            // Return raw string pointer cast for direct internal passing
            return (int)(intptr_t)node->str.value;

        case AST_VAR_REF: {
            int val = 0;
            if (get_var(node->var_ref.name, &val)) {
                return val;
            }
            return 0;
        }

        case AST_BINARY_OP: {
            int left = eval_expr(node->binary.left);
            int right = eval_expr(node->binary.right);
            switch (node->binary.op) {
                case TOKEN_PLUS:  return left + right;
                case TOKEN_MINUS: return left - right;
                case TOKEN_STAR:  return left * right;
                case TOKEN_SLASH: return right != 0 ? (left / right) : 0;
                case TOKEN_EQ:    return left == right;
                case TOKEN_NE:    return left != right;
                case TOKEN_LT:    return left < right;
                case TOKEN_LE:    return left <= right;
                case TOKEN_GT:    return left > right;
                case TOKEN_GE:    return left >= right;
                default:          return 0;
            }
        }

        case AST_FUNC_CALL: {
            const char *fn_name = node->func_call.name;

            // 1. Intercept Standard Library Calls (<stdio.h>, <math.h>)
            if (is_builtin_func(fn_name)) {
                int int_args[16] = {0};
                const char *format_str = NULL;
                int int_count = 0;

                int total_args = node->func_call.arg_count < 16 ? node->func_call.arg_count : 16;
                for (int i = 0; i < total_args; i++) {
                    const ASTNode *arg = node->func_call.args[i];
                    if (arg->type == AST_STRING) {
                        if (!format_str) format_str = arg->str.value;
                    } else {
                        int_args[int_count++] = eval_expr(arg);
                    }
                }
                return execute_builtin(fn_name, int_args, NULL, int_count, format_str);
            }

            // 2. User-Defined Functions
            const ASTNode *fn_def = find_function(fn_name);
            if (!fn_def) return 0;

            int evaluated_args[16];
            int count = node->func_call.arg_count < 16 ? node->func_call.arg_count : 16;
            for (int i = 0; i < count; i++) {
                evaluated_args[i] = eval_expr(node->func_call.args[i]);
            }

            push_frame();
            // Bind arguments into parameter symbol names
            for (int i = 0; i < fn_def->function.param_count && i < count; i++) {
                set_var(fn_def->function.symbol_table.symbols[i].name, evaluated_args[i]);
            }

            int prev_returned = g_has_returned;
            int prev_ret_val = g_return_value;
            g_has_returned = 0;
            g_return_value = 0;

            for (int i = 0; i < fn_def->function.body_count && !g_has_returned; i++) {
                eval_stmt(fn_def->function.body[i]);
            }

            int call_result = g_return_value;
            pop_frame();

            g_has_returned = prev_returned;
            g_return_value = prev_ret_val;
            return call_result;
        }

        default:
            return 0;
    }
}

static void eval_stmt(const ASTNode *node) {
    if (!node || g_has_returned) return;

    switch (node->type) {
        case AST_BLOCK:
            for (int i = 0; i < node->block.stmt_count && !g_has_returned; i++) {
                eval_stmt(node->block.stmts[i]);
            }
            break;

        case AST_VAR_DECL:
            set_var(node->var_decl.name, node->var_decl.init ? eval_expr(node->var_decl.init) : 0);
            break;

        case AST_ASSIGN:
            set_var(node->assign.name, eval_expr(node->assign.expr));
            break;

        case AST_RETURN:
            g_return_value = node->return_stmt.expr ? eval_expr(node->return_stmt.expr) : 0;
            g_has_returned = 1;
            break;

        case AST_IF:
            if (eval_expr(node->if_stmt.condition)) {
                eval_stmt(node->if_stmt.then_branch);
            } else if (node->if_stmt.else_branch) {
                eval_stmt(node->if_stmt.else_branch);
            }
            break;

        case AST_WHILE:
            while (eval_expr(node->while_stmt.condition) && !g_has_returned) {
                eval_stmt(node->while_stmt.body);
            }
            break;

        default:
            // Standalone expression statement (e.g. printf(...);)
            eval_expr(node);
            break;
    }
}

int eval_program(const ASTNode *program) {
    if (!program) return -1;

    g_program_ast = program;
    g_call_stack = NULL;
    g_has_returned = 0;
    g_return_value = 0;

    const ASTNode *main_fn = find_function("main");
    if (!main_fn) return -1;

    push_frame();
    for (int i = 0; i < main_fn->function.body_count && !g_has_returned; i++) {
        eval_stmt(main_fn->function.body[i]);
    }
    int final_result = g_return_value;
    pop_frame();

    return final_result;
}