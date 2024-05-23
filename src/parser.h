#ifndef PARSER_H
#define PARSER_H

#include <stdbool.h>

#include "lexer.h"


typedef struct Parser Parser;
typedef struct AST AST;
typedef struct ASTInteger ASTInteger;
typedef struct ASTSymbol ASTSymbol;
typedef struct ASTBinOp ASTBinOp;
typedef struct ASTUnaryOp ASTUnaryOp;
typedef struct ASTFuncCall ASTFuncCall;

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
    AST_INTEGER,
    AST_SYMBOL,
    AST_BINOP,
    AST_UNARYOP,

    // TODO: rename this to AST_FUNC_CALL everywhere to make it more explicit
    AST_FUNC_CALL,

    // TODO: remove AST_EMPTY type when there are more high level nodes like NODE_PROGRAMM or something similar
    AST_EMPTY,

    AST_TYPE_COUNT
} ASTType;

struct ASTInteger {
    int64_t value;
};

struct ASTSymbol {
    Token name;
};

struct ASTBinOp {
    AST* left;
    AST* right;
    OpType type;
};

struct ASTUnaryOp {
    AST* expr;
    OpType type;
};

struct ASTFuncCall {
    Token name;
    AST* arg;
};

struct AST {
    ASTType type;
    union {
        ASTInteger integer;
        ASTSymbol symbol;
        ASTBinOp binop;
        ASTUnaryOp unary_op;
        ASTFuncCall func_call;
        bool empty; // temporary for ASTType empty
    };
};

struct Parser {
    Tokens tokens;
    size_t pos;
};

AST* create_ast_integer(int64_t);
AST* create_ast_symbol(Token);
AST* create_ast_binop(AST*, AST*, OpType);
AST* create_ast_unaryop(AST*, OpType);
AST* create_ast_func_call(Token, AST*);
AST* create_ast_empty();

AST* parse_expr(Parser*);
AST* parse_from_string(char*);

const char* op_type_to_debug_string(OpType);
const char* op_type_to_string(OpType);
const char* ast_type_to_debug_string(ASTType);

#endif