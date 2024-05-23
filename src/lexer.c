#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "lexer.h"

void tokens_print(Tokens *tokens) {
    for (size_t i = 0; i < tokens->size; i++) {
        Token *token = &tokens->data[i];
        printf("%s('%s') ", token_type_string_table[token->type], token->text);
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
            // maybe we need this code again when introducing identifier for function calls e.g. sin(2)
            char buffer[FIXED_STRING_SIZE] = "";
            size_t first = i;
            while (isalpha(source[i]) || (i > first && (isalpha(source[i]) || isdigit(source[i])))) {
                strncat(buffer, &source[i], 1);
                i++;
            }
            if (!strcmp(buffer, "sqrt") || !strcmp(buffer, "sin") || !strcmp(buffer, "cos") || !strcmp(buffer, "pi")) {
                Token token = { .type = TOKEN_IDENTIFIER };
                strcpy(token.text, buffer);
                tokens_append(&tokens, token);
                i--;
            } else {
                i = first;
                Token token = { .type = TOKEN_IDENTIFIER };
                strncpy(token.text, &source[i], 1);
                tokens_append(&tokens, token);
            }
        } else if (source[i] == '+') {
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