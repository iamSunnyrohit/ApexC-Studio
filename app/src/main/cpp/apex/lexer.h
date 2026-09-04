#ifndef LEXER_H
#define LEXER_H

#include <stddef.h>

typedef enum {
    TOKEN_EOF = 0,
    TOKEN_INT_LIT,     // Integer literal e.g. 10
    TOKEN_IDENT,       // Identifier e.g. main
    TOKEN_KW_INT,      // "int"
    TOKEN_KW_RETURN,   // "return"
    TOKEN_KW_IF,       // "if"
    TOKEN_KW_ELSE,     // "else"
    TOKEN_KW_WHILE,    // "while"
    TOKEN_PLUS,        // '+'
    TOKEN_MINUS,       // '-'
    TOKEN_STAR,        // '*'
    TOKEN_SLASH,       // '/'
    TOKEN_ASSIGN,      // '='
    TOKEN_EQ,          // "=="
    TOKEN_NE,          // "!="
    TOKEN_LT,          // "<"
    TOKEN_LE,          // "<="
    TOKEN_GT,          // ">"
    TOKEN_GE,          // ">="
    TOKEN_COMMA,       // ','
    TOKEN_LPAREN,      // '('
    TOKEN_RPAREN,      // ')'
    TOKEN_LBRACE,      // '{'
    TOKEN_RBRACE,      // '}'
    TOKEN_SEMI,        // ';'
    TOKEN_ERROR        // Lexing error
} TokenType;

typedef struct {
    TokenType type;
    int int_value;
    char *text;
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

#endif // LEXER_H
