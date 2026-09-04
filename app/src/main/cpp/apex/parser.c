#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void advance(Parser *p) {
    token_free(&p->current_token);
    p->current_token = lexer_next_token(&p->lexer);
}

static int match(Parser *p, TokenType type) {
    return p->current_token.type == type;
}

static int expect(Parser *p, TokenType type) {
    if (match(p, type)) {
        advance(p);
        return 1;
    }
    return 0;
}

static int symtab_lookup(SymbolTable *st, const char *name) {
    for (int i = 0; i < st->symbol_count; i++) {
        if (strcmp(st->symbols[i].name, name) == 0) {
            return st->symbols[i].offset;
        }
    }
    return -1;
}

static int symtab_add(SymbolTable *st, const char *name) {
    int existing = symtab_lookup(st, name);
    if (existing != -1) return existing;

    if (st->symbol_count < 64) {
        st->total_stack_bytes += 8; // Align 8-byte stack allocation slots
        strncpy(st->symbols[st->symbol_count].name, name, 63);
        st->symbols[st->symbol_count].offset = st->total_stack_bytes;
        return st->symbols[st->symbol_count++].offset;
    }
    return 0;
}

// Forward declarations
static ASTNode *parse_expr(Parser *p);
static ASTNode *parse_stmt(Parser *p);
static ASTNode *parse_block(Parser *p);

static ASTNode *parse_primary(Parser *p) {
    if (match(p, TOKEN_INT_LIT)) {
        ASTNode *node = calloc(1, sizeof(ASTNode));
        node->type = AST_NUM;
        node->num.value = p->current_token.int_value;
        advance(p);
        return node;
    }

    if (match(p, TOKEN_STR_LIT)) {
        ASTNode *node = calloc(1, sizeof(ASTNode));
        node->type = AST_STRING;
        node->str.value = strdup(p->current_token.text);
        node->str.label_id = ++p->string_literal_count;
        advance(p);
        return node;
    }

    if (match(p, TOKEN_IDENT)) {
        char name[64];
        strncpy(name, p->current_token.text, 63);
        name[63] = '\0';
        advance(p);

        // Function call: ident(...)
        if (match(p, TOKEN_LPAREN)) {
            advance(p);
            ASTNode *node = calloc(1, sizeof(ASTNode));
            node->type = AST_FUNC_CALL;
            strncpy(node->func_call.name, name, 63);

            ASTNode *args[16];
            int count = 0;
            while (!match(p, TOKEN_RPAREN) && !match(p, TOKEN_EOF) && count < 16) {
                args[count++] = parse_expr(p);
                if (match(p, TOKEN_COMMA)) {
                    advance(p);
                } else {
                    break;
                }
            }
            expect(p, TOKEN_RPAREN);

            node->func_call.arg_count = count;
            node->func_call.args = malloc(sizeof(ASTNode*) * count);
            memcpy(node->func_call.args, args, sizeof(ASTNode*) * count);
            return node;
        }

        // Variable reference
        ASTNode *node = calloc(1, sizeof(ASTNode));
        node->type = AST_VAR_REF;
        strncpy(node->var_ref.name, name, 63);
        node->var_ref.offset = p->current_symtab ? symtab_lookup(p->current_symtab, name) : 0;
        return node;
    }

    if (match(p, TOKEN_LPAREN)) {
        advance(p);
        ASTNode *expr = parse_expr(p);
        expect(p, TOKEN_RPAREN);
        return expr;
    }

    return NULL;
}

static ASTNode *parse_multiplicative(Parser *p) {
    ASTNode *left = parse_primary(p);
    while (match(p, TOKEN_STAR) || match(p, TOKEN_SLASH)) {
        TokenType op = p->current_token.type;
        advance(p);
        ASTNode *right = parse_primary(p);
        ASTNode *bin = calloc(1, sizeof(ASTNode));
        bin->type = AST_BINARY_OP;
        bin->binary.op = op;
        bin->binary.left = left;
        bin->binary.right = right;
        left = bin;
    }
    return left;
}

static ASTNode *parse_additive(Parser *p) {
    ASTNode *left = parse_multiplicative(p);
    while (match(p, TOKEN_PLUS) || match(p, TOKEN_MINUS)) {
        TokenType op = p->current_token.type;
        advance(p);
        ASTNode *right = parse_multiplicative(p);
        ASTNode *bin = calloc(1, sizeof(ASTNode));
        bin->type = AST_BINARY_OP;
        bin->binary.op = op;
        bin->binary.left = left;
        bin->binary.right = right;
        left = bin;
    }
    return left;
}

