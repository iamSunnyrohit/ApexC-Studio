#ifndef APEXC_PARSER_H
#define APEXC_PARSER_H

#include "lexer.h"

typedef enum {
    AST_PROGRAM,
    AST_FUNCTION,
    AST_BLOCK,
    AST_VAR_DECL,
    AST_ASSIGN,
    AST_RETURN,
    AST_IF,
    AST_WHILE,
    AST_BINARY_OP,
    AST_NUM,
    AST_STRING,
    AST_VAR_REF,
    AST_FUNC_CALL
} ASTNodeType;

typedef struct ASTNode ASTNode;

typedef struct {
    char name[64];
    int offset;
} Symbol;

typedef struct {
    Symbol symbols[64];
    int symbol_count;
    int total_stack_bytes;
} SymbolTable;

struct ASTNode {
    ASTNodeType type;
    union {
        // AST_PROGRAM
        struct {
            ASTNode **functions;
            int function_count;
        } program;

        // AST_FUNCTION
        struct {
            char name[64];
            int param_count;
            ASTNode **body;
            int body_count;
            SymbolTable symbol_table;
        } function;

        // AST_BLOCK
        struct {
            ASTNode **stmts;
            int stmt_count;
        } block;

        // AST_VAR_DECL
        struct {
            char name[64];
            int offset;
            ASTNode *init;
        } var_decl;

        // AST_ASSIGN
        struct {
            char name[64];
            int offset;
            ASTNode *expr;
        } assign;

        // AST_RETURN
        struct {
            ASTNode *expr;
        } return_stmt;

        // AST_IF
        struct {
            ASTNode *condition;
            ASTNode *then_branch;
            ASTNode *else_branch;
        } if_stmt;

        // AST_WHILE
        struct {
            ASTNode *condition;
            ASTNode *body;
        } while_stmt;

        // AST_BINARY_OP
        struct {
            TokenType op;
            ASTNode *left;
            ASTNode *right;
        } binary;

        // AST_NUM
        struct {
            int value;
        } num;

        // AST_STRING
        struct {
            char *value;
            int label_id;
        } str;

        // AST_VAR_REF
        struct {
            char name[64];
            int offset;
        } var_ref;

        // AST_FUNC_CALL
        struct {
            char name[64];
            ASTNode **args;
            int arg_count;
        } func_call;
    };
};

typedef struct {
    Lexer lexer;
    Token current_token;
    SymbolTable *current_symtab;
    int string_literal_count;
} Parser;

Parser parser_init(const char *src);
ASTNode *parse_program(Parser *p);
void ast_free(ASTNode *node);

#endif // APEXC_PARSER_H