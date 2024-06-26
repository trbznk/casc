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

    AST *result = NULL;

    switch (token.type) {

        case TOKEN_NUMBER: {
            parser_eat(lexer, TOKEN_NUMBER);
            if (token.contains_dot) {
                result = init_ast_real(lexer->allocator, strtod(token.text.str, NULL));
            } else {
                result = init_ast_integer(lexer->allocator, atoi(token.text.str));
            }
            break;
        }

        case TOKEN_IDENTIFIER: {
            if (is_builtin_function(token.text)) {
                parser_eat(lexer, TOKEN_IDENTIFIER);
                parser_eat(lexer, TOKEN_L_PAREN);

                ASTArray args = {0};
                ast_array_append(lexer->allocator, &args, parse_expr(lexer));
                while (lexer_peek_token(lexer).type == TOKEN_COMMA) {
                    parser_eat(lexer, TOKEN_COMMA);
                    ast_array_append(lexer->allocator, &args, parse_expr(lexer));
                }

                parser_eat(lexer, TOKEN_R_PAREN);
                result = init_ast_call(lexer->allocator, token.text, args);
            } else {
                parser_eat(lexer, TOKEN_IDENTIFIER);
                result = init_ast_symbol(lexer->allocator, token.text);
            }
            break;
        }

        case TOKEN_L_PAREN: {
            parser_eat(lexer, TOKEN_L_PAREN);
            result = parse_expr(lexer);
            parser_eat(lexer, TOKEN_R_PAREN);
            break;
        }

        case TOKEN_MINUS: {
            parser_eat(lexer, TOKEN_MINUS);
            result = init_ast_unaryop(lexer->allocator, parse_factor(lexer), OP_USUB);
            break;
        }

        case TOKEN_PLUS: {
            parser_eat(lexer, TOKEN_PLUS);
            result = init_ast_unaryop(lexer->allocator, parse_factor(lexer), OP_UADD);
            break;
        }

        case TOKEN_EOF: {
            // I think we can ignore EOF token for now and simply return it
            result = init_ast_empty(lexer->allocator);
            break;
        }

        default: {
            printf("%s\n", token_type_to_string(token.type));
            assert(false);
        }

    }

    assert(result != NULL);

    // !
    if (lexer_peek_token(lexer).type == TOKEN_EXCLAMATION_MARK) {
        parser_eat(lexer, TOKEN_EXCLAMATION_MARK);

        ASTArray args = {0};
        ast_array_append(lexer->allocator, &args, result);
        result = init_ast_call(lexer->allocator, init_string("factorial"), args);
    }

    return result;
}

AST *parse_factor(Lexer *lexer) {
    AST *result = parse_exp(lexer);

     while (lexer_peek_token(lexer).type == TOKEN_CARET) {
        parser_eat(lexer, TOKEN_CARET);
        // when we use the same hierachy pattern like in parse_term or parse_expr, we need to call
        // parse_expr here. But unlike with addition and multiplication we want to parse pow operation
        // from right to left (this is more common, e.g. desmos, python, ...). 
        // For now I dont know if there are any edge cases, where this implementation is wrong.
        result = init_ast_binop(lexer->allocator, result, parse_factor(lexer), OP_POW);
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
                result = init_ast_binop(lexer->allocator, result, parse_factor(lexer), OP_MUL);
                break;
            case TOKEN_SLASH:
                parser_eat(lexer, TOKEN_SLASH);
                result = init_ast_binop(lexer->allocator, result, parse_factor(lexer), OP_DIV);
                break;
            case TOKEN_IDENTIFIER:
                result = init_ast_binop(lexer->allocator, result, parse_factor(lexer), OP_MUL);
                break;
            case TOKEN_L_PAREN:
                result = init_ast_binop(lexer->allocator, result, parse_exp(lexer), OP_MUL);
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
            result = init_ast_binop(lexer->allocator, result, parse_term(lexer), OP_ADD);
        } else if (lexer_peek_token(lexer).type == TOKEN_MINUS) {
            parser_eat(lexer, TOKEN_MINUS);
            result = init_ast_binop(lexer->allocator, result, parse_term(lexer), OP_SUB);
        }
    }

    return result;
}

AST *parse(Lexer *lexer) {
    AST* result = parse_expr(lexer);
    assert(lexer_peek_token(lexer).type == TOKEN_EOF);
    return result;
}
