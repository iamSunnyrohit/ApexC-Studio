#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

static void advance(Parser *p) {
    token_free(&p->current_token);
    p->current_token = lexer_next_token(&p->lexer);
}

static void parser_error(Parser *p, const char *fmt, ...) {
    if (p->has_error) return;
    p->has_error = true;

    va_list args;
    va_start(args, fmt);
    vsnprintf(p->error_msg, sizeof(p->error_msg), fmt, args);
    va_end(args);

    fprintf(stderr, "Syntax Error at line %d, col %d: %s\n",
            p->current_token.line, p->current_token.col, p->error_msg);
}

static bool expect(Parser *p, TokenType type, const char *expected_desc) {
    if (p->current_token.type != type) {
        parser_error(p, "Expected '%s', got '%s' (%s)",
                     expected_desc, token_type_to_string(p->current_token.type),
                     p->current_token.text ? p->current_token.text : "");
        return false;
    }
    advance(p);
    return true;
}

Parser parser_init(const char *src) {
    Parser p;
    p.lexer = lexer_init(src);
    p.has_error = false;
    p.error_msg[0] = '\0';
    p.current_token = lexer_next_token(&p.lexer);
    p.current_symtab = NULL;
    return p;
}

static void symbol_table_init(SymbolTable *symtab) {
    symtab->symbols = NULL;
    symtab->count = 0;
    symtab->capacity = 0;
    symtab->total_stack_bytes = 0;
}

static Symbol *symbol_table_lookup(const SymbolTable *symtab, const char *name) {
    if (!symtab) return NULL;
    for (int i = 0; i < symtab->count; i++) {
        if (strcmp(symtab->symbols[i].name, name) == 0) {
            return &symtab->symbols[i];
        }
    }
    return NULL;
}

static Symbol *symbol_table_add(SymbolTable *symtab, const char *name) {
    if (symtab->count >= symtab->capacity) {
        symtab->capacity = symtab->capacity == 0 ? 8 : symtab->capacity * 2;
        symtab->symbols = realloc(symtab->symbols, symtab->capacity * sizeof(Symbol));
    }
    symtab->total_stack_bytes += 4; // Each int variable is 4 bytes
    Symbol *sym = &symtab->symbols[symtab->count++];
    sym->name = strdup(name);
    sym->offset = symtab->total_stack_bytes; // Offset: 4, 8, 12, ...
    return sym;
}

static void symbol_table_free(SymbolTable *symtab) {
    if (!symtab) return;
    for (int i = 0; i < symtab->count; i++) {
        if (symtab->symbols[i].name) free(symtab->symbols[i].name);
    }
    if (symtab->symbols) free(symtab->symbols);
    symtab->symbols = NULL;
    symtab->count = 0;
    symtab->capacity = 0;
    symtab->total_stack_bytes = 0;
}

static ASTNode *create_num_node(int val) {
    ASTNode *node = calloc(1, sizeof(ASTNode));
    node->type = AST_NUM;
    node->num.value = val;
    return node;
}

static ASTNode *create_binary_node(TokenType op, ASTNode *left, ASTNode *right) {
    ASTNode *node = calloc(1, sizeof(ASTNode));
    node->type = AST_BINARY_OP;
    node->binary.op = op;
    node->binary.left = left;
    node->binary.right = right;
    return node;
}

static ASTNode *parse_expr(Parser *p, int min_prec);

