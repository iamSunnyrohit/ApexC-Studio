#include "eval.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char name[64];
    int value;
} Variable;

typedef struct Env {
    Variable vars[64];
    int var_count;
    struct Env *parent;
} Env;

static ASTNode *g_program = NULL;

static int eval_expr(ASTNode *node, Env *env);
static int eval_stmt(ASTNode *node, Env *env, int *returned, int *ret_val);

static ASTNode *find_function(const char *name) {
    if (!g_program || g_program->type != AST_PROGRAM) return NULL;
    for (int i = 0; i < g_program->program.function_count; i++) {
        ASTNode *fn = g_program->program.functions[i];
        if (strcmp(fn->function.name, name) == 0) {
            return fn;
        }
    }
    return NULL;
}

static void set_var(Env *env, const char *name, int val) {
    for (int i = 0; i < env->var_count; i++) {
        if (strcmp(env->vars[i].name, name) == 0) {
            env->vars[i].value = val;
            return;
        }
    }
    if (env->var_count < 64) {
        strncpy(env->vars[env->var_count].name, name, 63);
        env->vars[env->var_count].value = val;
        env->var_count++;
    }
}

static int get_var(Env *env, const char *name) {
    for (Env *cur = env; cur != NULL; cur = cur->parent) {
        for (int i = 0; i < cur->var_count; i++) {
            if (strcmp(cur->vars[i].name, name) == 0) {
                return cur->vars[i].value;
            }
        }
    }
    return 0;
}

static int call_func(const char *name, int *args, int arg_count) {
    ASTNode *fn = find_function(name);
    if (!fn) return 0;

    Env call_env = {0};
    call_env.parent = NULL;

    for (int i = 0; i < fn->function.param_count && i < arg_count; i++) {
        const char *pname = fn->function.symbol_table.symbols[i].name;
        set_var(&call_env, pname, args[i]);
    }

    int returned = 0;
    int ret_val = 0;
    for (int i = 0; i < fn->function.body_count && !returned; i++) {
        eval_stmt(fn->function.body[i], &call_env, &returned, &ret_val);
    }
    return ret_val;
}

static int eval_expr(ASTNode *node, Env *env) {
    if (!node) return 0;

    switch (node->type) {
        case AST_NUM:
            return node->num.value;

        case AST_VAR_REF:
            return get_var(env, node->var_ref.name);

        case AST_FUNC_CALL: {
            int args[16];
            int count = node->func_call.arg_count < 16 ? node->func_call.arg_count : 16;
            for (int i = 0; i < count; i++) {
                args[i] = eval_expr(node->func_call.args[i], env);
            }
            return call_func(node->func_call.name, args, count);
        }

        case AST_BINARY_OP: {
            int l = eval_expr(node->binary.left, env);
            int r = eval_expr(node->binary.right, env);
            switch (node->binary.op) {
                case TOKEN_PLUS:  return l + r;
                case TOKEN_MINUS: return l - r;
                case TOKEN_STAR:  return l * r;
                case TOKEN_SLASH: return r != 0 ? l / r : 0;
                case TOKEN_EQ:    return l == r;
                case TOKEN_NE:    return l != r;
                case TOKEN_LT:    return l < r;
                case TOKEN_LE:    return l <= r;
                case TOKEN_GT:    return l > r;
                case TOKEN_GE:    return l >= r;
                default: return 0;
            }
        }
        default:
            return 0;
    }
}

static int eval_stmt(ASTNode *node, Env *env, int *returned, int *ret_val) {
    if (!node || *returned) return 0;

    switch (node->type) {
        case AST_VAR_DECL:
            if (node->var_decl.init) {
                int val = eval_expr(node->var_decl.init, env);
                set_var(env, node->var_decl.name, val);
            }
            break;

        case AST_ASSIGN: {
            int val = eval_expr(node->assign.expr, env);
            set_var(env, node->assign.name, val);
            break;
        }

        case AST_RETURN:
            *ret_val = eval_expr(node->return_stmt.expr, env);
            *returned = 1;
            break;

        case AST_IF: {
            int cond = eval_expr(node->if_stmt.condition, env);
            if (cond) {
                eval_stmt(node->if_stmt.then_branch, env, returned, ret_val);
            } else if (node->if_stmt.else_branch) {
                eval_stmt(node->if_stmt.else_branch, env, returned, ret_val);
            }
            break;
        }

        case AST_WHILE:
            while (eval_expr(node->while_stmt.condition, env) && !(*returned)) {
                eval_stmt(node->while_stmt.body, env, returned, ret_val);
            }
            break;

        case AST_BLOCK:
            for (int i = 0; i < node->block.stmt_count && !(*returned); i++) {
                eval_stmt(node->block.stmts[i], env, returned, ret_val);
            }
            break;

        case AST_FUNC_CALL:
            eval_expr(node, env);
            break;

        default:
            break;
    }
    return 0;
}

int eval_program(ASTNode *program) {
    g_program = program;
    return call_func("main", NULL, 0);
}