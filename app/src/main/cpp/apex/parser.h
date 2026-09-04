#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include <stdbool.h>

typedef enum {
  AST_NUM,
  AST_BINARY_OP,
  AST_RETURN,
  AST_VAR_DECL,
  AST_ASSIGN,
  AST_VAR_REF,
  AST_IF,
  AST_WHILE,
  AST_BLOCK,
  AST_FUNC_CALL,
  AST_FUNCTION,
  AST_PROGRAM
} ASTNodeType;

typedef struct {
  char *name;
  int offset;
} Symbol;

typedef struct {
  Symbol *symbols;
  int count;
  int capacity;
  int total_stack_bytes;
} SymbolTable;

typedef struct ASTNode ASTNode;

struct ASTNode {
  ASTNodeType type;
  union {
    struct {
      int value;
    } num;

    struct {
      TokenType op; // TOKEN_PLUS, TOKEN_MINUS, TOKEN_STAR, TOKEN_SLASH,
                    // TOKEN_EQ, TOKEN_NE, etc.
      ASTNode *left;
      ASTNode *right;
    } binary;

    struct {
      ASTNode *expr;
    } return_stmt;

    struct {
      char *name;
      int offset;
      ASTNode *init; // Optional initializer expression
    } var_decl;

    struct {
      char *name;
      int offset;
      ASTNode *expr;
    } assign;

    struct {
      char *name;
      int offset;
    } var_ref;

    struct {
      ASTNode *condition;
      ASTNode *then_branch;
      ASTNode *else_branch;
    } if_stmt;

    struct {
      ASTNode *condition;
      ASTNode *body;
    } while_stmt;

    struct {
      ASTNode **stmts;
      int stmt_count;
    } block;

    struct {
      char *name;
      ASTNode **args;
      int arg_count;
    } func_call;

    struct {
      char *name;
      char **params;
      int param_count;
      ASTNode **body;
      int body_count;
      SymbolTable symbol_table;
    } function;

    struct {
      ASTNode **functions;
      int function_count;
    } program;
  };
};

typedef struct {
  Lexer lexer;
  Token current_token;
  bool has_error;
  char error_msg[256];
  SymbolTable *current_symtab;
} Parser;

Parser parser_init(const char *src);
ASTNode *parse_program(Parser *p);
void ast_free(ASTNode *node);
void ast_dump(const ASTNode *node, int indent);

#endif // PARSER_H
