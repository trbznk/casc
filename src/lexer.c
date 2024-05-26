#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

#include "lexer.h"

const char *BUILTIN_FUNCTIONS[] = {
    "sqrt", "sin", "cos", "ln", "log"
};
const size_t BUILTIN_FUNCTIONS_COUNT = sizeof(BUILTIN_FUNCTIONS) / sizeof(BUILTIN_FUNCTIONS[0]);

const char *BUILTIN_CONSTANTS[] = {
    "pi", "e"
};
const size_t BUILTIN_CONSTANTS_COUNT = sizeof(BUILTIN_CONSTANTS) / sizeof(BUILTIN_CONSTANTS[0]);

bool is_builtin_function(char *s) {
    for (size_t i = 0; i < BUILTIN_FUNCTIONS_COUNT; i++) {
        if (!strcmp(s, BUILTIN_FUNCTIONS[i])) return true;
    }  
    return false;
}

bool is_builtin_constant(char *s) {
    for (size_t i = 0; i < BUILTIN_CONSTANTS_COUNT; i++) {
        if (!strcmp(s, BUILTIN_CONSTANTS[i])) return true;
    }  
    return false;
}

void tokens_print(Tokens *tokens) {
    for (size_t i = 0; i < tokens->size; i++) {
        Token *token = &tokens->data[i];
        printf("%s('%s') ", token_type_to_string(token->type), token->text);
    }
    printf("\n");
}

void tokens_append(Tokens *tokens, Token token) {
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

Tokens tokenize(char* source) {
    Tokens tokens = {0};
    size_t i = 0;
    while (i < strlen(source)) {
        if (isdigit(source[i])) {
            char number_buffer[FIXED_STRING_SIZE] = "";
            while (isdigit(source[i])) {
                strncat(number_buffer, &source[i], 1);
                i++;
            }
            Token token = { .type = TOKEN_NUMBER };
            strcpy(token.text, number_buffer);
            tokens_append(&tokens, token);
            i--;
        } else if (isalpha(source[i])) {
            // collect alphanumeric chars into the buffer
            char buffer[FIXED_STRING_SIZE] = "";
            size_t first = i;
            while (isalpha(source[i]) || (i > first && (isalpha(source[i]) || isdigit(source[i])))) {
                strncat(buffer, &source[i], 1);
                i++;
            }

            // when the buffer matches a keyword this will become one identifier token
            // if (!strcmp(buffer, "sqrt") || !strcmp(buffer, "sin") || !strcmp(buffer, "cos") || !strcmp(buffer, "pi")) {
            if (is_builtin_function(buffer) || is_builtin_constant(buffer)) {
                Token token = { .type = TOKEN_IDENTIFIER };
                strcpy(token.text, buffer);
                tokens_append(&tokens, token);
                i--;
            // when not, every char becomes a seperate identifier token
            } else {
                i = first;
                Token token = { .type = TOKEN_IDENTIFIER };
                strncpy(token.text, &source[i], 1);
                tokens_append(&tokens, token);
            }
        } else if (source[i] == '+') {
            // TODO: Remove char -> text copy.
            Token token = { .type = TOKEN_PLUS };
            strncpy(token.text, &source[i], 1);
            tokens_append(&tokens, token);
        } else if (source[i] == '-') {
            Token token = { .type = TOKEN_MINUS };
            strncpy(token.text, &source[i], 1);
            tokens_append(&tokens, token);
        } else if (source[i] == '*') {
            Token token = { .type = TOKEN_STAR };
            strncpy(token.text, &source[i], 1);
            tokens_append(&tokens, token);
        } else if (source[i] == '/') {
            Token token = { .type = TOKEN_SLASH };
            strncpy(token.text, &source[i], 1);
            tokens_append(&tokens, token);
        } else if (source[i] == '^') {
            Token token = { .type = TOKEN_CARET };
            strncpy(token.text, &source[i], 1);
            tokens_append(&tokens, token);
        } else if (source[i] == '(') {
            Token token = { .type = TOKEN_L_PAREN };
            strncpy(token.text, &source[i], 1);
            tokens_append(&tokens, token);
        } else if (source[i] == ')') {
            Token token = { .type = TOKEN_R_PAREN };
            strncpy(token.text, &source[i], 1);
            tokens_append(&tokens, token);
        } else if (isspace(source[i])) {
            // ignore spaces for now
        } else if (source[i] == '#') {
            Token token = { .type = TOKEN_HASH };
            strncpy(token.text, &source[i], 1);
            tokens_append(&tokens, token);
        } else if (source[i] == ',') {
            Token token = { .type = TOKEN_COMMA };
            strncpy(token.text, &source[i], 1);
            tokens_append(&tokens, token);
        } else {
            fprintf(stderr, "ERROR: Can't tokenize '%c'\n", source[i]);
            exit(1);
        }
        i++;
    }
    assert(i == strlen(source));
    tokens_append(&tokens, (Token){ .type=TOKEN_EOF });
    return tokens;
}

const char *token_type_to_string(TokenType type) {
    switch (type) {
        case TOKEN_NUMBER: return "NUMBER";
        case TOKEN_IDENTIFIER: return "IDENTIFIER";
        case TOKEN_PLUS: return "PLUS";
        case TOKEN_MINUS: return "MINUS";
        case TOKEN_STAR: return "STAR";
        case TOKEN_SLASH: return "SLASH";
        case TOKEN_CARET: return "CARET";
        case TOKEN_L_PAREN: return "L_PAREN";
        case TOKEN_R_PAREN: return "R_PAREN";
        case TOKEN_HASH: return "HASH";
        case TOKEN_COMMA: return "COMMA";
        case TOKEN_EOF: return "EOF";
        default: assert(false);
    }
}
