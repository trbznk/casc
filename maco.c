#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <assert.h>

#define FIXED_STRING_SIZE 32

typedef enum {
    TOKEN_NUMBER,
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_STAR,
    TOKEN_SLASH,

    TOKEN_L_PAREN,
    TOKEN_R_PAREN,

    TOKEN_TYPE_COUNT
} TokenType;

typedef struct {
    TokenType type;
    char lexeme[FIXED_STRING_SIZE];
} Token;

typedef struct {
    size_t size;
    size_t capacity;
    Token *data;    
} Tokens;

const char* token_type_to_string(TokenType type) {
    static const char* names[] = {
        "NUMBER",
        "PLUS",
        "MINUS",
        "STAR",
        "SLASH",
        "L_PAREN",
        "R_PAREN"
    };
    _Static_assert(sizeof(names)/sizeof(names[0]) == TOKEN_TYPE_COUNT, "ERROR: Not enough names for token types.");

    assert(type < TOKEN_TYPE_COUNT);

    return names[type];
}

void tokens_push(Tokens *tokens, Token token) {
    const size_t alloc_capacity = 10;
    if (tokens->size >= tokens->capacity) {
        if (tokens->size == 0) {
            tokens->data = malloc(alloc_capacity*sizeof(Token));
        } else {
            tokens->data = realloc(tokens->data ,(tokens->size+alloc_capacity)*sizeof(Token));
        }
        tokens->capacity = tokens->capacity + alloc_capacity;
    }
    tokens->data[tokens->size] = token;
    tokens->size++;
}

int main() {
    char *source = "(42 + 6) - 3333 * (4 + 12)/9";
    printf("source: '%s'\n", source);

    // tokenize source
    Tokens tokens = {0};
    for (size_t i = 0; i < strlen(source); i++) {
        if (isdigit(source[i])) {
            char number_buffer[FIXED_STRING_SIZE] = "";
            while (isdigit(source[i])) {
                strncat(number_buffer, &source[i], 1);
                i++;
            }
            Token token;
            token.type = TOKEN_NUMBER;
            strcpy(token.lexeme, number_buffer);
            tokens_push(&tokens, token);
            i--;
        } else if (source[i] == '+') {
            Token token;
            token.type = TOKEN_PLUS;
            strncpy(token.lexeme, &source[i], 1);
            tokens_push(&tokens, token);
        } else if (source[i] == '-') {
            Token token;
            token.type = TOKEN_MINUS;
            strncpy(token.lexeme, &source[i], 1);
            tokens_push(&tokens, token);
        } else if (source[i] == '*') {
            Token token;
            token.type = TOKEN_STAR;
            strncpy(token.lexeme, &source[i], 1);
            tokens_push(&tokens, token);
        } else if (source[i] == '/') {
            Token token;
            token.type = TOKEN_SLASH;
            strncpy(token.lexeme, &source[i], 1);
            tokens_push(&tokens, token);
        } else if (source[i] == '(') {
            Token token;
            token.type = TOKEN_L_PAREN;
            strncpy(token.lexeme, &source[i], 1);
            tokens_push(&tokens, token);
        } else if (source[i] == ')') {
            Token token;
            token.type = TOKEN_R_PAREN;
            strncpy(token.lexeme, &source[i], 1);
            tokens_push(&tokens, token);
        } else if (isspace(source[i])) {
            // ignore spaces for now
        } else {
            fprintf(stderr, "ERROR: Can't tokenize '%c'\n", source[i]);
            exit(1);
        }
    }

    // print tokens
    for (size_t i = 0; i < tokens.size; i++) {
        Token *token = &tokens.data[i];
        printf("%s('%s') ", token_type_to_string(token->type), token->lexeme);
    }
    printf("\n");

    printf("tokens.size=%zu, tokens.capacity=%zu\n", tokens.size, tokens.capacity);
}
