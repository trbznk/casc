#ifndef LEXER_H
#define LEXER_H

#define FIXED_STRING_SIZE 32

typedef enum {
    TOKEN_NUMBER,
    TOKEN_IDENTIFIER,
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_STAR,
    TOKEN_SLASH,
    TOKEN_CARET,

    TOKEN_L_PAREN,
    TOKEN_R_PAREN,

    TOKEN_EOF,

    TOKEN_TYPE_COUNT
} TokenType;

static const char *token_type_string_table[] = {
    "NUMBER",
    "IDENTIFIER",
    "PLUS",
    "MINUS",
    "STAR",
    "SLASH",
    "CARET",
    "L_PAREN",
    "R_PAREN",
    "EOF"
};
_Static_assert(sizeof(token_type_string_table)/sizeof(token_type_string_table[0]) == TOKEN_TYPE_COUNT, "");

typedef struct {
    TokenType type;

    // TODO: I think we don't want to have fixed strings here in general
    char text[FIXED_STRING_SIZE];
} Token;

// TODO: transform this into a lexer. This is not only for naming, but to make it clearer
//       and differ it from the parser. Although we can implemente methods like peek_next_char
//       and similar things more easily.
typedef struct {
    size_t size;
    size_t capacity;
    Token *data;
} Tokens;

Tokens tokenize(char*);



#endif