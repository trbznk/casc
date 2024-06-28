#pragma once

#include <stdlib.h>

#define MAX_VARIABLES 1024

//
// forward declarations
//

typedef struct AST AST;
typedef struct Parser Parser;
typedef struct Arena Arena;
typedef struct Allocator Allocator;
typedef struct String String;
typedef struct Lexer Lexer;

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

struct String {
    char *str;
    usize size;
};

String init_string(const char *str);
String char_to_string(Allocator *allocator, char c);
String string_slice(Allocator *allocator, String s, usize start, usize stop);
String string_concat(Allocator *allocator, String s1, String s2);
String string_insert(Allocator *allocator, String s1, String s2, usize idx);
String string_insert_char(Allocator *allocator, String s1, char c, usize idx);
bool string_eq(String s1, String s2);
void print(String s);

//
// lexer
//

typedef enum {
    TOKEN_NUMBER,
    TOKEN_IDENTIFIER,
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_STAR,
    TOKEN_SLASH,
    TOKEN_CARET,

    TOKEN_EQUAL,

    TOKEN_L_PAREN,
    TOKEN_R_PAREN,

    TOKEN_COMMA,
    TOKEN_HASH,
    TOKEN_EXCLAMATION_MARK,

    TOKEN_NEW_LINE,
    TOKEN_EOF,

    TOKEN_TYPE_COUNT
} TokenType;

typedef struct {
    TokenType type;
    String text;
    
    // TODO: replace this and the occurences with string method
    //       string_contains
    bool contains_dot;
} Token;

struct Lexer {
    Allocator *allocator;
    String source;
    usize pos;
};

char lexer_current_char(Lexer*);
Token lexer_peek_token(Lexer*);
Token lexer_next_token(Lexer*);
void lexer_print_tokens(Lexer*);

const char* token_type_to_string(TokenType);

bool is_builtin_function(String);
bool is_builtin_constant(String);

//
// ast
//

#define INTEGER(value) init_ast_integer(ip->allocator, value)
#define REAL(value) init_ast_real(ip->allocator, value)
#define SYMBOL(name) init_ast_symbol(ip->allocator, name)
#define CONSTANT(name) init_ast_constant(ip->allocator, name)
#define ADD(left, right) init_ast_binop(ip->allocator, left, right, OP_ADD)
#define SUB(left, right) init_ast_binop(ip->allocator, left, right, OP_SUB)
#define MUL(left, right) init_ast_binop(ip->allocator, left, right, OP_MUL)
#define DIV(left, right) init_ast_binop(ip->allocator, left, right, OP_DIV)
#define POW(left, right) init_ast_binop(ip->allocator, left, right, OP_POW)
#define CALL(name, args) init_ast_call(ip->allocator, name, args)
#define ASSIGN(target, value) init_ast_assign(ip->allocator, target, value)
#define EMPTY() init_ast_empty(ip->allocator)

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
    AST_PROGRAM,

    AST_INTEGER,
    AST_REAL,
    AST_SYMBOL,
    AST_CONSTANT,
    AST_BINOP,
    AST_UNARYOP,

    AST_CALL,
    AST_ASSIGN,

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
            String name;
        } symbol;

        struct {
            String name;
        } constant;

        struct {
            AST *left;
            AST *right;
            OpType op;
        } binop;

        struct {
            AST *operand;
            OpType op;
        } unaryop;

        struct {
            String name;
            ASTArray args;
        } func_call;

        struct {
            AST *target;
            AST *value;
        } assign;

        struct {
            ASTArray statements;
        } program;

        bool empty; // TODO: temporary for ASTType empty
    };
};

AST* init_ast_program(Allocator*);
AST* init_ast_assign(Allocator*, AST *target, AST *value);
AST* init_ast_integer(Allocator*, i64);
AST* init_ast_real(Allocator*, f64);
AST* init_ast_symbol(Allocator*, String);
AST* init_ast_constant(Allocator*, String);
AST* init_ast_binop(Allocator*, AST*, AST*, OpType);
AST* init_ast_unaryop(Allocator*, AST*, OpType);
AST* init_ast_call(Allocator*, String, ASTArray);
AST* init_ast_empty(Allocator*);

void ast_array_append(Allocator*, ASTArray*, AST*);

const char *op_type_to_debug_string(OpType);
const char *op_type_to_string(OpType);
const char *ast_type_to_debug_string(ASTType);
uint8_t op_type_precedence(OpType);

ASTArray ast_to_flat_array(Allocator*, AST*);
bool ast_contains(AST*, AST*);
bool ast_is_fraction(AST*);
bool ast_is_numeric(AST*);

f64 ast_to_f64(AST*);

//
// parser
//

AST* parse(Lexer*);
AST* parse_expr(Lexer*);
AST* parse_factor(Lexer*);

//
// interp
//

typedef struct FunctionSignature FunctionSignature;
struct FunctionSignature {
    // TODO: really not possible to use our own String type here?
    const char *name;

    // maybe we change this to i32 in the future to encode 
    // variadic amount of args with something like -1.
    usize args_count;
};

extern const FunctionSignature BUILTIN_FUNCTIONS[];
extern const usize BUILTIN_FUNCTIONS_COUNT;

extern const char *BUILTIN_CONSTANTS[];
extern const usize BUILTIN_CONSTANTS_COUNT;

typedef struct  {
    String name;
    AST *value;
} Variable;

typedef struct {
    Allocator *allocator;

    // TODO: maybe put the variables on the heap
    Variable variables[MAX_VARIABLES];
    usize variables_count;
} Interp;

AST *interp(Interp*, AST*);
AST *interp_binop_pow(Interp*, AST*, AST*);

bool ast_match(AST*, AST*);
bool ast_match_type(AST*, AST*);

String _ast_to_string(Allocator*, AST*, u8);
#define ast_to_string(allocator, node) _ast_to_string(allocator, node, 0)
String ast_to_debug_string(Allocator*, AST*);

//
// gui
//

void init_gui();

//
// general
//

#define panic(msg) fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, msg); exit(1);
#define todo() panic("not implemented");
#define mark(i) printf("... MARK %d\n", i); 

struct Arena {
    void *memory;
    Arena *prev;

    usize offset;

    // TODO: when arena size is always the same we dont need this one
    usize size;
};

struct Allocator {
    Arena *arena; 
};

Allocator init_allocator();
void *alloc(Allocator *allocator, usize size);
void free_allocator(Allocator *allocator);