static ASTNode *parse_relational(Parser *p) {
    ASTNode *left = parse_additive(p);
    while (match(p, TOKEN_LT) || match(p, TOKEN_LE) ||
           match(p, TOKEN_GT) || match(p, TOKEN_GE) ||
           match(p, TOKEN_EQ) || match(p, TOKEN_NE)) {
        TokenType op = p->current_token.type;
        advance(p);
        ASTNode *right = parse_additive(p);
        ASTNode *bin = calloc(1, sizeof(ASTNode));
        bin->type = AST_BINARY_OP;
        bin->binary.op = op;
        bin->binary.left = left;
        bin->binary.right = right;
        left = bin;
    }
    return left;
}

static ASTNode *parse_expr(Parser *p) {
    return parse_relational(p);
}

static ASTNode *parse_stmt(Parser *p) {
    if (match(p, TOKEN_LBRACE)) {
        return parse_block(p);
    }

    // Variable declaration: int x = expr;
    if (match(p, TOKEN_KW_INT)) {
        advance(p);
        char name[64];
        strncpy(name, p->current_token.text, 63);
        name[63] = '\0';
        expect(p, TOKEN_IDENT);

        int offset = p->current_symtab ? symtab_add(p->current_symtab, name) : 0;

        ASTNode *init_expr = NULL;
        if (match(p, TOKEN_ASSIGN)) {
            advance(p);
            init_expr = parse_expr(p);
        }
        expect(p, TOKEN_SEMI);

        ASTNode *node = calloc(1, sizeof(ASTNode));
        node->type = AST_VAR_DECL;
        strncpy(node->var_decl.name, name, 63);
        node->var_decl.offset = offset;
        node->var_decl.init = init_expr;
        return node;
    }

    // Return statement: return expr;
    if (match(p, TOKEN_KW_RETURN)) {
        advance(p);
        ASTNode *expr = parse_expr(p);
        expect(p, TOKEN_SEMI);

        ASTNode *node = calloc(1, sizeof(ASTNode));
        node->type = AST_RETURN;
        node->return_stmt.expr = expr;
        return node;
    }

    // If statement: if (cond) stmt [else stmt]
    if (match(p, TOKEN_KW_IF)) {
        advance(p);
        expect(p, TOKEN_LPAREN);
        ASTNode *cond = parse_expr(p);
        expect(p, TOKEN_RPAREN);

        ASTNode *then_branch = parse_stmt(p);
        ASTNode *else_branch = NULL;
        if (match(p, TOKEN_KW_ELSE)) {
            advance(p);
            else_branch = parse_stmt(p);
        }

        ASTNode *node = calloc(1, sizeof(ASTNode));
        node->type = AST_IF;
        node->if_stmt.condition = cond;
        node->if_stmt.then_branch = then_branch;
        node->if_stmt.else_branch = else_branch;
        return node;
    }

    // While statement: while (cond) stmt
    if (match(p, TOKEN_KW_WHILE)) {
        advance(p);
        expect(p, TOKEN_LPAREN);
        ASTNode *cond = parse_expr(p);
        expect(p, TOKEN_RPAREN);

        ASTNode *body = parse_stmt(p);

        ASTNode *node = calloc(1, sizeof(ASTNode));
        node->type = AST_WHILE;
        node->while_stmt.condition = cond;
        node->while_stmt.body = body;
        return node;
    }

    // Identifier starting line: assignment or function call
    if (match(p, TOKEN_IDENT)) {
        Token next = lexer_peek_token(&p->lexer);
        if (next.type == TOKEN_ASSIGN) {
            char name[64];
            strncpy(name, p->current_token.text, 63);
            name[63] = '\0';
            advance(p); // ident
            advance(p); // =
            ASTNode *expr = parse_expr(p);
            expect(p, TOKEN_SEMI);

            ASTNode *node = calloc(1, sizeof(ASTNode));
            node->type = AST_ASSIGN;
            strncpy(node->assign.name, name, 63);
            node->assign.offset = p->current_symtab ? symtab_lookup(p->current_symtab, name) : 0;
            node->assign.expr = expr;
            token_free(&next);
            return node;
        }
        token_free(&next);
    }

    // Expression statement (e.g. printf(...);)
    ASTNode *expr = parse_expr(p);
    expect(p, TOKEN_SEMI);
    return expr;
}