static ASTNode *parse_primary(Parser *p) {
    if (p->has_error) return NULL;

    if (p->current_token.type == TOKEN_INT_LIT) {
        int val = p->current_token.int_value;
        advance(p);
        return create_num_node(val);
    }

    if (p->current_token.type == TOKEN_IDENT) {
        char *name = p->current_token.text;
        Token peek = lexer_peek_token(&p->lexer);
        if (peek.type == TOKEN_LPAREN) {
            char *func_name = strdup(name);
            advance(p); // Consume ident
            advance(p); // Consume '('

            int cap = 4;
            int count = 0;
            ASTNode **args = malloc(cap * sizeof(ASTNode *));

            while (p->current_token.type != TOKEN_RPAREN && p->current_token.type != TOKEN_EOF && !p->has_error) {
                ASTNode *arg = parse_expr(p, 0);
                if (!arg) break;
                if (count >= cap) {
                    cap *= 2;
                    args = realloc(args, cap * sizeof(ASTNode *));
                }
                args[count++] = arg;
                if (p->current_token.type == TOKEN_COMMA) {
                    advance(p); // Consume ','
                } else {
                    break;
                }
            }

            if (!expect(p, TOKEN_RPAREN, ")")) {
                for (int i = 0; i < count; i++) ast_free(args[i]);
                free(args);
                free(func_name);
                return NULL;
            }

            ASTNode *node = calloc(1, sizeof(ASTNode));
            node->type = AST_FUNC_CALL;
            node->func_call.name = func_name;
            node->func_call.args = args;
            node->func_call.arg_count = count;
            return node;
        } else {
            Symbol *sym = symbol_table_lookup(p->current_symtab, name);
            if (!sym) {
                parser_error(p, "Undeclared variable '%s'", name);
                advance(p);
                return NULL;
            }
            ASTNode *node = calloc(1, sizeof(ASTNode));
            node->type = AST_VAR_REF;
            node->var_ref.name = strdup(name);
            node->var_ref.offset = sym->offset;
            advance(p);
            return node;
        }
    }

    if (p->current_token.type == TOKEN_LPAREN) {
        advance(p); // Consume '('
        ASTNode *expr = parse_expr(p, 0);
        if (!expect(p, TOKEN_RPAREN, ")")) {
            ast_free(expr);
            return NULL;
        }
        return expr;
    }

    if (p->current_token.type == TOKEN_MINUS) {
        advance(p);
        ASTNode *operand = parse_primary(p);
        return create_binary_node(TOKEN_MINUS, create_num_node(0), operand);
    }

    if (p->current_token.type == TOKEN_PLUS) {
        advance(p);
        return parse_primary(p);
    }

    parser_error(p, "Expected integer literal, identifier, or '(', got '%s'",
                 token_type_to_string(p->current_token.type));
    return NULL;
}

static int get_precedence(TokenType type) {
    switch (type) {
        case TOKEN_STAR:
        case TOKEN_SLASH:
            return 40;
        case TOKEN_PLUS:
        case TOKEN_MINUS:
            return 30;
        case TOKEN_LT:
        case TOKEN_LE:
        case TOKEN_GT:
        case TOKEN_GE:
            return 20;
        case TOKEN_EQ:
        case TOKEN_NE:
            return 10;
        default:
            return 0;
    }
}

static ASTNode *parse_expr(Parser *p, int min_prec) {
    if (p->has_error) return NULL;

    ASTNode *left = parse_primary(p);
    if (!left || p->has_error) return left;

    while (1) {
        TokenType type = p->current_token.type;
        int prec = get_precedence(type);
        if (prec < min_prec || prec == 0) {
            break;
        }

        TokenType op = type;
        advance(p);

        ASTNode *right = parse_expr(p, prec + 1); // Left-associative
        if (!right) {
            ast_free(left);
            return NULL;
        }

        left = create_binary_node(op, left, right);
    }

    return left;
}

