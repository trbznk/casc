#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>

#define FIXED_STRING_SIZE 32

// forward declaration
typedef struct Node Node;
typedef struct Parser Parser;
Node* parse_expr(Parser* parser);
Node* interp(Node* node);

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

typedef struct {
    TokenType type;
    char lexeme[FIXED_STRING_SIZE];
} Token;

typedef struct {
    size_t size;
    size_t capacity;
    Token *tokens;    
} Lexer;

typedef enum {
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,

    OP_UADD,
    OP_USUB,

    OP_TYPE_COUNT
} OpType;

typedef enum {
    NODE_INTEGER,
    NODE_BINOP,
    NODE_UNARYOP,

    NODE_TYPE_COUNT
} NodeType;

struct Node {
    Node* left;
    Node* right;
    Node* expr;

    NodeType type;
    OpType op_type;
    int value_i32;
};

struct Parser {
    Lexer lexer;
    size_t pos;
};

const char* token_type_name(TokenType type) {
    static const char* names[] = {
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
    _Static_assert(sizeof(names)/sizeof(names[0]) == TOKEN_TYPE_COUNT, "ERROR: Wrong number of names.");
    assert(type < TOKEN_TYPE_COUNT);
    return names[type];
}

const char* op_type_name(OpType type) {
    static const char* names[] = {
        "Add",
        "Sub",
        "Mul",
        "Div",
        "UAdd",
        "USub"
    };
    _Static_assert(sizeof(names)/sizeof(names[0]) == OP_TYPE_COUNT, "ERROR: Wrong number of names.");
    assert(type < OP_TYPE_COUNT);
    return names[type];
}

const char* node_type_name(NodeType type) {
    static const char* names[] = {
        "Integer",
        "BinOp",
        "UnaryOp"
    };
    _Static_assert(sizeof(names)/sizeof(names[0]) == NODE_TYPE_COUNT, "ERROR: Wrong number of names.");
    assert(type < NODE_TYPE_COUNT);
    return names[type];
}

void lexer_push(Lexer *lexer, Token token) {
    const size_t alloc_capacity = 10;
    if (lexer->size >= lexer->capacity) {
        if (lexer->size == 0) {
            lexer->tokens = malloc(alloc_capacity*sizeof(Token));
        } else {
            lexer->tokens = realloc(lexer->tokens ,(lexer->size+alloc_capacity)*sizeof(Token));
        }
        lexer->capacity = lexer->capacity + alloc_capacity;
    }
    lexer->tokens[lexer->size] = token;
    lexer->size++;
}

Lexer lex(char* source) {
    Lexer lexer = {0};
    size_t i = 0;
    while (i < strlen(source)) {
        if (isdigit(source[i])) {
            char number_buffer[FIXED_STRING_SIZE] = "";
            while (isdigit(source[i])) {
                strncat(number_buffer, &source[i], 1);
                i++;
            }
            Token token = { .type = TOKEN_NUMBER };
            strcpy(token.lexeme, number_buffer);
            lexer_push(&lexer, token);
            i--;
        } else if (isalpha(source[i])) {
            char number_buffer[FIXED_STRING_SIZE] = "";
            size_t first = i;
            while (isalpha(source[i]) || (i > first && (isalpha(source[i]) || isdigit(source[i])))) {
                strncat(number_buffer, &source[i], 1);
                i++;
            }
            Token token = { .type = TOKEN_IDENTIFIER };
            strcpy(token.lexeme, number_buffer);
            lexer_push(&lexer, token);
            i--;
        } else if (source[i] == '+') {
            Token token = { .type = TOKEN_PLUS };
            strncpy(token.lexeme, &source[i], 1);
            lexer_push(&lexer, token);
        } else if (source[i] == '-') {
            Token token = { .type = TOKEN_MINUS };
            strncpy(token.lexeme, &source[i], 1);
            lexer_push(&lexer, token);
        } else if (source[i] == '*') {
            Token token = { .type = TOKEN_STAR };
            strncpy(token.lexeme, &source[i], 1);
            lexer_push(&lexer, token);
        } else if (source[i] == '/') {
            Token token = { .type = TOKEN_SLASH };
            strncpy(token.lexeme, &source[i], 1);
            lexer_push(&lexer, token);
        } else if (source[i] == '^') {
            Token token = { .type = TOKEN_CARET };
            strncpy(token.lexeme, &source[i], 1);
            lexer_push(&lexer, token);
        } else if (source[i] == '(') {
            Token token = { .type = TOKEN_L_PAREN };
            strncpy(token.lexeme, &source[i], 1);
            lexer_push(&lexer, token);
        } else if (source[i] == ')') {
            Token token = { .type = TOKEN_R_PAREN };
            strncpy(token.lexeme, &source[i], 1);
            lexer_push(&lexer, token);
        } else if (isspace(source[i])) {
            // ignore spaces for now
        } else {
            fprintf(stderr, "ERROR: Can't tokenize '%c'\n", source[i]);
            exit(1);
        }
        i++;
    }
    assert(i == strlen(source));
    lexer_push(&lexer, (Token){ .type=TOKEN_EOF });
    return lexer;
}

Token parser_next_token(Parser* parser) {
    Token next_token = parser->lexer.tokens[parser->pos];
    parser->pos++;
    return next_token;
}

void parser_expect(Parser* parser, TokenType type) {
    if (parser->lexer.tokens[parser->pos].type == type) {
        parser->pos++;
    } else {
        fprintf(stderr, "ERROR: Expected %s got %s.", token_type_name(type), token_type_name(parser->lexer.tokens[parser->pos].type));
    }
}

Node* ast_integer(int value) {
    Node* node = malloc(sizeof(Node));
    node->type = NODE_INTEGER;
    node->value_i32 = value;
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

bool node_eq(Node* left, Node* right) {
    if (left->type == right->type) {
        switch (left->type) {
            case NODE_INTEGER:
                return left->value_i32 == right->value_i32;
            case NODE_BINOP:
                return node_eq(left->left, right->left) && node_eq(left->right, right->right);
            default: fprintf(stderr, "ERROR: node type %s not implemented.\n", node_type_name(left->type)); exit(1);
        }
    }
    return false;
}

Node* parse_factor(Parser* parser) {
    Token token = parser->lexer.tokens[parser->pos];
    if (token.type == TOKEN_NUMBER) {
        parser_expect(parser, TOKEN_NUMBER);
        return ast_integer(atoi(token.lexeme));   
    } else if (token.type == TOKEN_L_PAREN) {
        parser_expect(parser, TOKEN_L_PAREN);
        Node* r = parse_expr(parser);
        parser_expect(parser, TOKEN_R_PAREN);
        return r;
    } else if (token.type == TOKEN_MINUS) {
        parser_expect(parser, TOKEN_MINUS);
        return ast_unaryop(parse_factor(parser), OP_USUB);
    } else if (token.type == TOKEN_PLUS) {
        parser_expect(parser, TOKEN_PLUS);
        return ast_unaryop(parse_factor(parser), OP_UADD);
    } else {
        assert(false);
    }
}

Node* parse_term(Parser* parser) {
    Node* result = parse_factor(parser);
    while (parser->lexer.tokens[parser->pos].type == TOKEN_STAR || parser->lexer.tokens[parser->pos].type == TOKEN_SLASH) {
        if (parser->lexer.tokens[parser->pos].type == TOKEN_STAR) {
            parser_expect(parser, TOKEN_STAR);
            result = ast_binop(result, parse_factor(parser), OP_MUL);
        } else if (parser->lexer.tokens[parser->pos].type == TOKEN_SLASH) {
            parser_expect(parser, TOKEN_SLASH);
            result = ast_binop(result, parse_factor(parser), OP_DIV);
        }
    }
    return result;
}

Node* parse_expr(Parser* parser) {
    Node* result = parse_term(parser);

    assert(parser->pos < parser->lexer.size);
    while (parser->lexer.tokens[parser->pos].type == TOKEN_PLUS || parser->lexer.tokens[parser->pos].type == TOKEN_MINUS) {
        if (parser->lexer.tokens[parser->pos].type == TOKEN_PLUS) {
            parser_expect(parser, TOKEN_PLUS);
            result = ast_binop(result, parse_term(parser), OP_ADD);
        } else if (parser->lexer.tokens[parser->pos].type == TOKEN_MINUS) {
            parser_expect(parser, TOKEN_MINUS);
            result = ast_binop(result, parse_term(parser), OP_SUB);
        }
    }

    return result;
}

char* node_to_string(Node* node) {
    // TODO: memory management needed here
    char* output = malloc(1024);
    switch (node->type) {
        case NODE_INTEGER: sprintf(output, "%s(%d)", node_type_name(node->type), node->value_i32); break;
        case NODE_BINOP: sprintf(output, "%s(%s, %s)", op_type_name(node->op_type), node_to_string(node->left), node_to_string(node->right)); break;
        case NODE_UNARYOP: sprintf(output, "%s(%s)", op_type_name(node->op_type), node_to_string(node->expr)); break;
        default: fprintf(stderr, "ERROR: node type %s not implemented.\n", node_type_name(node->type)); exit(1);
    }
    return output;
}

Node* interp_binop(Node* node) {
    // depth first
    Node* left = interp(node->left);
    Node* right = interp(node->right);
    OpType op_type = node->op_type;

    if (left->type == NODE_INTEGER && right->type == NODE_INTEGER) {
        switch (op_type) {
            case OP_ADD: return ast_integer(left->value_i32 + right->value_i32);
            case OP_SUB: return ast_integer(left->value_i32 - right->value_i32);
            case OP_MUL: return ast_integer(left->value_i32 * right->value_i32);
            case OP_DIV: {
                if (left->value_i32 % right->value_i32 == 0) {
                    return ast_integer(left->value_i32 / right->value_i32);
                } else {
                    return ast_binop(left, right, op_type);
                }
            }
            default: assert(false);
        }
    } else if (left->type == NODE_INTEGER && right->type == NODE_BINOP && right->op_type == OP_DIV) {
        switch (op_type) {
            case OP_ADD: {
                // c + a/b
                // -> (cb)/b + a/b
                Node* r = ast_binop(ast_binop(ast_binop(left, right->right, OP_MUL), right->right, OP_DIV), right, OP_ADD);
                return interp_binop(r);
            }
            case OP_SUB: assert(false);
            case OP_MUL: {
                Node* r = ast_binop(ast_binop(left, right->left, OP_MUL), right->right, OP_DIV);
                return interp_binop(r);
            }
            case OP_DIV: assert(false);
            default: assert(false);
        }
    }

    return ast_binop(left, right, op_type);
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

Node* interp(Node* node) {
    switch (node->type) {
        case NODE_BINOP:
            return interp_binop(node);
        case NODE_UNARYOP:
            return interp_unaryop(node);
        case NODE_INTEGER:
            return node;
        default:
            fprintf(stderr, "ERROR: Cannot interpretate node type %s.\n", node_type_name(node->type));
            exit(1);
    }
}

void test() {
    size_t id = 0;
    #define TEST_CASE(source, test_case) {{ \
        Lexer lexer = lex(source); \
        Parser parser = { .lexer = lexer, .pos=0 }; \
        Node* ast = parse_expr(&parser); \
        Node* output = interp(ast); \
        printf("test %02zu...........................", id);\
        if (!node_eq(output, test_case)) {\
            printf("failed\n");\
            exit(1); \
        } else { \
            printf("passed\n");\
        }\
        id++;\
    }}
    TEST_CASE("27 + 3", ast_integer(30));
    TEST_CASE("27 - 7", ast_integer(20));
    TEST_CASE("7 - 3 + 2 - 1", ast_integer(5));
    TEST_CASE("10 + 1 + 2 - 3 + 4 + 6 - 15", ast_integer(5));
    TEST_CASE("7 * 4 / 2", ast_integer(14));
    TEST_CASE("7 * 4 / 2 * 3", ast_integer(42));
    TEST_CASE("10 * 4  * 2 * 3 / 8", ast_integer(30));
    TEST_CASE("7 - 8 / 4", ast_integer(5));
    TEST_CASE("14 + 2 * 3 - 6 / 2", ast_integer(17));
    TEST_CASE("7 + 3 * (10 / (12 / (3 + 1) - 1))", ast_integer(22));
    TEST_CASE("7 + 3 * (10 / (12 / (3 + 1) - 1)) / (2 + 3) - 5 - 3 + (8)", ast_integer(10));
    TEST_CASE("7 + (((3 + 2)))", ast_integer(12));
    TEST_CASE("- 3", ast_integer(-3));
    TEST_CASE("+ 3", ast_integer(3));
    TEST_CASE("5 - - - + - 3", ast_integer(8));
    TEST_CASE("5 - - - + - (3 + 4) - +2", ast_integer(10));
}

int main() {
    test();

    char *source = "7 * 4 / 2 * 3";
    printf("source................\n'%s'\n", source);
    Lexer lexer = lex(source);
    Parser parser = { .lexer = lexer, .pos=0 };
    Node* ast = parse_expr(&parser);

    // print tokens
    printf("tokens................\n");
    for (size_t i = 0; i < lexer.size; i++) {
        Token *token = &lexer.tokens[i];
        printf("%s('%s') ", token_type_name(token->type), token->lexeme);
    }
    printf("\n");
    printf("ast................\n");
    printf("%s\n", node_to_string(ast));
    // printf("lexer.size=%zu, lexer.capacity=%zu\n", lexer.size, lexer.capacity);

    printf("output................\n");
    Node* output = interp(ast);
    printf("%s\n", node_to_string(output));
}
