#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include "casc.h"

void parser_eat(Lexer *lexer, TokenType type) {
    if (lexer_peek_token(lexer).type == type) {
        lexer_next_token(lexer);
    } else {
        fprintf(stderr, "ERROR: Expected %s got %s.\n", token_type_to_string(type), token_type_to_string(lexer_peek_token(lexer).type));
        assert(false);
    }
}

AST *parse_exp(Lexer *lexer) {
    Token token = lexer_peek_token(lexer);

    switch (token.type) {
        case TOKEN_NUMBER:
            parser_eat(lexer, TOKEN_NUMBER);
            if (token.contains_dot) {
                return init_ast_real(lexer->arena, strtod(token.text, NULL));
            } else {
                return init_ast_integer(lexer->arena, atoi(token.text));
            }
        case TOKEN_IDENTIFIER:
            if (is_builtin_function(token.text)) {
                parser_eat(lexer, TOKEN_IDENTIFIER);
                parser_eat(lexer, TOKEN_L_PAREN);

                ASTArray args = {0};
                ast_array_append(lexer->arena, &args, parse_expr(lexer));
                while (lexer_peek_token(lexer).type == TOKEN_COMMA) {
                    parser_eat(lexer, TOKEN_COMMA);
                    ast_array_append(lexer->arena, &args, parse_expr(lexer));
                }

                parser_eat(lexer, TOKEN_R_PAREN);
                return init_ast_call(lexer->arena, token.text, args);
            } else {
                parser_eat(lexer, TOKEN_IDENTIFIER);
                return init_ast_symbol(lexer->arena, token.text);
            }
        case TOKEN_L_PAREN:
            parser_eat(lexer, TOKEN_L_PAREN);
            AST* r = parse_expr(lexer);
            parser_eat(lexer, TOKEN_R_PAREN);
            return r;
        case TOKEN_MINUS:
            parser_eat(lexer, TOKEN_MINUS);
            return init_ast_unaryop(lexer->arena, parse_factor(lexer), OP_USUB);
        case TOKEN_PLUS:
            parser_eat(lexer, TOKEN_PLUS);
            return init_ast_unaryop(lexer->arena, parse_factor(lexer), OP_UADD);
        case TOKEN_EOF:
            // I think we can ignore EOF token for now and simply return it
            return init_ast_empty(lexer->arena);
        default:
            printf("%s\n", token_type_to_string(token.type));
            assert(false);
    }

}

AST *parse_factor(Lexer *lexer) {
    AST *result = parse_exp(lexer);

     while (lexer_peek_token(lexer).type == TOKEN_CARET) {
        parser_eat(lexer, TOKEN_CARET);
        // when we use the same hierachy pattern like in parse_term or parse_expr, we need to call
        // parse_expr here. But unlike with addition and multiplication we want to parse pow operation
        // from right to left (this is more common, e.g. desmos, python, ...). 
        // For now I dont know if there are any edge cases, where this implementation is wrong.
        result = init_ast_binop(lexer->arena, result, parse_factor(lexer), OP_POW);
    }

    return result;
}

AST *parse_term(Lexer *lexer) {
    AST* result = parse_factor(lexer);

    while (
        lexer_peek_token(lexer).type == TOKEN_STAR ||
        lexer_peek_token(lexer).type == TOKEN_SLASH ||
        lexer_peek_token(lexer).type == TOKEN_IDENTIFIER ||
        lexer_peek_token(lexer).type == TOKEN_L_PAREN
    ) {
        switch (lexer_peek_token(lexer).type) {
            case TOKEN_STAR:
                parser_eat(lexer, TOKEN_STAR);
                result = init_ast_binop(lexer->arena, result, parse_factor(lexer), OP_MUL);
                break;
            case TOKEN_SLASH:
                parser_eat(lexer, TOKEN_SLASH);
                result = init_ast_binop(lexer->arena, result, parse_factor(lexer), OP_DIV);
                break;
            case TOKEN_IDENTIFIER:
                result = init_ast_binop(lexer->arena, result, parse_factor(lexer), OP_MUL);
                break;
            case TOKEN_L_PAREN:
                result = init_ast_binop(lexer->arena, result, parse_exp(lexer), OP_MUL);
                break;
            default:
                assert(false);
        }
    }

    return result;
}

AST *parse_expr(Lexer* lexer) {
    AST* result = parse_term(lexer);

    while (lexer_peek_token(lexer).type == TOKEN_PLUS || lexer_peek_token(lexer).type == TOKEN_MINUS) {
        if (lexer_peek_token(lexer).type == TOKEN_PLUS) {
            parser_eat(lexer, TOKEN_PLUS);
            result = init_ast_binop(lexer->arena, result, parse_term(lexer), OP_ADD);
        } else if (lexer_peek_token(lexer).type == TOKEN_MINUS) {
            parser_eat(lexer, TOKEN_MINUS);
            result = init_ast_binop(lexer->arena, result, parse_term(lexer), OP_SUB);
        }
    }

    return result;
}

AST *parse(Lexer *lexer) {
    AST* result = parse_expr(lexer);
    assert(lexer_peek_token(lexer).type == TOKEN_EOF);
    return result;
}