static ASTNode *parse_statement(Parser *p) {
    if (p->has_error) return NULL;

    if (p->current_token.type == TOKEN_LBRACE) {
        advance(p); // Consume '{'
        int cap = 8;
        int count = 0;
        ASTNode **stmts = malloc(cap * sizeof(ASTNode *));

        while (p->current_token.type != TOKEN_RBRACE && p->current_token.type != TOKEN_EOF && !p->has_error) {
            ASTNode *s = parse_statement(p);
            if (!s) break;
            if (count >= cap) {
                cap *= 2;
                stmts = realloc(stmts, cap * sizeof(ASTNode *));
            }
            stmts[count++] = s;
        }

        if (!expect(p, TOKEN_RBRACE, "}")) {
            for (int i = 0; i < count; i++) ast_free(stmts[i]);
            free(stmts);
            return NULL;
        }

        ASTNode *node = calloc(1, sizeof(ASTNode));
        node->type = AST_BLOCK;
        node->block.stmts = stmts;
        node->block.stmt_count = count;
        return node;
    }

    if (p->current_token.type == TOKEN_KW_RETURN) {
        advance(p); // Consume 'return'
        ASTNode *expr = parse_expr(p, 0);
        if (!expect(p, TOKEN_SEMI, ";")) {
            ast_free(expr);
            return NULL;
        }

        ASTNode *node = calloc(1, sizeof(ASTNode));
        node->type = AST_RETURN;
        node->return_stmt.expr = expr;
        return node;
    }

    if (p->current_token.type == TOKEN_KW_IF) {
        advance(p); // Consume 'if'
        if (!expect(p, TOKEN_LPAREN, "(")) return NULL;
        ASTNode *cond = parse_expr(p, 0);
        if (!expect(p, TOKEN_RPAREN, ")")) {
            if (cond) ast_free(cond);
            return NULL;
        }
        ASTNode *then_b = parse_statement(p);
        ASTNode *else_b = NULL;
        if (p->current_token.type == TOKEN_KW_ELSE) {
            advance(p); // Consume 'else'
            else_b = parse_statement(p);
        }
        ASTNode *node = calloc(1, sizeof(ASTNode));
        node->type = AST_IF;
        node->if_stmt.condition = cond;
        node->if_stmt.then_branch = then_b;
        node->if_stmt.else_branch = else_b;
        return node;
    }

    if (p->current_token.type == TOKEN_KW_WHILE) {
        advance(p); // Consume 'while'
        if (!expect(p, TOKEN_LPAREN, "(")) return NULL;
        ASTNode *cond = parse_expr(p, 0);
        if (!expect(p, TOKEN_RPAREN, ")")) {
            if (cond) ast_free(cond);
            return NULL;
        }
        ASTNode *body = parse_statement(p);
        ASTNode *node = calloc(1, sizeof(ASTNode));
        node->type = AST_WHILE;
        node->while_stmt.condition = cond;
        node->while_stmt.body = body;
        return node;
    }

    if (p->current_token.type == TOKEN_KW_INT) {
        advance(p); // Consume 'int'
        if (p->current_token.type != TOKEN_IDENT) {
            parser_error(p, "Expected variable name after 'int', got '%s'",
                         token_type_to_string(p->current_token.type));
            return NULL;
        }

        char *var_name = strdup(p->current_token.text);
        advance(p);

        if (symbol_table_lookup(p->current_symtab, var_name)) {
            parser_error(p, "Redeclaration of variable '%s'", var_name);
            free(var_name);
            return NULL;
        }

        Symbol *sym = symbol_table_add(p->current_symtab, var_name);

        ASTNode *init = NULL;
        if (p->current_token.type == TOKEN_ASSIGN) {
            advance(p); // Consume '='
            init = parse_expr(p, 0);
            if (!init || p->has_error) {
                free(var_name);
                return NULL;
            }
        }

        if (!expect(p, TOKEN_SEMI, ";")) {
            if (init) ast_free(init);
            free(var_name);
            return NULL;
        }

        ASTNode *node = calloc(1, sizeof(ASTNode));
        node->type = AST_VAR_DECL;
        node->var_decl.name = var_name;
        node->var_decl.offset = sym->offset;
        node->var_decl.init = init;
        return node;
    }

    if (p->current_token.type == TOKEN_IDENT) {
        Token peek = lexer_peek_token(&p->lexer);
        if (peek.type == TOKEN_ASSIGN) {
            char *var_name = strdup(p->current_token.text);
            advance(p); // Consume ident
            advance(p); // Consume '='

            Symbol *sym = symbol_table_lookup(p->current_symtab, var_name);
            if (!sym) {
                parser_error(p, "Undeclared variable '%s'", var_name);
                free(var_name);
                return NULL;
            }

            ASTNode *expr = parse_expr(p, 0);
            if (!expr || p->has_error) {
                free(var_name);
                return NULL;
            }

            if (!expect(p, TOKEN_SEMI, ";")) {
                ast_free(expr);
                free(var_name);
                return NULL;
            }

            ASTNode *node = calloc(1, sizeof(ASTNode));
            node->type = AST_ASSIGN;
            node->assign.name = var_name;
            node->assign.offset = sym->offset;
            node->assign.expr = expr;
            return node;
        } else {
            ASTNode *expr = parse_expr(p, 0);
            if (!expect(p, TOKEN_SEMI, ";")) {
                if (expr) ast_free(expr);
                return NULL;
            }
            return expr;
        }
    }

    parser_error(p, "Expected statement, got '%s'",
                 token_type_to_string(p->current_token.type));
    return NULL;
}

