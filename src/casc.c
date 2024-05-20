#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#include <math.h>

#include "casc.h"

#define FIXED_STRING_SIZE 32

// forward declarations
typedef struct Node Node;
typedef struct Parser Parser;
Node* parse_expr(Parser*);
Node* interp(Node*);
Node* interp_binop(Node*);

typedef enum {
    TOKEN_NUMBER,
    TOKEN_IDENTIFIER,
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_STAR,
    TOKEN_SLASH,
    TOKEN_CARET,

    TOKEN_L_PAREN,
    TOKEN_R_PAREN,

    TOKEN_EOF,

    TOKEN_TYPE_COUNT
} TokenType;

static const char *token_type_string_table[] = {
    "NUMBER",
    "IDENTIFIER",
    "PLUS",
    "MINUS",
    "STAR",
    "SLASH",
    "CARET",
    "L_PAREN",
    "R_PAREN",
    "EOF"
};
_Static_assert(sizeof(token_type_string_table)/sizeof(token_type_string_table[0]) == TOKEN_TYPE_COUNT, "");

typedef struct {
    TokenType type;

    // TODO: I think we don't want to have fixed strings here in general
    char text[FIXED_STRING_SIZE];
} Token;

typedef struct {
    size_t size;
    size_t capacity;
    Token *data;
} Tokens;

typedef enum {
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,

    OP_UADD,
    OP_USUB,

    OP_TYPE_COUNT
} OpType;

static const char *op_type_string_table[] = {
    "Add",
    "Sub",
    "Mul",
    "Div",
    "UAdd",
    "USub"
};
_Static_assert(sizeof(op_type_string_table)/sizeof(op_type_string_table[0]) == OP_TYPE_COUNT, "");

static const char op_type_char_table[] = {
    '+',
    '-',
    '*',
    '/',
    '+',
    '-'
};
_Static_assert(sizeof(op_type_char_table)/sizeof(op_type_char_table[0]) == OP_TYPE_COUNT, "");

typedef enum {
    NODE_INTEGER,
    NODE_SYMBOL,
    NODE_BINOP,
    NODE_UNARYOP,
    NODE_FUNC,

    NODE_TYPE_COUNT
} NodeType;

static const char *node_type_string_table[] = {
    "Integer",
    "Symbol",
    "BinOp",
    "UnaryOp",
    "Func"
};
_Static_assert(sizeof(node_type_string_table)/sizeof(node_type_string_table[0]) == NODE_TYPE_COUNT, "");

typedef enum {
    FUNC_SQRT,

    FUNC_TYPE_COUNT
} FuncType;

static const char *func_type_string_table[] = {
    "sqrt"
};
_Static_assert(sizeof(func_type_string_table)/sizeof(func_type_string_table[0]) == FUNC_TYPE_COUNT, "");

struct Node {
    Node* left;
    Node* right;
    Node* expr;

    char name;
    int value_i32;

    NodeType type;
    OpType op_type;
    FuncType func_type;
};

