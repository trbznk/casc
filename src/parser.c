#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include "casc.h"

void parser_eat(Parser* parser, TokenType type) {
    if (lexer_peek_token(&parser->lexer).type == type) {
        lexer_next_token(&parser->lexer);
    } else {
        fprintf(stderr, "ERROR: Expected %s got %s.\n", token_type_to_string(type), token_type_to_string(lexer_peek_token(&parser->lexer).type));
        assert(false);
    }
}

AST *parse_exp(Parser* parser) {
    Token token = lexer_peek_token(&parser->lexer);

    switch (token.type) {
        case TOKEN_NUMBER:
            parser_eat(parser, TOKEN_NUMBER);
            return create_ast_integer(atoi(token.text));
        case TOKEN_IDENTIFIER:
            if (is_builtin_function(token.text)) {
                parser_eat(parser, TOKEN_IDENTIFIER);
                parser_eat(parser, TOKEN_L_PAREN);

                ASTArray args = {0};
                ast_array_append(&args, parse_expr(parser));
                while (lexer_peek_token(&parser->lexer).type == TOKEN_COMMA) {
                    parser_eat(parser, TOKEN_COMMA);
                    ast_array_append(&args, parse_expr(parser));
                }

                parser_eat(parser, TOKEN_R_PAREN);
                return create_ast_func_call(token, args);
            } else if (is_builtin_constant(token.text)) {
                parser_eat(parser, TOKEN_IDENTIFIER);
                return create_ast_constant(token);
            } else {
                parser_eat(parser, TOKEN_IDENTIFIER);
                return create_ast_symbol(token);
            }
        case TOKEN_L_PAREN:
            parser_eat(parser, TOKEN_L_PAREN);
            AST* r = parse_expr(parser);
            parser_eat(parser, TOKEN_R_PAREN);
            return r;
        case TOKEN_MINUS:
            parser_eat(parser, TOKEN_MINUS);
            return create_ast_unaryop(parse_factor(parser), OP_USUB);
        case TOKEN_PLUS:
            parser_eat(parser, TOKEN_PLUS);
            return create_ast_unaryop(parse_factor(parser), OP_UADD);
        case TOKEN_EOF:
            // I think we can ignore EOF token for now and simply return it
            return create_ast_empty();
        default:
            printf("%s\n", token_type_to_string(token.type));
            assert(false);
    }

}

AST *parse_factor(Parser *parser) {
    AST* result = parse_exp(parser);

     while (lexer_peek_token(&parser->lexer).type == TOKEN_CARET) {
        parser_eat(parser, TOKEN_CARET);
        // when we use the same hierachy pattern like in parse_term or parse_expr, we need to call
        // parse_expr here. But unlike with addition and multiplication we want to parse pow operation
        // from right to left (this is more common, e.g. desmos, python, ...). 
        // For now I dont know if there are any edge cases, where this implementation is wrong.
        result = create_ast_binop(result, parse_factor(parser), OP_POW);
    }

    return result;
}

AST* parse_term(Parser* parser) {
    AST* result = parse_factor(parser);

    while (
        lexer_peek_token(&parser->lexer).type == TOKEN_STAR ||
        lexer_peek_token(&parser->lexer).type == TOKEN_SLASH ||
        lexer_peek_token(&parser->lexer).type == TOKEN_IDENTIFIER ||
        lexer_peek_token(&parser->lexer).type == TOKEN_L_PAREN
    ) {
        switch (lexer_peek_token(&parser->lexer).type) {
            case TOKEN_STAR:
                parser_eat(parser, TOKEN_STAR);
                result = create_ast_binop(result, parse_factor(parser), OP_MUL);
                break;
            case TOKEN_SLASH:
                parser_eat(parser, TOKEN_SLASH);
                result = create_ast_binop(result, parse_factor(parser), OP_DIV);
                break;
            case TOKEN_IDENTIFIER:
                result = create_ast_binop(result, parse_factor(parser), OP_MUL);
                break;
            case TOKEN_L_PAREN:
                result = create_ast_binop(result, parse_exp(parser), OP_MUL);
                break;
            default:
                assert(false);
        }
    }

    return result;
}

AST* parse_expr(Parser* parser) {
    AST* result = parse_term(parser);

    while (lexer_peek_token(&parser->lexer).type == TOKEN_PLUS || lexer_peek_token(&parser->lexer).type == TOKEN_MINUS) {
        if (lexer_peek_token(&parser->lexer).type == TOKEN_PLUS) {
            parser_eat(parser, TOKEN_PLUS);
            result = create_ast_binop(result, parse_term(parser), OP_ADD);
        } else if (lexer_peek_token(&parser->lexer).type == TOKEN_MINUS) {
            parser_eat(parser, TOKEN_MINUS);
            result = create_ast_binop(result, parse_term(parser), OP_SUB);
        }
    }

    return result;
}

AST* parse(Parser *parser) {
    AST* result = parse_expr(parser);
    assert(lexer_peek_token(&parser->lexer).type == TOKEN_EOF);
    return result;
}

AST* parse_from_string(char* source) {
    Lexer lexer = { .source=source, .pos=0 };

    Parser parser;
    parser.lexer = lexer;

    AST* ast = parse(&parser);
    return ast;
}