static ASTNode *parse_function(Parser *p) {
    if (p->has_error) return NULL;

    if (!expect(p, TOKEN_KW_INT, "int")) return NULL;

    if (p->current_token.type != TOKEN_IDENT) {
        parser_error(p, "Expected function name, got '%s'",
                     token_type_to_string(p->current_token.type));
        return NULL;
    }

    char *func_name = strdup(p->current_token.text);
    advance(p);

    if (!expect(p, TOKEN_LPAREN, "(")) {
        free(func_name);
        return NULL;
    }

    SymbolTable symtab;
    symbol_table_init(&symtab);
    p->current_symtab = &symtab;

    int param_cap = 4;
    int param_count = 0;
    char **params = malloc(param_cap * sizeof(char *));

    while (p->current_token.type != TOKEN_RPAREN && p->current_token.type != TOKEN_EOF && !p->has_error) {
        if (!expect(p, TOKEN_KW_INT, "int")) break;
        if (p->current_token.type != TOKEN_IDENT) {
            parser_error(p, "Expected parameter name, got '%s'",
                         token_type_to_string(p->current_token.type));
            break;
        }
        char *pname = strdup(p->current_token.text);
        advance(p);

        symbol_table_add(&symtab, pname);

        if (param_count >= param_cap) {
            param_cap *= 2;
            params = realloc(params, param_cap * sizeof(char *));
        }
        params[param_count++] = pname;

        if (p->current_token.type == TOKEN_COMMA) {
            advance(p);
        } else {
            break;
        }
    }

    if (!expect(p, TOKEN_RPAREN, ")")) {
        for (int i = 0; i < param_count; i++) free(params[i]);
        free(params);
        free(func_name);
        symbol_table_free(&symtab);
        p->current_symtab = NULL;
        return NULL;
    }

    if (!expect(p, TOKEN_LBRACE, "{")) {
        for (int i = 0; i < param_count; i++) free(params[i]);
        free(params);
        free(func_name);
        symbol_table_free(&symtab);
        p->current_symtab = NULL;
        return NULL;
    }

    int capacity = 8;
    int count = 0;
    ASTNode **body = malloc(capacity * sizeof(ASTNode *));

    while (p->current_token.type != TOKEN_RBRACE &&
           p->current_token.type != TOKEN_EOF &&
           !p->has_error) {
        ASTNode *stmt = parse_statement(p);
        if (!stmt) break;

        if (count >= capacity) {
            capacity *= 2;
            body = realloc(body, capacity * sizeof(ASTNode *));
        }
        body[count++] = stmt;
    }

    p->current_symtab = NULL;

    if (!expect(p, TOKEN_RBRACE, "}")) {
        for (int i = 0; i < count; i++) ast_free(body[i]);
        free(body);
        for (int i = 0; i < param_count; i++) free(params[i]);
        free(params);
        free(func_name);
        symbol_table_free(&symtab);
        return NULL;
    }

    ASTNode *node = calloc(1, sizeof(ASTNode));
    node->type = AST_FUNCTION;
    node->function.name = func_name;
    node->function.params = params;
    node->function.param_count = param_count;
    node->function.body = body;
    node->function.body_count = count;
    node->function.symbol_table = symtab;
    return node;
}

ASTNode *parse_program(Parser *p) {
    int capacity = 8;
    int count = 0;
    ASTNode **functions = malloc(capacity * sizeof(ASTNode *));

    while (p->current_token.type != TOKEN_EOF && !p->has_error) {
        ASTNode *func = parse_function(p);
        if (!func) break;

        if (count >= capacity) {
            capacity *= 2;
            functions = realloc(functions, capacity * sizeof(ASTNode *));
        }
        functions[count++] = func;
    }

    if (p->has_error) {
        for (int i = 0; i < count; i++) ast_free(functions[i]);
        free(functions);
        return NULL;
    }

    ASTNode *program = calloc(1, sizeof(ASTNode));
    program->type = AST_PROGRAM;
    program->program.functions = functions;
    program->program.function_count = count;
    return program;
}

void ast_free(ASTNode *node) {
    if (!node) return;

    switch (node->type) {
        case AST_NUM:
            break;
        case AST_BINARY_OP:
            ast_free(node->binary.left);
            ast_free(node->binary.right);
            break;
        case AST_RETURN:
            ast_free(node->return_stmt.expr);
            break;
        case AST_VAR_DECL:
            if (node->var_decl.name) free(node->var_decl.name);
            if (node->var_decl.init) ast_free(node->var_decl.init);
            break;
        case AST_ASSIGN:
            if (node->assign.name) free(node->assign.name);
            if (node->assign.expr) ast_free(node->assign.expr);
            break;
        case AST_VAR_REF:
            if (node->var_ref.name) free(node->var_ref.name);
            break;
        case AST_IF:
            if (node->if_stmt.condition) ast_free(node->if_stmt.condition);
            if (node->if_stmt.then_branch) ast_free(node->if_stmt.then_branch);
            if (node->if_stmt.else_branch) ast_free(node->if_stmt.else_branch);
            break;
        case AST_WHILE:
            if (node->while_stmt.condition) ast_free(node->while_stmt.condition);
            if (node->while_stmt.body) ast_free(node->while_stmt.body);
            break;
        case AST_BLOCK:
            for (int i = 0; i < node->block.stmt_count; i++) {
                ast_free(node->block.stmts[i]);
            }
            if (node->block.stmts) free(node->block.stmts);
            break;
        case AST_FUNC_CALL:
            if (node->func_call.name) free(node->func_call.name);
            for (int i = 0; i < node->func_call.arg_count; i++) {
                ast_free(node->func_call.args[i]);
            }
            if (node->func_call.args) free(node->func_call.args);
            break;
        case AST_FUNCTION:
            if (node->function.name) free(node->function.name);
            for (int i = 0; i < node->function.param_count; i++) {
                free(node->function.params[i]);
            }
            if (node->function.params) free(node->function.params);
            for (int i = 0; i < node->function.body_count; i++) {
                ast_free(node->function.body[i]);
            }
            if (node->function.body) free(node->function.body);
            symbol_table_free(&node->function.symbol_table);
            break;
        case AST_PROGRAM:
            for (int i = 0; i < node->program.function_count; i++) {
                ast_free(node->program.functions[i]);
            }
            if (node->program.functions) free(node->program.functions);
            break;
    }

    free(node);
}

