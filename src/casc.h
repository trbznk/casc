#ifndef CASC_H
#define CASC_H

#include <stdlib.h>

//
// general
//

typedef struct Allocator Allocator;

struct Allocator {
    void *memory;
    size_t head;
};

Allocator create_allocator(size_t);
void allocator_free(Allocator*);
void *mmalloc(Allocator*, size_t);

//
// lexer
//

#define FIXED_STRING_SIZE 32

extern const char *BUILTIN_FUNCTIONS[];
extern const size_t BUILTIN_FUNCTIONS_COUNT;

extern const char *BUILTIN_CONSTANTS[];
extern const size_t BUILTIN_CONSTANTS_COUNT;

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

    TOKEN_COMMA,
    TOKEN_HASH,

    TOKEN_EOF,

    TOKEN_TYPE_COUNT
} TokenType;

typedef struct {
    TokenType type;

    // TODO: I think we don't want to have fixed strings here in general
    char text[FIXED_STRING_SIZE];
} Token;

// TODO: transform this into a lexer. This is not only for naming, but to make it clearer
//       and differ it from the parser. Although we can implemente methods like peek_next_char
//       and similar things more easily.
typedef struct Lexer Lexer;
struct Lexer {
    char* source;
    size_t pos;
};

char lexer_current_char(Lexer*);
Token lexer_peek_token(Lexer*);
Token lexer_next_token(Lexer*);
void lexer_print_tokens(Lexer*);

const char* token_type_to_string(TokenType);

bool is_builtin_function(char*);
bool is_builtin_constant(char*);

//
// ast
//

typedef struct AST AST;
typedef struct ASTArray ASTArray;

typedef enum {
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_POW,

    OP_UADD,
    OP_USUB,

    OP_TYPE_COUNT
} OpType;

typedef enum {
    AST_INTEGER,
    AST_SYMBOL,
    AST_CONSTANT,
    AST_BINOP,
    AST_UNARYOP,

    // TODO: rename this to AST_FUNC_CALL everywhere to make it more explicit
    AST_FUNC_CALL,

    // TODO: remove AST_EMPTY type when there are more high level nodes like NODE_PROGRAMM or something similar
    AST_EMPTY,

    AST_TYPE_COUNT
} ASTType;

struct ASTArray {
    size_t size;
    AST **data;
};

struct AST {
    ASTType type;
    union {

        struct {
            int64_t value;
        } integer;

        struct {
            Token name;
        } symbol;
        
        struct {
            Token name;
        } constant;

        struct {
            AST* left;
            AST* right;
            OpType type;
        } binop;

        struct {
            AST* expr;
            OpType type;
        } unaryop;

        struct {
            Token name;
            ASTArray args;
        } func_call;

        bool empty; // TODO: temporary for ASTType empty. Remove this
    };
};

AST* create_ast_integer(int64_t);
AST* create_ast_symbol(Token);
AST* create_ast_constant(Token);
AST* create_ast_binop(AST*, AST*, OpType);
AST* create_ast_unaryop(AST*, OpType);
AST* create_ast_func_call(Token, ASTArray);
AST* create_ast_empty();

void ast_array_append(ASTArray*, AST*);

const char* op_type_to_debug_string(OpType);
const char* op_type_to_string(OpType);
const char* ast_type_to_debug_string(ASTType);

//
// parser
//

typedef struct Parser Parser;

struct Parser {
    Lexer lexer;
};

AST* parse(Parser*);
AST* parse_expr(Parser*);
AST* parse_factor(Parser*);
AST* parse_from_string(char*);

//
// interp
//

AST *interp(AST*);
AST *interp_binop_pow(AST*);

AST* interp_from_string(char*);

bool ast_match(AST*, AST*);
bool ast_match_type(AST*, AST*);

char *ast_to_string(AST*);
char *ast_to_debug_string(AST*);

//
// gui
//
void init_gui();

#endif