struct Parser {
    Tokens tokens;
    size_t pos;
};

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
            char number_buffer[FIXED_STRING_SIZE] = "";
            size_t first = i;
            while (isalpha(source[i]) || (i > first && (isalpha(source[i]) || isdigit(source[i])))) {
                strncat(number_buffer, &source[i], 1);
                i++;
            }
            if (!strcmp(number_buffer, "sqrt")) {
                Token token = { .type = TOKEN_IDENTIFIER };
                strcpy(token.text, number_buffer);
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

void parser_expect(Parser* parser, TokenType type) {
    if (parser->tokens.data[parser->pos].type == type) {
        parser->pos++;
    } else {
        fprintf(stderr, "ERROR: Expected %s got %s.", token_type_string_table[type], token_type_string_table[parser->tokens.data[parser->pos].type]);
    }
}   

Node* ast_integer(int value) {
    Node* node = malloc(sizeof(Node));
    node->type = NODE_INTEGER;
    node->value_i32 = value;
    return node;
}

Node* ast_symbol(char name) {
    Node* node = malloc(sizeof(Node));
    node->type = NODE_SYMBOL;
    node->name = name;
    return node;
}

Node* ast_binop(Node* left, Node* right, OpType op_type) {
    Node* node = malloc(sizeof(Node));
    node->type = NODE_BINOP;
    node->left = left;
    node->right = right;
    node->op_type = op_type;
    return node;
}

Node* ast_unaryop(Node* expr, OpType op_type) {
    Node* node = malloc(sizeof(Node));
    node->type = NODE_UNARYOP;
    node->expr = expr;
    node->op_type = op_type;
    return node;
}

Node* ast_func(FuncType func_type, Node* expr) {
    Node* node = malloc(sizeof(Node));
    node->type = NODE_FUNC;
    node->expr = expr;
    node->func_type = func_type;
    return node;
}

bool node_eq(Node* left, Node* right) {
    if (left->type == right->type) {
        switch (left->type) {
            case NODE_INTEGER:
                return left->value_i32 == right->value_i32;
            case NODE_SYMBOL:
                return left->name == right->name;
            case NODE_BINOP:
                return node_eq(left->left, right->left) && node_eq(left->right, right->right);
            case NODE_FUNC:
                return left->func_type == right->func_type && node_eq(left->expr, right->expr);
            default: fprintf(stderr, "ERROR: Cannot do 'node_eq' because node type '%s' is not implemented.\n", node_type_string_table[left->type]); exit(1);
        }
    }
    return false;
}

Node* parse_factor(Parser* parser) {
    Token current_token = parser->tokens.data[parser->pos];
    if (current_token.type == TOKEN_NUMBER) {
        parser_expect(parser, TOKEN_NUMBER);
        return ast_integer(atoi(current_token.text));
    } else if (current_token.type == TOKEN_IDENTIFIER) {
        if (!strcmp(current_token.text, "sqrt")) {
            parser_expect(parser, TOKEN_IDENTIFIER);
            parser_expect(parser, TOKEN_L_PAREN);
            Node* body = parse_expr(parser);
            parser_expect(parser, TOKEN_R_PAREN);
            return ast_func(FUNC_SQRT, body);
        } else {
            parser_expect(parser, TOKEN_IDENTIFIER);
            return ast_symbol(current_token.text[0]);
        }
    } else if (current_token.type == TOKEN_L_PAREN) {
        parser_expect(parser, TOKEN_L_PAREN);
        Node* r = parse_expr(parser);
        parser_expect(parser, TOKEN_R_PAREN);
        return r;
    } else if (current_token.type == TOKEN_MINUS) {
        parser_expect(parser, TOKEN_MINUS);
        return ast_unaryop(parse_factor(parser), OP_USUB);
    } else if (current_token.type == TOKEN_PLUS) {
        parser_expect(parser, TOKEN_PLUS);
        return ast_unaryop(parse_factor(parser), OP_UADD);
    } else {
        assert(false);
    }
}

Node* parse_term(Parser* parser) {
    Node* result = parse_factor(parser);

    while (parser->tokens.data[parser->pos].type == TOKEN_STAR || parser->tokens.data[parser->pos].type == TOKEN_SLASH || parser->tokens.data[parser->pos].type == TOKEN_IDENTIFIER) {
        if (parser->tokens.data[parser->pos].type == TOKEN_STAR) {
            parser_expect(parser, TOKEN_STAR);
            result = ast_binop(result, parse_factor(parser), OP_MUL);
        } else if (parser->tokens.data[parser->pos].type == TOKEN_SLASH) {
            parser_expect(parser, TOKEN_SLASH);
            result = ast_binop(result, parse_factor(parser), OP_DIV);
        } else if (parser->tokens.data[parser->pos].type == TOKEN_IDENTIFIER) {
            result = ast_binop(result, parse_factor(parser), OP_MUL);
        }
    }
    return result;
}

Node* parse_expr(Parser* parser) {
    Node* result = parse_term(parser);

    assert(parser->pos < parser->tokens.size);
    while (parser->tokens.data[parser->pos].type == TOKEN_PLUS || parser->tokens.data[parser->pos].type == TOKEN_MINUS) {
        if (parser->tokens.data[parser->pos].type == TOKEN_PLUS) {
            parser_expect(parser, TOKEN_PLUS);
            result = ast_binop(result, parse_term(parser), OP_ADD);
        } else if (parser->tokens.data[parser->pos].type == TOKEN_MINUS) {
            parser_expect(parser, TOKEN_MINUS);
            result = ast_binop(result, parse_term(parser), OP_SUB);
        }
    }

    return result;
}

char *node_to_debug_string(Node* node) {
    // TODO: memory management needed here
    char *output = malloc(1024);
    switch (node->type) {
        case NODE_INTEGER: sprintf(output, "%s(%d)", node_type_string_table[node->type], node->value_i32); break;
        case NODE_SYMBOL: sprintf(output, "%s(%c)", node_type_string_table[node->type], node->name); break;
        case NODE_BINOP: sprintf(output, "%s(%s, %s)", op_type_string_table[node->op_type], node_to_debug_string(node->left), node_to_debug_string(node->right)); break;
        case NODE_UNARYOP: sprintf(output, "%s(%s)", op_type_string_table[node->op_type], node_to_debug_string(node->expr)); break;
        case NODE_FUNC: sprintf(output, "%s(%s)", func_type_string_table[node->func_type], node_to_debug_string(node->expr)); break;
        default: fprintf(stderr, "ERROR: Cannot do 'node_to_debug_string' because node type '%s' is not implemented.\n", node_type_string_table[node->type]); exit(1);
    }
    return output;
}

char *node_to_string(Node* node) {
    char *output = malloc(1024);
    switch (node->type) {
        case NODE_INTEGER: sprintf(output, "%d", node->value_i32); break;
        case NODE_SYMBOL: sprintf(output, "%c", node->name); break;
        case NODE_BINOP: {
            sprintf(output, "(%s%c%s)", node_to_string(node->left), op_type_char_table[node->op_type], node_to_string(node->right));
            break;
        }
        case NODE_UNARYOP: sprintf(output, "%c%s", op_type_char_table[node->op_type], node_to_string(node->expr)); break;
        case NODE_FUNC: sprintf(output, "%s(%s)", func_type_string_table[node->func_type], node_to_string(node->expr)); break;
        default: fprintf(stderr, "ERROR: Cannot do 'node_to_debug_string' because node type '%s' is not implemented.\n", node_type_string_table[node->type]); exit(1);
    }
    return output;
}

Node* interp_binop_add(Node* node) {
    Node* left = interp(node->left);
    Node* right = interp(node->right);
    if (left->type == NODE_INTEGER && right->type == NODE_INTEGER) {
        return ast_integer(left->value_i32 + right->value_i32);
    } else if (node_eq(left, right)) {
        return ast_binop(ast_integer(2), left, OP_MUL);
    } else if (left->type == NODE_INTEGER && right->type == NODE_BINOP && right->op_type == OP_DIV) {
        // c + a/b
        // -> (cb)/b + a/b
        Node* r = ast_binop(ast_binop(ast_binop(left, right->right, OP_MUL), right->right, OP_DIV), right, OP_ADD);
        return interp_binop(r);
    } else if (left->type == NODE_BINOP && left->op_type == OP_MUL) {
        if (node_eq(left->right, right)) {
            Node* new_left = interp_binop(ast_binop(ast_integer(1), left->left, OP_ADD));
            return ast_binop(new_left, right, OP_MUL);
        }
    }
    return ast_binop(left, right, OP_ADD);
}

Node* interp_binop_sub(Node* node) {
    Node* left = interp(node->left);
    Node* right = interp(node->right);
    if (left->type == NODE_INTEGER && right->type == NODE_INTEGER) {
        return ast_integer(left->value_i32 - right->value_i32);
    }
    return ast_binop(left, right, OP_SUB);
}

Node* interp_binop_mul(Node* node) {
    Node* left = interp(node->left);
    Node* right = interp(node->right);
    if (left->type == NODE_INTEGER && right->type == NODE_INTEGER) {
        return ast_integer(left->value_i32 * right->value_i32);
    }
    return ast_binop(left, right, OP_MUL);
}

Node* interp_binop_div(Node* node) {
    Node* left = interp(node->left);
    Node* right = interp(node->right);
    OpType op_type = node->op_type;
    if (left->type == NODE_INTEGER && right->type == NODE_INTEGER) {
        if (left->value_i32 % right->value_i32 == 0) {
            return ast_integer(left->value_i32 / right->value_i32);
        } else {
            return ast_binop(left, right, op_type);
        }
    } else if (left->type == NODE_INTEGER && right->type == NODE_BINOP && right->op_type == OP_DIV) {
        Node* r = ast_binop(ast_binop(left, right->left, OP_MUL), right->right, OP_DIV);
        return interp_binop(r);
    }
    return ast_binop(left, right, OP_DIV);
}

Node* interp_binop(Node* node) {
    switch (node->op_type) {
        case OP_ADD: return interp_binop_add(node);
        case OP_SUB: return interp_binop_sub(node);
        case OP_MUL: return interp_binop_mul(node);
        case OP_DIV: return interp_binop_div(node);
        default: assert(false);
    }
}

Node* interp_unaryop(Node* node) {
    Node* expr = interp(node->expr);
    OpType op_type = node->op_type;

    if (expr->type == NODE_INTEGER) {
        switch (op_type) {
            case OP_UADD: return ast_integer(+expr->value_i32);
            case OP_USUB: return ast_integer(-expr->value_i32);
            default: assert(false);
        }
    }

    return ast_unaryop(expr, op_type);
}

Node* interp_func(Node* node) {
    Node* expr = interp(node->expr);
    if (expr->type == NODE_INTEGER && node->func_type == FUNC_SQRT) {
        double result = sqrt((double)node->expr->value_i32);
        if (fmod((double)node->expr->value_i32, result) == 0.0) {
            return ast_integer((int)result);
        }

        // check for perfect square
        // @Speed: this is probably an inefficient way to compute the biggest perfect sqaure factor of a given number
        for (int q = node->expr->value_i32; q > 1; q--) {
            double sqrt_of_q = sqrtf((double)q);
            bool is_perfect_square = sqrt_of_q == floor(sqrt_of_q);
            if (is_perfect_square && node->expr->value_i32 % q == 0) {
                int p = node->expr->value_i32 / q;
                return ast_binop(ast_integer((int)sqrt_of_q), ast_func(FUNC_SQRT, ast_integer(p)), OP_MUL);
            }
        }
    }

    return node;
}

Node* interp(Node* node) {
    switch (node->type) {
        case NODE_BINOP:
            return interp_binop(node);
        case NODE_UNARYOP:
            return interp_unaryop(node);
        case NODE_INTEGER:
        case NODE_SYMBOL:
            return node;
        case NODE_FUNC:
            return interp_func(node);
        default:
            fprintf(stderr, "ERROR: Cannot interp node type '%s'\n", node_type_string_table[node->type]);
            exit(1);
    }
}

void test() {
    size_t test_id = 1;

    #define TEST_SOURCE_TO_PARSE(source, test_case) {{ \
        Tokens tokens = tokenize(source); \
        Parser parser = { .tokens = tokens, .pos=0 }; \
        Node* ast = parse_expr(&parser); \
        printf("test %02zu...........................", test_id);\
        if (!node_eq(ast, test_case)) {\
            printf("failed\n");\
            printf("%s\n", node_to_string(ast));\
            printf("!=\n");\
            printf("%s\n", node_to_string(test_case));\
            exit(1); \
        } else { \
            printf("passed\n");\
        }\
        test_id++;\
    }}

    #define TEST_SOURCE_TO_INTERP(source, test_case) {{ \
        Tokens tokens = tokenize(source); \
        Parser parser = { .tokens = tokens, .pos=0 }; \
        Node* ast = parse_expr(&parser); \
        Node* output = interp(ast); \
        printf("test %02zu...........................", test_id);\
        if (!node_eq(output, test_case)) {\
            printf("failed\n");\
            printf("%s\n", node_to_string(output));\
            printf("!=\n");\
            printf("%s\n", node_to_string(test_case));\
            exit(1); \
        } else { \
            printf("passed\n");\
        }\
        test_id++;\
    }}

    // ruslanspivak interpreter tutorial tests
    TEST_SOURCE_TO_INTERP("27 + 3", ast_integer(30));
    TEST_SOURCE_TO_INTERP("27 - 7", ast_integer(20));
    TEST_SOURCE_TO_INTERP("7 - 3 + 2 - 1", ast_integer(5));
    TEST_SOURCE_TO_INTERP("10 + 1 + 2 - 3 + 4 + 6 - 15", ast_integer(5));
    TEST_SOURCE_TO_INTERP("7 * 4 / 2", ast_integer(14));
    TEST_SOURCE_TO_INTERP("7 * 4 / 2 * 3", ast_integer(42));
    TEST_SOURCE_TO_INTERP("10 * 4  * 2 * 3 / 8", ast_integer(30));
    TEST_SOURCE_TO_INTERP("7 - 8 / 4", ast_integer(5));
    TEST_SOURCE_TO_INTERP("14 + 2 * 3 - 6 / 2", ast_integer(17));
    TEST_SOURCE_TO_INTERP("7 + 3 * (10 / (12 / (3 + 1) - 1))", ast_integer(22));
    TEST_SOURCE_TO_INTERP("7 + 3 * (10 / (12 / (3 + 1) - 1)) / (2 + 3) - 5 - 3 + (8)", ast_integer(10));
    TEST_SOURCE_TO_INTERP("7 + (((3 + 2)))", ast_integer(12));
    TEST_SOURCE_TO_INTERP("- 3", ast_integer(-3));
    TEST_SOURCE_TO_INTERP("+ 3", ast_integer(3));
    TEST_SOURCE_TO_INTERP("5 - - - + - 3", ast_integer(8));
    TEST_SOURCE_TO_INTERP("5 - - - + - (3 + 4) - +2", ast_integer(10));

    // things from the sympy tutorial
    TEST_SOURCE_TO_PARSE("sqrt(3)", ast_func(FUNC_SQRT, ast_integer(3)));
    TEST_SOURCE_TO_INTERP("sqrt(8)", ast_binop(ast_integer(2), ast_func(FUNC_SQRT, ast_integer(2)), OP_MUL));

    // misc
    TEST_SOURCE_TO_INTERP("sqrt(9)", ast_integer(3));
}

void tokens_print(Tokens *tokens) {
    for (size_t i = 0; i < tokens->size; i++) {
        Token *token = &tokens->data[i];
        printf("%s('%s') ", token_type_string_table[token->type], token->text);
    }
    printf("\n");
}