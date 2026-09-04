#ifndef APEXC_LEXER_H
#define APEXC_LEXER_H

#include <stddef.h>

typedef enum {
    TOKEN_EOF,
    TOKEN_INT_LIT,
    TOKEN_STR_LIT,      // String literal: "..."
    TOKEN_IDENT,
    TOKEN_KW_INT,
    TOKEN_KW_RETURN,
    TOKEN_KW_IF,
    TOKEN_KW_ELSE,
    TOKEN_KW_WHILE,
    TOKEN_PLUS,         // +
    TOKEN_MINUS,        // -
    TOKEN_STAR,         // *
    TOKEN_SLASH,        // /
    TOKEN_ASSIGN,       // =
    TOKEN_EQ,           // ==
    TOKEN_NE,           // !=
    TOKEN_LT,           // <
    TOKEN_LE,           // <=
    TOKEN_GT,           // >
    TOKEN_GE,           // >=
    TOKEN_COMMA,        // ,
    TOKEN_LPAREN,       // (
    TOKEN_RPAREN,       // )
    TOKEN_LBRACE,       // {
    TOKEN_RBRACE,       // }
    TOKEN_SEMI,         // ;
    TOKEN_ERROR
} TokenType;

typedef struct {
    TokenType type;
    int int_value;
    char *text;         // Identifier name, raw symbol, or decoded string literal
    int line;
    int col;
} Token;

typedef struct {
    const char *src;
    size_t pos;
    size_t len;
    int line;
    int col;
} Lexer;

Lexer lexer_init(const char *src);
Token lexer_next_token(Lexer *l);
Token lexer_peek_token(Lexer *l);
void token_free(Token *t);
const char *token_type_to_string(TokenType type);

#endif // APEXC_LEXER_H