#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "lexer.h"
#include "parser.h"

void parser_expect(Parser* parser, TokenType type) {
    if (parser->tokens.data[parser->pos].type == type) {
        parser->pos++;
    } else {
        fprintf(stderr, "ERROR: Expected %s got %s.", token_type_to_string(type), token_type_to_string(parser->tokens.data[parser->pos].type));
    }
}   

AST* create_ast_integer(int64_t value) {
    AST* node = malloc(sizeof(AST));
    node->type = AST_INTEGER;
    node->integer = (ASTInteger){ .value=value };
    return node;
}

AST* create_ast_symbol(Token name) {
    AST *node = malloc(sizeof(AST));
    node->type = AST_SYMBOL;
    node->symbol = (ASTSymbol){ .name=name };
    return node;
}

AST* create_ast_constant(Token name) {
    AST *node = malloc(sizeof(AST));
    node->type = AST_CONSTANT;
    node->constant = (ASTConstant){ .name=name };
    return node;
}

AST* create_ast_binop(AST* left, AST* right, OpType type) {
    AST *node = malloc(sizeof(AST));
    node->type = AST_BINOP;
    node->binop = (ASTBinOp){ .left=left, .right=right, .type=type };
    return node;
}

AST* create_ast_unaryop(AST* expr, OpType type) {
    AST* node = malloc(sizeof(AST));
    node->type = AST_UNARYOP;
    node->unaryop = (ASTUnaryOp){ .expr=expr, .type=type };
    return node;
}

AST* create_ast_func_call(Token name, AST *arg) {
    AST *node = malloc(sizeof(AST));
    node->type = AST_FUNC_CALL;
    node->func_call = (ASTFuncCall){ .name=name, .arg=arg };
    return node;
}

AST* create_ast_empty() {
    AST* node = malloc(sizeof(AST));
    node->type = AST_EMPTY;
    node->empty = true;
    return node;
}

AST *create_ast_dummy(int flags) {
    AST* node = malloc(sizeof(AST));
    node->type = AST_DUMMY;
    node->dummy = flags;
    return node;
}

AST *parse_exp(Parser* parser) {
    Token current_token = parser->tokens.data[parser->pos];
    if (current_token.type == TOKEN_NUMBER) {
        parser_expect(parser, TOKEN_NUMBER);
        return create_ast_integer(atoi(current_token.text));
    } else if (current_token.type == TOKEN_IDENTIFIER) {
        if (
            !strcmp(current_token.text, "sqrt") ||
            !strcmp(current_token.text, "sin") ||
            !strcmp(current_token.text, "cos") ||
            !strcmp(current_token.text, "dummy")
        ) {
            parser_expect(parser, TOKEN_IDENTIFIER);
            parser_expect(parser, TOKEN_L_PAREN);
            AST* arg = parse_expr(parser);
            parser_expect(parser, TOKEN_R_PAREN);
            return create_ast_func_call(current_token, arg);
        } else if (!strcmp(current_token.text, "pi")) {
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

const char* op_type_to_debug_string(OpType type) {
    switch(type) {
        case OP_ADD: return "Add";
        case OP_SUB: return "Sub";
        case OP_MUL: return "Mul";
        case OP_DIV: return "Div";
        case OP_POW: return "Pow";
        case OP_UADD: return "UAdd";
        case OP_USUB: return "USub";
        default: assert(false);
    }
}

const char* op_type_to_string(OpType type) {
    switch (type) {
        case OP_ADD:
        case OP_UADD: return "+";
        case OP_SUB:
        case OP_USUB: return "-";
        case OP_MUL: return "*";
        case OP_DIV: return "/";
        case OP_POW: return "^";
        default: assert(false);
    }
}

const char* ast_type_to_debug_string(ASTType type) {
    switch (type) {
        case AST_INTEGER: return "Integer";
        case AST_SYMBOL: return "Symbol";
        case AST_CONSTANT: return "Constant";
        case AST_BINOP: return "BinOp";
        case AST_UNARYOP: return "UnaryOp";
        case AST_FUNC_CALL: return "FuncCall";
        case AST_DUMMY: return "Dummy";
        case AST_EMPTY: return "Empty";
        case AST_TYPE_COUNT: assert(false);
    }
}
