#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include "casc.h"

void parser_expect(Parser* parser, TokenType type) {
    if (parser->tokens.data[parser->pos].type == type) {
        parser->pos++;
    } else {
        fprintf(stderr, "ERROR: Expected %s got %s.\n", token_type_to_string(type), token_type_to_string(parser->tokens.data[parser->pos].type));
    }
}

AST *parse_exp(Parser* parser) {
    Token current_token = parser->tokens.data[parser->pos];
    if (current_token.type == TOKEN_NUMBER) {
        parser_expect(parser, TOKEN_NUMBER);
        return create_ast_integer(atoi(current_token.text));
    } else if (current_token.type == TOKEN_IDENTIFIER) {
        if (is_builtin_function(current_token.text)) {
            parser_expect(parser, TOKEN_IDENTIFIER);
            parser_expect(parser, TOKEN_L_PAREN);

            ASTArray args = {0};
            ast_array_append(&args, parse_expr(parser));
            while (parser->tokens.data[parser->pos].type == TOKEN_COMMA) {
                parser_expect(parser, TOKEN_COMMA);
                ast_array_append(&args, parse_expr(parser));
            }

            parser_expect(parser, TOKEN_R_PAREN);
            return create_ast_func_call(current_token, args);
        } else if (is_builtin_constant(current_token.text)) {
            parser_expect(parser, TOKEN_IDENTIFIER);
            return create_ast_constant(current_token);
        } else {
            parser_expect(parser, TOKEN_IDENTIFIER);
            return create_ast_symbol(current_token);
        }
    } else if (current_token.type == TOKEN_L_PAREN) {
        parser_expect(parser, TOKEN_L_PAREN);
        AST* r = parse_expr(parser);
        parser_expect(parser, TOKEN_R_PAREN);
        return r;
    } else if (current_token.type == TOKEN_MINUS) {
        parser_expect(parser, TOKEN_MINUS);
        return create_ast_unaryop(parse_factor(parser), OP_USUB);
    } else if (current_token.type == TOKEN_PLUS) {
        parser_expect(parser, TOKEN_PLUS);
        return create_ast_unaryop(parse_factor(parser), OP_UADD);
    } else if (current_token.type == TOKEN_EOF) {
        // I think we can ignore EOF token for now and simply return it
        return create_ast_empty();
    } else {
        printf("%s\n", token_type_to_string(current_token.type));
        assert(false);
    }
}

AST *parse_factor(Parser *parser) {
    AST* result = parse_exp(parser);

     while (parser->tokens.data[parser->pos].type == TOKEN_CARET) {
        parser_expect(parser, TOKEN_CARET);
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
        parser->tokens.data[parser->pos].type == TOKEN_STAR ||
        parser->tokens.data[parser->pos].type == TOKEN_SLASH ||
        parser->tokens.data[parser->pos].type == TOKEN_IDENTIFIER ||
        parser->tokens.data[parser->pos].type == TOKEN_L_PAREN
    ) {
        if (parser->tokens.data[parser->pos].type == TOKEN_STAR) {
            parser_expect(parser, TOKEN_STAR);
            result = create_ast_binop(result, parse_factor(parser), OP_MUL);
        } else if (parser->tokens.data[parser->pos].type == TOKEN_SLASH) {
            parser_expect(parser, TOKEN_SLASH);
            result = create_ast_binop(result, parse_factor(parser), OP_DIV);
        } else if (parser->tokens.data[parser->pos].type == TOKEN_IDENTIFIER) {
            result = create_ast_binop(result, parse_factor(parser), OP_MUL);
        } else if (parser->tokens.data[parser->pos].type == TOKEN_L_PAREN) {
            result = create_ast_binop(result, parse_exp(parser), OP_MUL);
        }
    }

    return result;
}

AST* parse_expr(Parser* parser) {
    AST* result = parse_term(parser);

    assert(parser->pos < parser->tokens.size);
    while (parser->tokens.data[parser->pos].type == TOKEN_PLUS || parser->tokens.data[parser->pos].type == TOKEN_MINUS) {
        if (parser->tokens.data[parser->pos].type == TOKEN_PLUS) {
            parser_expect(parser, TOKEN_PLUS);
            result = create_ast_binop(result, parse_term(parser), OP_ADD);
        } else if (parser->tokens.data[parser->pos].type == TOKEN_MINUS) {
            parser_expect(parser, TOKEN_MINUS);
            result = create_ast_binop(result, parse_term(parser), OP_SUB);
        }
    }

    return result;
}

AST* parse(Parser *parser) {
    AST* result = parse_expr(parser);
    assert(parser->tokens.data[parser->pos].type == TOKEN_EOF);
    return result;
}

AST* parse_from_string(char* input) {
    Tokens tokens = tokenize(input);
    Parser parser = { .tokens = tokens, .pos=0 };
    AST* ast = parse(&parser);
    return ast;
}

