#include "lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

Lexer lexer_init(const char *src) {
    Lexer l;
    l.src = src;
    l.pos = 0;
    l.len = src ? strlen(src) : 0;
    l.line = 1;
    l.col = 1;
    return l;
}

static char peek_char(const Lexer *l) {
    if (l->pos >= l->len) return '\0';
    return l->src[l->pos];
}

static char advance_char(Lexer *l) {
    if (l->pos >= l->len) return '\0';
    char c = l->src[l->pos++];
    if (c == '\n') {
        l->line++;
        l->col = 1;
    } else {
        l->col++;
    }
    return c;
}

static void skip_whitespace_and_comments(Lexer *l) {
    while (l->pos < l->len) {
        char c = peek_char(l);
        if (isspace((unsigned char)c)) {
            advance_char(l);
        } else if (c == '/' && l->pos + 1 < l->len && l->src[l->pos + 1] == '/') {
            // Line comment
            advance_char(l);
            advance_char(l);
            while (l->pos < l->len && peek_char(l) != '\n') {
                advance_char(l);
            }
        } else if (c == '/' && l->pos + 1 < l->len && l->src[l->pos + 1] == '*') {
            // Block comment
            advance_char(l);
            advance_char(l);
            while (l->pos < l->len) {
                if (peek_char(l) == '*' && l->pos + 1 < l->len && l->src[l->pos + 1] == '/') {
                    advance_char(l);
                    advance_char(l);
                    break;
                }
                advance_char(l);
            }
        } else {
            break;
        }
    }
}

Token lexer_next_token(Lexer *l) {
    skip_whitespace_and_comments(l);

    Token tok;
    tok.type = TOKEN_EOF;
    tok.int_value = 0;
    tok.text = NULL;
    tok.line = l->line;
    tok.col = l->col;

    if (l->pos >= l->len) {
        tok.type = TOKEN_EOF;
        tok.text = strdup("EOF");
        return tok;
    }

    char c = peek_char(l);

    // Number literal
    if (isdigit((unsigned char)c)) {
        size_t start = l->pos;
        int value = 0;
        while (l->pos < l->len && isdigit((unsigned char)peek_char(l))) {
            char d = advance_char(l);
            value = value * 10 + (d - '0');
        }
        size_t len = l->pos - start;
        tok.type = TOKEN_INT_LIT;
        tok.int_value = value;
        tok.text = malloc(len + 1);
        memcpy(tok.text, l->src + start, len);
        tok.text[len] = '\0';
        return tok;
    }

    // Identifiers & Keywords
    if (isalpha((unsigned char)c) || c == '_') {
        size_t start = l->pos;
        while (l->pos < l->len && (isalnum((unsigned char)peek_char(l)) || peek_char(l) == '_')) {
            advance_char(l);
        }
        size_t len = l->pos - start;
        tok.text = malloc(len + 1);
        memcpy(tok.text, l->src + start, len);
        tok.text[len] = '\0';

        if (strcmp(tok.text, "int") == 0) {
            tok.type = TOKEN_KW_INT;
        } else if (strcmp(tok.text, "return") == 0) {
            tok.type = TOKEN_KW_RETURN;
        } else if (strcmp(tok.text, "if") == 0) {
            tok.type = TOKEN_KW_IF;
        } else if (strcmp(tok.text, "else") == 0) {
            tok.type = TOKEN_KW_ELSE;
        } else if (strcmp(tok.text, "while") == 0) {
            tok.type = TOKEN_KW_WHILE;
        } else {
            tok.type = TOKEN_IDENT;
        }
        return tok;
    }

    // Two-character and single-character operators & punctuators
    if (c == '=') {
        advance_char(l);
        if (peek_char(l) == '=') {
            advance_char(l);
            tok.type = TOKEN_EQ;
            tok.text = strdup("==");
            return tok;
        }
        tok.type = TOKEN_ASSIGN;
        tok.text = strdup("=");
        return tok;
    }

    if (c == '!') {
        advance_char(l);
        if (peek_char(l) == '=') {
            advance_char(l);
            tok.type = TOKEN_NE;
            tok.text = strdup("!=");
            return tok;
        }
        tok.type = TOKEN_ERROR;
        tok.text = strdup("!");
        return tok;
    }

    if (c == '<') {
        advance_char(l);
        if (peek_char(l) == '=') {
            advance_char(l);
            tok.type = TOKEN_LE;
            tok.text = strdup("<=");
            return tok;
        }
        tok.type = TOKEN_LT;
        tok.text = strdup("<");
        return tok;
    }

    if (c == '>') {
        advance_char(l);
        if (peek_char(l) == '=') {
            advance_char(l);
            tok.type = TOKEN_GE;
            tok.text = strdup(">=");
            return tok;
        }
        tok.type = TOKEN_GT;
        tok.text = strdup(">");
        return tok;
    }

    // Single character operators & punctuators
    advance_char(l);
    tok.text = malloc(2);
    tok.text[0] = c;
    tok.text[1] = '\0';

    switch (c) {
        case '+': tok.type = TOKEN_PLUS; break;
        case '-': tok.type = TOKEN_MINUS; break;
        case '*': tok.type = TOKEN_STAR; break;
        case '/': tok.type = TOKEN_SLASH; break;
        case ',': tok.type = TOKEN_COMMA; break;
        case '(': tok.type = TOKEN_LPAREN; break;
        case ')': tok.type = TOKEN_RPAREN; break;
        case '{': tok.type = TOKEN_LBRACE; break;
        case '}': tok.type = TOKEN_RBRACE; break;
        case ';': tok.type = TOKEN_SEMI; break;
        default:
            tok.type = TOKEN_ERROR;
            break;
    }

    return tok;
}

Token lexer_peek_token(Lexer *l) {
    Lexer saved = *l;
    Token t = lexer_next_token(&saved);
    return t;
}

void token_free(Token *t) {
    if (t && t->text) {
        free(t->text);
        t->text = NULL;
    }
}

const char *token_type_to_string(TokenType type) {
    switch (type) {
        case TOKEN_EOF: return "EOF";
        case TOKEN_INT_LIT: return "INT_LIT";
        case TOKEN_IDENT: return "IDENT";
        case TOKEN_KW_INT: return "int";
        case TOKEN_KW_RETURN: return "return";
        case TOKEN_KW_IF: return "if";
        case TOKEN_KW_ELSE: return "else";
        case TOKEN_KW_WHILE: return "while";
        case TOKEN_PLUS: return "+";
        case TOKEN_MINUS: return "-";
        case TOKEN_STAR: return "*";
        case TOKEN_SLASH: return "/";
        case TOKEN_ASSIGN: return "=";
        case TOKEN_EQ: return "==";
        case TOKEN_NE: return "!=";
        case TOKEN_LT: return "<";
        case TOKEN_LE: return "<=";
        case TOKEN_GT: return ">";
        case TOKEN_GE: return ">=";
        case TOKEN_COMMA: return ",";
        case TOKEN_LPAREN: return "(";
        case TOKEN_RPAREN: return ")";
        case TOKEN_LBRACE: return "{";
        case TOKEN_RBRACE: return "}";
        case TOKEN_SEMI: return ";";
        case TOKEN_ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}
