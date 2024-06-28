#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

#include "casc.h"

void lexer_print_tokens(Lexer *lexer) {
    Token token;
    do {
        token = lexer_next_token(lexer);
        printf("%s('%s') ", token_type_to_string(token.type), token.text.str);
    } while (token.type != TOKEN_EOF);
    printf("\n");
}

inline char lexer_current_char(Lexer *lexer) {
    return lexer->source.str[lexer->pos];
}

Token lexer_next_token(Lexer *lexer) {
    Token token = {0};
    assert(!token.contains_dot);

    if (lexer->pos == lexer->source.size) {
        token.type = TOKEN_EOF;
        return token;
    }

    if (isdigit(lexer_current_char(lexer))) {
        // lex number
        String buffer = init_string("");
        while (isdigit(lexer_current_char(lexer)) || lexer_current_char(lexer) == '.') {
            char current_char = lexer_current_char(lexer);
            buffer = string_concat(lexer->allocator, buffer, char_to_string(lexer->allocator, current_char));
            lexer->pos += 1;
        }
        token.type = TOKEN_NUMBER;

        u32 dots_count = 0;
        for (usize i = 0; i < buffer.size; i++) {
            if (buffer.str[i] == '.') {
                dots_count += 1;
                token.contains_dot = true;
            }
        }
        assert(dots_count < 2);
        assert(buffer.str[0] != '.');
        assert(buffer.str[buffer.size-1] != '.');

        token.text = buffer;
        return token;
    } else if (isalpha(lexer_current_char(lexer))) {
        // lex identifier
        String buffer = init_string("");
        usize first = lexer->pos;
        while (
            isalpha(lexer_current_char(lexer)) ||
            (lexer->pos > first && (isalpha(lexer_current_char(lexer)) ||
            isdigit(lexer_current_char(lexer))))
        ) {
            char current_char = lexer_current_char(lexer);
            buffer = string_concat(lexer->allocator, buffer, char_to_string(lexer->allocator, current_char));
            lexer->pos += 1;
        }

        token.type = TOKEN_IDENTIFIER;
        token.text = buffer;
        return token;
    } else if (lexer_current_char(lexer) == '+') {
        token.type = TOKEN_PLUS;
        token.text = char_to_string(lexer->allocator, lexer_current_char(lexer));
        lexer->pos += 1;
        return token;
    } else if (lexer_current_char(lexer) == '-') {
        token.type = TOKEN_MINUS;
        token.text = char_to_string(lexer->allocator, lexer_current_char(lexer));
        lexer->pos += 1;
        return token;
    } else if (lexer_current_char(lexer) == '*') {
        token.type = TOKEN_STAR;
        token.text = char_to_string(lexer->allocator, lexer_current_char(lexer));
        lexer->pos += 1;
        return token;
    } else if (lexer_current_char(lexer) == '/') {
        token.type = TOKEN_SLASH;
        token.text = char_to_string(lexer->allocator, lexer_current_char(lexer));
        lexer->pos += 1;
        return token;
    } else if (lexer_current_char(lexer) == '^') {
        token.type = TOKEN_CARET;
        token.text = char_to_string(lexer->allocator, lexer_current_char(lexer));
        lexer->pos += 1;
        return token;
    } else if (lexer_current_char(lexer) == '(') {
        token.type = TOKEN_L_PAREN;
        token.text = char_to_string(lexer->allocator, lexer_current_char(lexer));
        lexer->pos += 1;
        return token;
    } else if (lexer_current_char(lexer) == ')') {
        token.type = TOKEN_R_PAREN;
        token.text = char_to_string(lexer->allocator, lexer_current_char(lexer));
        lexer->pos += 1;
        return token;
    } else if (lexer_current_char(lexer) == ' ') {
        // we dont use the isspace() function here because its although true for \n char
        lexer->pos += 1;
        return lexer_next_token(lexer);
    } else if (lexer_current_char(lexer) == '\n') {
        token.type = TOKEN_NEW_LINE;
        token.text = char_to_string(lexer->allocator, lexer_current_char(lexer));
        lexer->pos += 1;

        while (isspace(lexer_current_char(lexer))) {
            lexer->pos += 1;
        }

        return token;
    } else if (lexer_current_char(lexer) == ',') {
        token.type = TOKEN_COMMA;
        token.text = char_to_string(lexer->allocator, lexer_current_char(lexer));
        lexer->pos += 1;
        return token;
    } else if (lexer_current_char(lexer) == '=') {
        token.type = TOKEN_EQUAL;
        token.text = char_to_string(lexer->allocator, lexer_current_char(lexer));
        lexer->pos += 1;
        return token;
    } else if (lexer_current_char(lexer) == '!') {
        token.type = TOKEN_EXCLAMATION_MARK;
        token.text = char_to_string(lexer->allocator, lexer_current_char(lexer));
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
        case TOKEN_NEW_LINE: return "NEW_LINE";
        case TOKEN_EOF: return "EOF";
        default:
            printf("%d\n", type); 
            assert(false);
    }
}
