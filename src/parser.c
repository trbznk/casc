#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include "casc.h"

void parser_eat(Worker *w, TokenType type) {
    if (lexer_peek_token(&w->lexer).type == type) {
        lexer_next_token(&w->lexer);
    } else {
        fprintf(stderr, "ERROR: Expected %s got %s.\n", token_type_to_string(type), token_type_to_string(lexer_peek_token(&w->lexer).type));
        assert(false);
    }
}

AST *parse_exp(Worker* w) {
    Token token = lexer_peek_token(&w->lexer);

    switch (token.type) {
        case TOKEN_NUMBER:
            parser_eat(w, TOKEN_NUMBER);
            return create_ast_integer(&w->arena, atoi(token.text));
        case TOKEN_IDENTIFIER:
            if (is_builtin_function(token.text)) {
                parser_eat(w, TOKEN_IDENTIFIER);
                parser_eat(w, TOKEN_L_PAREN);

                ASTArray args = {0};
                ast_array_append(&args, parse_expr(w));
                while (lexer_peek_token(&w->lexer).type == TOKEN_COMMA) {
                    parser_eat(w, TOKEN_COMMA);
                    ast_array_append(&args, parse_expr(w));
                }

                parser_eat(w, TOKEN_R_PAREN);
                return create_ast_func_call(&w->arena, token, args);
            } else {
                parser_eat(w, TOKEN_IDENTIFIER);
                return create_ast_symbol(&w->arena, token.text);
            }
        case TOKEN_L_PAREN:
            parser_eat(w, TOKEN_L_PAREN);
            AST* r = parse_expr(w);
            parser_eat(w, TOKEN_R_PAREN);
            return r;
        case TOKEN_MINUS:
            parser_eat(w, TOKEN_MINUS);
            return create_ast_unaryop(&w->arena, parse_factor(w), OP_USUB);
        case TOKEN_PLUS:
            parser_eat(w, TOKEN_PLUS);
            return create_ast_unaryop(&w->arena, parse_factor(w), OP_UADD);
        case TOKEN_EOF:
            // I think we can ignore EOF token for now and simply return it
            return create_ast_empty(&w->arena);
        default:
            printf("%s\n", token_type_to_string(token.type));
            assert(false);
    }

}

AST *parse_factor(Worker *w) {
    AST* result = parse_exp(w);

     while (lexer_peek_token(&w->lexer).type == TOKEN_CARET) {
        parser_eat(w, TOKEN_CARET);
        // when we use the same hierachy pattern like in parse_term or parse_expr, we need to call
        // parse_expr here. But unlike with addition and multiplication we want to parse pow operation
        // from right to left (this is more common, e.g. desmos, python, ...). 
        // For now I dont know if there are any edge cases, where this implementation is wrong.
        result = create_ast_binop(&w->arena, result, parse_factor(w), OP_POW);
    }

    return result;
}

AST* parse_term(Worker* w) {
    AST* result = parse_factor(w);

    while (
        lexer_peek_token(&w->lexer).type == TOKEN_STAR ||
        lexer_peek_token(&w->lexer).type == TOKEN_SLASH ||
        lexer_peek_token(&w->lexer).type == TOKEN_IDENTIFIER ||
        lexer_peek_token(&w->lexer).type == TOKEN_L_PAREN
    ) {
        switch (lexer_peek_token(&w->lexer).type) {
            case TOKEN_STAR:
                parser_eat(w, TOKEN_STAR);
                result = create_ast_binop(&w->arena, result, parse_factor(w), OP_MUL);
                break;
            case TOKEN_SLASH:
                parser_eat(w, TOKEN_SLASH);
                result = create_ast_binop(&w->arena, result, parse_factor(w), OP_DIV);
                break;
            case TOKEN_IDENTIFIER:
                result = create_ast_binop(&w->arena, result, parse_factor(w), OP_MUL);
                break;
            case TOKEN_L_PAREN:
                result = create_ast_binop(&w->arena, result, parse_exp(w), OP_MUL);
                break;
            default:
                assert(false);
        }
    }

    return result;
}

AST* parse_expr(Worker* w) {
    AST* result = parse_term(w);

    while (lexer_peek_token(&w->lexer).type == TOKEN_PLUS || lexer_peek_token(&w->lexer).type == TOKEN_MINUS) {
        if (lexer_peek_token(&w->lexer).type == TOKEN_PLUS) {
            parser_eat(w, TOKEN_PLUS);
            result = create_ast_binop(&w->arena, result, parse_term(w), OP_ADD);
        } else if (lexer_peek_token(&w->lexer).type == TOKEN_MINUS) {
            parser_eat(w, TOKEN_MINUS);
            result = create_ast_binop(&w->arena, result, parse_term(w), OP_SUB);
        }
    }

    return result;
}

AST* parse(Worker *w) {
    AST* result = parse_expr(w);
    assert(lexer_peek_token(&w->lexer).type == TOKEN_EOF);
    return result;
}

AST* parse_from_string(Worker* w, char* source) {
    w->lexer.source = source;
    AST* ast = parse(w);
    return ast;
}
