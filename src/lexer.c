#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

#include "casc.h"

const char *BUILTIN_FUNCTIONS[] = {
    "sqrt", "sin", "cos", "ln", "exp", "log", "diff"
};
const usize BUILTIN_FUNCTIONS_COUNT = sizeof(BUILTIN_FUNCTIONS) / sizeof(BUILTIN_FUNCTIONS[0]);

const char *BUILTIN_CONSTANTS[] = {
    "pi", "e"
};
const usize BUILTIN_CONSTANTS_COUNT = sizeof(BUILTIN_CONSTANTS) / sizeof(BUILTIN_CONSTANTS[0]);

bool is_builtin_function(char *s) {
    for (usize i = 0; i < BUILTIN_FUNCTIONS_COUNT; i++) {
        if (!strcmp(s, BUILTIN_FUNCTIONS[i])) return true;
    }  
    return false;
}

bool is_builtin_constant(char *s) {
    for (usize i = 0; i < BUILTIN_CONSTANTS_COUNT; i++) {
        if (!strcmp(s, BUILTIN_CONSTANTS[i])) return true;
    }  
    return false;
}

void lexer_print_tokens(Lexer *lexer) {
    Token token;
    do {
        token = lexer_next_token(lexer);
        printf("%s('%s') ", token_type_to_string(token.type), token.text);
    } while (token.type != TOKEN_EOF);
    printf("\n");
}

void print_tokens_from_string(char* source) {
    Lexer lexer = {0};
    lexer.source = source;
    lexer_print_tokens(&lexer);
}

inline char lexer_current_char(Lexer *lexer) {
    return lexer->source[lexer->pos];
}

Token lexer_next_token(Lexer *lexer) {
    Token token = {0};
    assert(!token.contains_dot);

    if (lexer->pos == strlen(lexer->source)) {
        token.type = TOKEN_EOF;
        return token;
    }

    if (isdigit(lexer_current_char(lexer))) {
        char buffer[64] = "";
        while (isdigit(lexer_current_char(lexer)) || lexer_current_char(lexer) == '.') {
            char current_char = lexer_current_char(lexer);
            strncat(buffer, &current_char, 1);
            lexer->pos += 1;
        }
        token.type = TOKEN_NUMBER;

        u32 dots_count = 0;
        for (usize i = 0; i < strlen(buffer); i++) {
            if (buffer[i] == '.') {
                dots_count += 1;
                token.contains_dot = true;
            }
        }
        assert(dots_count < 2);
        assert(buffer[0] != '.');
        assert(buffer[strlen(buffer)-1] != '.');

        strcpy(token.text, buffer);
        return token;
    } else if (isalpha(lexer_current_char(lexer))) {
        // collect alphanumeric chars into the buffer
        char buffer[64] = "";
        usize first = lexer->pos;
        while (
            isalpha(lexer_current_char(lexer)) ||
            (lexer->pos > first && (isalpha(lexer_current_char(lexer)) ||
            isdigit(lexer_current_char(lexer))))
        ) {
            char current_char = lexer_current_char(lexer);
            strncat(buffer, &current_char, 1);
            lexer->pos += 1;
        }

        if (is_builtin_function(buffer) || is_builtin_constant(buffer)) {
            token.type = TOKEN_IDENTIFIER;
            strcpy(token.text, buffer);
            return token;
        // when not, every char becomes a seperate identifier token
        } else {
            lexer->pos = first;
            token.type = TOKEN_IDENTIFIER;
            char current_char = lexer_current_char(lexer);
            strncpy(token.text, &current_char, 1);

            lexer->pos += 1;
            return token;
        }
    } else if (lexer_current_char(lexer) == '+') {
        token.type = TOKEN_PLUS;
        token.text[0] = lexer_current_char(lexer);
        lexer->pos += 1;
        return token;
    } else if (lexer_current_char(lexer) == '-') {
        token.type = TOKEN_MINUS;
        token.text[0] = lexer_current_char(lexer);
        lexer->pos += 1;
        return token;
    } else if (lexer_current_char(lexer) == '*') {
        token.type = TOKEN_STAR;
        token.text[0] = lexer_current_char(lexer);
        lexer->pos += 1;
        return token;
    } else if (lexer_current_char(lexer) == '/') {
        token.type = TOKEN_SLASH;
        token.text[0] = lexer_current_char(lexer);
        lexer->pos += 1;
        return token;
    } else if (lexer_current_char(lexer) == '^') {
        token.type = TOKEN_CARET;
        token.text[0] = lexer_current_char(lexer);
        lexer->pos += 1;
        return token;
    } else if (lexer_current_char(lexer) == '(') {
        token.type = TOKEN_L_PAREN;
        token.text[0] = lexer_current_char(lexer);
        lexer->pos += 1;
        return token;
    } else if (lexer_current_char(lexer) == ')') {
        token.type = TOKEN_R_PAREN;
        token.text[0] = lexer_current_char(lexer);
        lexer->pos += 1;
        return token;
    } else if (isspace(lexer_current_char(lexer))) {
        // ignore spaces for now
        lexer->pos += 1;
        return lexer_next_token(lexer);
    } else if (lexer_current_char(lexer) == '#') {
        token.type = TOKEN_HASH;
        token.text[0] = lexer_current_char(lexer);
        lexer->pos += 1;
        return token;
    } else if (lexer_current_char(lexer) == ',') {
        token.type = TOKEN_COMMA;
        token.text[0] = lexer_current_char(lexer);
        lexer->pos += 1;
        return token;
    } else {
        fprintf(stderr, "ERROR: Can't tokenize '%c'\n", lexer_current_char(lexer));
        exit(1);
    }

    assert(false); // unreachable
    token.type = TOKEN_EOF;
    return token;
}

Token lexer_peek_token(Lexer *lexer) {
    usize old_pos = lexer->pos;
    Token token = lexer_next_token(lexer);
    lexer->pos = old_pos;
    return token;
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
        default:
            printf("%d\n", type); 
            assert(false);
    }
}
