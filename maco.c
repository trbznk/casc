#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <assert.h>

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

    TOKEN_TYPE_COUNT
} TokenType;

typedef struct {
    TokenType type;
    char lexeme[FIXED_STRING_SIZE];
} Token;

typedef struct {
    size_t size;
    size_t capacity;
    Token *tokens;    
} Lexer;

const char* token_type_to_string(TokenType type) {
    static const char* names[] = {
        "NUMBER",
        "IDENTIFIER",
        "PLUS",
        "MINUS",
        "STAR",
        "SLASH",
        "CARET",
        "L_PAREN",
        "R_PAREN"
    };
    _Static_assert(sizeof(names)/sizeof(names[0]) == TOKEN_TYPE_COUNT, "ERROR: Not enough names for token types.");

    assert(type < TOKEN_TYPE_COUNT);

    return names[type];
}

void lexer_push(Lexer *lexer, Token token) {
    const size_t alloc_capacity = 10;
    if (lexer->size >= lexer->capacity) {
        if (lexer->size == 0) {
            lexer->tokens = malloc(alloc_capacity*sizeof(Token));
        } else {
            lexer->tokens = realloc(lexer->tokens ,(lexer->size+alloc_capacity)*sizeof(Token));
        }
        lexer->capacity = lexer->capacity + alloc_capacity;
    }
    lexer->tokens[lexer->size] = token;
    lexer->size++;
}

Lexer lex(char* source) {
    Lexer lexer = {0};
    for (size_t i = 0; i < strlen(source); i++) {
        if (isdigit(source[i])) {
            char number_buffer[FIXED_STRING_SIZE] = "";
            while (isdigit(source[i])) {
                strncat(number_buffer, &source[i], 1);
                i++;
            }
            Token token = { .type = TOKEN_NUMBER };
            strcpy(token.lexeme, number_buffer);
            lexer_push(&lexer, token);
            i--;
        } else if (isalpha(source[i])) {
            char number_buffer[FIXED_STRING_SIZE] = "";
            size_t first = i;
            while (isalpha(source[i]) || (i > first && (isalpha(source[i]) || isdigit(source[i])))) {
                strncat(number_buffer, &source[i], 1);
                i++;
            }
            Token token = { .type = TOKEN_IDENTIFIER };
            strcpy(token.lexeme, number_buffer);
            lexer_push(&lexer, token);
            i--;
        } else if (source[i] == '+') {
            Token token = { .type = TOKEN_PLUS };
            strncpy(token.lexeme, &source[i], 1);
            lexer_push(&lexer, token);
        } else if (source[i] == '-') {
            Token token = { .type = TOKEN_MINUS };
            strncpy(token.lexeme, &source[i], 1);
            lexer_push(&lexer, token);
        } else if (source[i] == '*') {
            Token token = { .type = TOKEN_STAR };
            strncpy(token.lexeme, &source[i], 1);
            lexer_push(&lexer, token);
        } else if (source[i] == '/') {
            Token token = { .type = TOKEN_SLASH };
            strncpy(token.lexeme, &source[i], 1);
            lexer_push(&lexer, token);
        } else if (source[i] == '^') {
            Token token = { .type = TOKEN_CARET };
            strncpy(token.lexeme, &source[i], 1);
            lexer_push(&lexer, token);
        } else if (source[i] == '(') {
            Token token = { .type = TOKEN_L_PAREN };
            strncpy(token.lexeme, &source[i], 1);
            lexer_push(&lexer, token);
        } else if (source[i] == ')') {
            Token token = { .type = TOKEN_R_PAREN };
            strncpy(token.lexeme, &source[i], 1);
            lexer_push(&lexer, token);
        } else if (isspace(source[i])) {
            // ignore spaces for now
        } else {
            fprintf(stderr, "ERROR: Can't tokenize '%c'\n", source[i]);
            exit(1);
        }
    }
    return lexer;
}

int main() {
    char *source = "2x(42 + 6) - 3333 * (4 + 12 + x2)/9 + (x^2 - y^2)";
    printf("source: '%s'\n", source);
    Lexer lexer = lex(source);

    // print tokens
    for (size_t i = 0; i < lexer.size; i++) {
        Token *token = &lexer.tokens[i];
        printf("%s('%s') ", token_type_to_string(token->type), token->lexeme);
    }
    printf("\n");

    printf("lexer.size=%zu, lexer.capacity=%zu\n", lexer.size, lexer.capacity);
}
