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

AST *parse_exp(Worker* worker) {
    Token token = lexer_peek_token(&worker->lexer);

    switch (token.type) {
        case TOKEN_NUMBER:
            parser_eat(worker, TOKEN_NUMBER);
            if (token.contains_dot) {
                return create_ast_real(strtod(token.text, NULL));
            } else {
                return create_ast_integer(atoi(token.text));
            }
        case TOKEN_IDENTIFIER:
            if (is_builtin_function(token.text)) {
                parser_eat(worker, TOKEN_IDENTIFIER);
                parser_eat(worker, TOKEN_L_PAREN);

                ASTArray args = {0};
                ast_array_append(&worker->arena, &args, parse_expr(worker));
                while (lexer_peek_token(&worker->lexer).type == TOKEN_COMMA) {
                    parser_eat(worker, TOKEN_COMMA);
                    ast_array_append(&worker->arena, &args, parse_expr(worker));
                }

                parser_eat(worker, TOKEN_R_PAREN);
                return create_ast_call(&worker->arena, token.text, args);
            } else {
                parser_eat(worker, TOKEN_IDENTIFIER);
                return _create_ast_symbol(&worker->arena, token.text);
            }
        case TOKEN_L_PAREN:
            parser_eat(worker, TOKEN_L_PAREN);
            AST* r = parse_expr(worker);
            parser_eat(worker, TOKEN_R_PAREN);
            return r;
        case TOKEN_MINUS:
            parser_eat(worker, TOKEN_MINUS);
            return create_ast_unaryop(&worker->arena, parse_factor(worker), OP_USUB);
        case TOKEN_PLUS:
            parser_eat(worker, TOKEN_PLUS);
            return create_ast_unaryop(&worker->arena, parse_factor(worker), OP_UADD);
        case TOKEN_EOF:
            // I think we can ignore EOF token for now and simply return it
            return create_ast_empty(&worker->arena);
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
        result = _create_ast_binop(&w->arena, result, parse_factor(w), OP_POW);
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
                result = _create_ast_binop(&w->arena, result, parse_factor(w), OP_MUL);
                break;
            case TOKEN_SLASH:
                parser_eat(w, TOKEN_SLASH);
                result = _create_ast_binop(&w->arena, result, parse_factor(w), OP_DIV);
                break;
            case TOKEN_IDENTIFIER:
                result = _create_ast_binop(&w->arena, result, parse_factor(w), OP_MUL);
                break;
            case TOKEN_L_PAREN:
                result = _create_ast_binop(&w->arena, result, parse_exp(w), OP_MUL);
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
            result = _create_ast_binop(&w->arena, result, parse_term(w), OP_ADD);
        } else if (lexer_peek_token(&w->lexer).type == TOKEN_MINUS) {
            parser_eat(w, TOKEN_MINUS);
            result = _create_ast_binop(&w->arena, result, parse_term(w), OP_SUB);
        }
    }

    return result;
}

AST* parse(Worker *w) {
    AST* result = parse_expr(w);
    assert(lexer_peek_token(&w->lexer).type == TOKEN_EOF);
    return result;
}

AST* parse_from_string(Worker *worker, char *source) {
    worker->lexer.source = source;
    AST* ast = parse(worker);
    return ast;
}
