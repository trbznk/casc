// TODO: maybe there shouldnt be ASTConstant. Just AST Symbol
// TODO: remove Token fields from AST nodes

#ifndef CASC_H
#define CASC_H

#include <stdlib.h>

//
// forward declarations
//

typedef struct AST AST;
typedef struct Parser Parser;
typedef struct Arena Arena;
typedef struct Worker Worker;

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

#define INTEGER(value) create_ast_integer(&w->arena, value)
#define ADD(left, right) create_ast_binop(&w->arena, left, right, OP_ADD)
#define SUB(left, right) create_ast_binop(&w->arena, left, right, OP_SUB)
#define MUL(left, right) create_ast_binop(&w->arena, left, right, OP_MUL)
#define DIV(left, right) create_ast_binop(&w->arena, left, right, OP_DIV)

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
            char* name;
        } symbol;

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

AST* create_ast_integer(Arena*, int64_t);
AST* create_ast_symbol(Arena*, char*);
AST* create_ast_binop(Arena*, AST*, AST*, OpType);
AST* create_ast_unaryop(Arena*, AST*, OpType);
AST* create_ast_func_call(Arena*, Token, ASTArray);
AST* create_ast_empty(Arena*);

void ast_array_append(ASTArray*, AST*);

const char* op_type_to_debug_string(OpType);
const char* op_type_to_string(OpType);
const char* ast_type_to_debug_string(ASTType);

//
// parser
//

AST* parse(Worker*);
AST* parse_expr(Worker*);
AST* parse_factor(Worker*);

AST* parse_from_string(Worker*, char*);

//
// interp
//

AST *interp(Worker*, AST*);
AST *interp_binop_pow(Worker*, AST*);

AST* interp_from_string(Worker*, char*);

bool ast_match(AST*, AST*);
bool ast_match_type(AST*, AST*);

char *ast_to_string(AST*);
char *ast_to_debug_string(AST*);

//
// gui
//
void init_gui();

//
// general
//

struct Arena {
    void *memory;
    size_t offset;
    size_t size;
};

Arena create_arena(size_t);
void arena_free(Arena*);
void *arena_alloc(Arena*, size_t);

struct Worker {
    Arena arena;
    Lexer lexer;
};

Worker create_worker();

#endif