static void print_indent(int indent) {
    for (int i = 0; i < indent; i++) printf("  ");
}

void ast_dump(const ASTNode *node, int indent) {
    if (!node) return;

    print_indent(indent);

    switch (node->type) {
        case AST_PROGRAM:
            printf("Program (%d functions):\n", node->program.function_count);
            for (int i = 0; i < node->program.function_count; i++) {
                ast_dump(node->program.functions[i], indent + 1);
            }
            break;

        case AST_FUNCTION:
            printf("Function '%s' (params: ", node->function.name);
            for (int i = 0; i < node->function.param_count; i++) {
                printf("%s%s", node->function.params[i], (i + 1 < node->function.param_count) ? ", " : "");
            }
            printf("):\n");
            for (int i = 0; i < node->function.body_count; i++) {
                ast_dump(node->function.body[i], indent + 1);
            }
            break;

        case AST_VAR_DECL:
            printf("VarDecl '%s' [offset x29 -%d]", node->var_decl.name, node->var_decl.offset);
            if (node->var_decl.init) {
                printf(" =\n");
                ast_dump(node->var_decl.init, indent + 1);
            } else {
                printf("\n");
            }
            break;

        case AST_ASSIGN:
            printf("Assign '%s' [offset x29 -%d] =\n", node->assign.name, node->assign.offset);
            ast_dump(node->assign.expr, indent + 1);
            break;

        case AST_VAR_REF:
            printf("VarRef '%s' [offset x29 -%d]\n", node->var_ref.name, node->var_ref.offset);
            break;

        case AST_NUM:
            printf("Num %d\n", node->num.value);
            break;

        case AST_BINARY_OP:
            printf("BinaryOp '%s':\n", token_type_to_string(node->binary.op));
            ast_dump(node->binary.left, indent + 1);
            ast_dump(node->binary.right, indent + 1);
            break;

        case AST_RETURN:
            printf("Return:\n");
            ast_dump(node->return_stmt.expr, indent + 1);
            break;

        case AST_IF:
            printf("If:\n");
            print_indent(indent + 1);
            printf("Condition:\n");
            ast_dump(node->if_stmt.condition, indent + 2);
            print_indent(indent + 1);
            printf("Then:\n");
            ast_dump(node->if_stmt.then_branch, indent + 2);
            if (node->if_stmt.else_branch) {
                print_indent(indent + 1);
                printf("Else:\n");
                ast_dump(node->if_stmt.else_branch, indent + 2);
            }
            break;

        case AST_WHILE:
            printf("While:\n");
            print_indent(indent + 1);
            printf("Condition:\n");
            ast_dump(node->while_stmt.condition, indent + 2);
            print_indent(indent + 1);
            printf("Body:\n");
            ast_dump(node->while_stmt.body, indent + 2);
            break;

        case AST_BLOCK:
            printf("Block (%d stmts):\n", node->block.stmt_count);
            for (int i = 0; i < node->block.stmt_count; i++) {
                ast_dump(node->block.stmts[i], indent + 1);
            }
            break;

        case AST_FUNC_CALL:
            printf("FuncCall '%s' (%d args):\n", node->func_call.name, node->func_call.arg_count);
            for (int i = 0; i < node->func_call.arg_count; i++) {
                ast_dump(node->func_call.args[i], indent + 1);
            }
            break;

        default:
            printf("Unknown AST Node\n");
            break;
    }
}
