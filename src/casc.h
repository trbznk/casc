#pragma once

#include <stdlib.h>

//
// Basic Types
//

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef size_t usize;
typedef float f32;
typedef double f64;

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

extern const char *BUILTIN_FUNCTIONS[];
extern const usize BUILTIN_FUNCTIONS_COUNT;

extern const char *BUILTIN_CONSTANTS[];
extern const usize BUILTIN_CONSTANTS_COUNT;

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
    char text[64];
    
    bool contains_dot;
} Token;

typedef struct Lexer Lexer;
struct Lexer {
    char* source;
    usize pos;
};

char lexer_current_char(Lexer*);
Token lexer_peek_token(Lexer*);
Token lexer_next_token(Lexer*);
void lexer_print_tokens(Lexer*);
void print_tokens_from_string(char*);

const char* token_type_to_string(TokenType);

bool is_builtin_function(char*);
bool is_builtin_constant(char*);

//
// ast
//

#define create_ast_integer(value) _create_ast_integer(&worker->arena, value)
#define create_ast_real(value) _create_ast_real(&worker->arena, value)
#define create_ast_symbol(name) _create_ast_symbol(&worker->arena, name)
#define create_ast_add(left, right) _create_ast_binop(&worker->arena, left, right, OP_ADD)
#define create_ast_sub(left, right) _create_ast_binop(&worker->arena, left, right, OP_SUB)
#define create_ast_mul(left, right) _create_ast_binop(&worker->arena, left, right, OP_MUL)
#define create_ast_div(left, right) _create_ast_binop(&worker->arena, left, right, OP_DIV)
#define create_ast_pow(left, right) _create_ast_binop(&worker->arena, left, right, OP_POW)

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
    AST_REAL,
    AST_SYMBOL,
    AST_BINOP,
    AST_UNARYOP,

    AST_CALL,

    // @remove AST_EMPTY type when there are more high level nodes like NODE_PROGRAMM or something similar
    AST_EMPTY,

    AST_TYPE_COUNT
} ASTType;

struct ASTArray {
    usize size;
    AST **data;
};

struct AST {
    ASTType type;
    union {

        struct {
            i64 value;
        } integer;

        struct {
            f64 value;
        } real;

        struct {
            // fixed length? @todo
            char *name;
        } symbol;

        struct {
            AST *left;
            AST *right;
            OpType type;
        } binop;

        struct {
            AST *expr;
            OpType type;
        } unaryop;

        struct {
            char name[64];
            ASTArray args;
        } func_call;

        bool empty; // temporary for ASTType empty - @remove
    };
};

AST* _create_ast_integer(Arena*, i64);
AST* _create_ast_real(Arena*, f64);
AST* _create_ast_symbol(Arena*, char*);
AST* _create_ast_binop(Arena*, AST*, AST*, OpType);
AST* create_ast_unaryop(Arena*, AST*, OpType);
AST* create_ast_call(Arena*, char*, ASTArray);
AST* create_ast_empty(Arena*);

void ast_array_append(Arena*, ASTArray*, AST*);

const char *op_type_to_debug_string(OpType);
const char *op_type_to_string(OpType);
const char *ast_type_to_debug_string(ASTType);
uint8_t op_type_precedence(OpType);

ASTArray ast_to_flat_array(Arena*, AST*);
bool ast_contains(AST*, AST*);

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

char *_ast_to_string(Arena*, AST*, u8);
#define ast_to_string(arena, node) _ast_to_string(arena, node, 0)
char *ast_to_debug_string(Arena*, AST*);

//
// gui
//

void init_gui();

//
// general
//

#define todo() printf("%s:%d: not implemented\n", __FILE__, __LINE__); exit(1);

struct Arena {
    void *memory;

    usize offset;
    usize size;

    u32 reallocs_count;
};

Arena create_arena(usize);
void arena_free(Arena*);
void *arena_alloc(Arena*, usize);

struct Worker {
    Arena arena;
    Lexer lexer;
};

Worker create_worker();
