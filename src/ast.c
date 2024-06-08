#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#include "casc.h"

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
        case AST_REAL: return "Real";
        case AST_SYMBOL: return "Symbol";
        case AST_BINOP: return "BinOp";
        case AST_UNARYOP: return "UnaryOp";
        case AST_CALL: return "FuncCall";
        case AST_EMPTY: return "Empty";
        case AST_TYPE_COUNT: assert(false);
    }
}

u8 op_type_precedence(OpType type) {
    switch (type) {
        case OP_POW:
            return 4;
        case OP_UADD:
        case OP_USUB:
            return 3;
        case OP_MUL:
        case OP_DIV:
            return 2;
        case OP_ADD:
        case OP_SUB:
            return 1;
        default:
            return 0;
    }
}

void ast_array_append(Arena *arena, ASTArray *array, AST *node) {
    array->data = arena_alloc(arena, sizeof(AST));
    array->data[array->size] = node;
    array->size++;
};

AST* _create_ast_integer(Arena* arena, i64 value) {
    AST* node = arena_alloc(arena, sizeof(AST));
    node->type = AST_INTEGER;
    node->integer.value = value;
    return node;
}

AST* _create_ast_real(Arena* arena, f64 value) {
    AST* node = arena_alloc(arena, sizeof(AST));
    node->type = AST_REAL;
    node->real.value = value;
    return node;
}

AST* _create_ast_symbol(Arena* arena, char *name) {
    AST *node = arena_alloc(arena, sizeof(AST));
    node->type = AST_SYMBOL;
    node->symbol.name = arena_alloc(arena, strlen(name)+1);
    strcpy(node->symbol.name, name);
    assert(strlen(node->symbol.name) == strlen(name));
    return node;
}

AST* _create_ast_binop(Arena* arena, AST* left, AST* right, OpType type) {
    AST *node = arena_alloc(arena, sizeof(AST));
    node->type = AST_BINOP;
    node->binop.left = left;
    node->binop.right = right;
    node->binop.type = type;
    return node;
}

AST* create_ast_unaryop(Arena* arena, AST* expr, OpType type) {
    AST* node = arena_alloc(arena, sizeof(AST));
    node->type = AST_UNARYOP;
    node->unaryop.expr = expr;
    node->unaryop.type = type;
    return node;
}

AST* create_ast_call(Arena* arena, char *name, ASTArray args) {
    AST *node = arena_alloc(arena, sizeof(AST));
    node->type = AST_CALL;
    strcpy(node->func_call.name, name);
    node->func_call.args = args;
    return node;
}

AST* create_ast_empty(Arena* arena) {
    AST* node = arena_alloc(arena, sizeof(AST));
    node->type = AST_EMPTY;
    node->empty = true;
    return node;
}

void _ast_to_flat_array(Arena* arena, AST* ast, ASTArray* array) {

    switch (ast->type) {
        case AST_INTEGER:
        case AST_SYMBOL:
            ast_array_append(arena, array, ast);
            break;
        case AST_BINOP:
            ast_array_append(arena, array, ast);
            _ast_to_flat_array(arena, ast->binop.left, array);
            _ast_to_flat_array(arena, ast->binop.right, array);
            break;
        case AST_CALL:
            ast_array_append(arena, array, ast);
            for (usize i = 0; i < ast->func_call.args.size; i++) {
                _ast_to_flat_array(arena, ast->func_call.args.data[i], array);
            }
            break;
        default: 
            printf("type %s not implemented\n", ast_type_to_debug_string(ast->type));
            assert(false);
    }

}

ASTArray ast_to_flat_array(Arena* arena, AST* ast) {
    ASTArray array = {0};

    _ast_to_flat_array(arena, ast, &array);
    return array;
}

bool ast_contains(AST *node, AST *target) {
    if (ast_match(node, target)) {
        return true;
    } else {
        switch (node->type) {
            case AST_INTEGER:
                return false;
            case AST_BINOP:
                return ast_contains(node->binop.left, target) || ast_contains(node->binop.right, target);
            default:
                printf("type '%s' not implemented\n", ast_type_to_debug_string(node->type));
                assert(false);
        }
    }
};

f64 ast_to_f64(AST *node) {
    switch (node->type) {
        case AST_INTEGER: return (f64) node->integer.value;
        case AST_REAL:    return node->real.value;
        default:          panic("not allowed");
    }
}

bool ast_is_numeric(AST* node) {
    switch (node->type) {
        case AST_INTEGER:
        case AST_REAL:
            return true;
        default:
            return false;
    }
}