static ASTNode *parse_block(Parser *p) {
    expect(p, TOKEN_LBRACE);
    ASTNode *stmts[128];
    int count = 0;

    while (!match(p, TOKEN_RBRACE) && !match(p, TOKEN_EOF) && count < 128) {
        stmts[count++] = parse_stmt(p);
    }
    expect(p, TOKEN_RBRACE);

    ASTNode *node = calloc(1, sizeof(ASTNode));
    node->type = AST_BLOCK;
    node->block.stmt_count = count;
    node->block.stmts = malloc(sizeof(ASTNode*) * count);
    memcpy(node->block.stmts, stmts, sizeof(ASTNode*) * count);
    return node;
}

static ASTNode *parse_function(Parser *p) {
    expect(p, TOKEN_KW_INT); // return type (int)

    char name[64];
    strncpy(name, p->current_token.text, 63);
    name[63] = '\0';
    expect(p, TOKEN_IDENT);

    ASTNode *fn = calloc(1, sizeof(ASTNode));
    fn->type = AST_FUNCTION;
    strncpy(fn->function.name, name, 63);

    p->current_symtab = &fn->function.symbol_table;
    memset(p->current_symtab, 0, sizeof(SymbolTable));

    expect(p, TOKEN_LPAREN);
    int param_count = 0;
    while (!match(p, TOKEN_RPAREN) && !match(p, TOKEN_EOF)) {
        expect(p, TOKEN_KW_INT);
        char pname[64];
        strncpy(pname, p->current_token.text, 63);
        pname[63] = '\0';
        expect(p, TOKEN_IDENT);

        symtab_add(p->current_symtab, pname);
        param_count++;

        if (match(p, TOKEN_COMMA)) {
            advance(p);
        } else {
            break;
        }
    }
    expect(p, TOKEN_RPAREN);
    fn->function.param_count = param_count;

    expect(p, TOKEN_LBRACE);
    ASTNode *body[128];
    int count = 0;
    while (!match(p, TOKEN_RBRACE) && !match(p, TOKEN_EOF) && count < 128) {
        body[count++] = parse_stmt(p);
    }
    expect(p, TOKEN_RBRACE);

    fn->function.body_count = count;
    fn->function.body = malloc(sizeof(ASTNode*) * count);
    memcpy(fn->function.body, body, sizeof(ASTNode*) * count);

    p->current_symtab = NULL;
    return fn;
}

Parser parser_init(const char *src) {
    Parser p;
    p.lexer = lexer_init(src);
    p.current_token.text = NULL;
    p.current_symtab = NULL;
    p.string_literal_count = 0;
    advance(&p);
    return p;
}

ASTNode *parse_program(Parser *p) {
    ASTNode *fns[32];
    int count = 0;

    while (!match(p, TOKEN_EOF) && count < 32) {
        fns[count++] = parse_function(p);
    }

    ASTNode *prog = calloc(1, sizeof(ASTNode));
    prog->type = AST_PROGRAM;
    prog->program.function_count = count;
    prog->program.functions = malloc(sizeof(ASTNode*) * count);
    memcpy(prog->program.functions, fns, sizeof(ASTNode*) * count);
    return prog;
}

void ast_free(ASTNode *node) {
    if (!node) return;
    switch (node->type) {
        case AST_PROGRAM:
            for (int i = 0; i < node->program.function_count; i++) {
                ast_free(node->program.functions[i]);
            }
            free(node->program.functions);
            break;
        case AST_FUNCTION:
            for (int i = 0; i < node->function.body_count; i++) {
                ast_free(node->function.body[i]);
            }
            free(node->function.body);
            break;
        case AST_BLOCK:
            for (int i = 0; i < node->block.stmt_count; i++) {
                ast_free(node->block.stmts[i]);
            }
            free(node->block.stmts);
            break;
        case AST_VAR_DECL:
            if (node->var_decl.init) ast_free(node->var_decl.init);
            break;
        case AST_ASSIGN:
            if (node->assign.expr) ast_free(node->assign.expr);
            break;
        case AST_RETURN:
            if (node->return_stmt.expr) ast_free(node->return_stmt.expr);
            break;
        case AST_IF:
            ast_free(node->if_stmt.condition);
            ast_free(node->if_stmt.then_branch);
            if (node->if_stmt.else_branch) ast_free(node->if_stmt.else_branch);
            break;
        case AST_WHILE:
            ast_free(node->while_stmt.condition);
            ast_free(node->while_stmt.body);
            break;
        case AST_BINARY_OP:
            ast_free(node->binary.left);
            ast_free(node->binary.right);
            break;
        case AST_STRING:
            if (node->str.value) free(node->str.value);
            break;
        case AST_FUNC_CALL:
            for (int i = 0; i < node->func_call.arg_count; i++) {
                ast_free(node->func_call.args[i]);
            }
            free(node->func_call.args);
            break;
        default:
            break;
    }
    free(node);
}