#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include "casc.h"

// TODO: return string
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

// TODO: return string
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

// TODO: return string
const char* ast_type_to_debug_string(ASTType type) {
    switch (type) {
        case AST_PROGRAM: return "Program";
        case AST_ASSIGN: return "Assign";
        case AST_INTEGER: return "Integer";
        case AST_REAL: return "Real";
        case AST_SYMBOL: return "Symbol";
        case AST_CONSTANT: return "Constant";
        case AST_BINOP: return "BinOp";
        case AST_UNARYOP: return "UnaryOp";
        case AST_CALL: return "FuncCall";
        case AST_EMPTY: return "Empty";
        case AST_LIST: return "List";
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

ASTArray init_ast_array_with_capacity(Allocator *allocator, usize capacity) {
    ASTArray a = {0};
    a.capacity = capacity;

    // TODO: use capacity here
    (void) allocator;

    return a;
}

void ast_array_append(Allocator *allocator, ASTArray *array, AST *node) {
    // TODO: we dont use the capacity field right now!

    AST **new_data = alloc(allocator, sizeof(AST)*(array->size+1));

    // currently we move old values to new_data because
    // there is no real realloc for arena @cleanup
    for (usize i = 0; i < array->size; i++) {
        new_data[i] = array->data[i];
    }
    new_data[array->size] = node;

    array->data = new_data;
    array->size++;
};

AST* init_ast_integer(Allocator* allocator, i64 value) {
    AST* node = alloc(allocator, sizeof(AST));
    node->type = AST_INTEGER;
    node->integer.value = value;
    return node;
}

AST* init_ast_real(Allocator* allocator, f64 value) {
    AST* node = alloc(allocator, sizeof(AST));
    node->type = AST_REAL;
    node->real.value = value;
    return node;
}

AST* init_ast_symbol(Allocator* allocator, String name) {
    AST *node = alloc(allocator, sizeof(AST));
    node->type = AST_SYMBOL;
    node->symbol.name = name;
    return node;
}

AST* init_ast_constant(Allocator* allocator, String name) {
    AST *node = alloc(allocator, sizeof(AST));
    node->type = AST_CONSTANT;
    node->constant.name = name;
    return node;
}

AST* init_ast_binop(Allocator* allocator, AST* left, AST* right, OpType op) {
    AST *node = alloc(allocator, sizeof(AST));
    node->type = AST_BINOP;
    node->binop.left = left;
    node->binop.right = right;
    node->binop.op = op;
    return node;
}

AST* init_ast_unaryop(Allocator* allocator, AST* operand, OpType op) {
    AST *node = alloc(allocator, sizeof(AST));
    node->type = AST_UNARYOP;
    node->unaryop.operand = operand;
    node->unaryop.op = op;
    return node;
}

AST* init_ast_call(Allocator* allocator, String name, ASTArray args) {
    AST *node = alloc(allocator, sizeof(AST));
    node->type = AST_CALL;
    node->func_call.name = name;
    node->func_call.args = args;
    return node;
}

AST* init_ast_program(Allocator* allocator) {
    AST *node = alloc(allocator, sizeof(AST));
    node->type = AST_PROGRAM;
    node->program.statements.data = NULL;
    node->program.statements.size = 0;
    return node;
}

AST* init_ast_assign(Allocator* allocator, AST *target, AST *value) {
    AST *node = alloc(allocator, sizeof(AST));
    node->type = AST_ASSIGN;
    node->assign.target = target;
    node->assign.value = value;
    return node;
}

AST* init_ast_empty(Allocator* allocator) {
    AST* node = alloc(allocator, sizeof(AST));
    node->type = AST_EMPTY;
    node->empty = true;
    return node;
}

AST *init_ast_list(Allocator *allocator, usize capacity) {
    AST *node = alloc(allocator, sizeof(AST));
    node->type = AST_LIST;
    node->list.nodes = init_ast_array_with_capacity(allocator, capacity);
    return node;
}

void _ast_to_flat_array(Allocator* allocator, AST* ast, ASTArray* array) {

    switch (ast->type) {
        case AST_INTEGER:
        case AST_SYMBOL:
            ast_array_append(allocator, array, ast);
            break;
        case AST_BINOP:
            ast_array_append(allocator, array, ast);
            _ast_to_flat_array(allocator, ast->binop.left, array);
            _ast_to_flat_array(allocator, ast->binop.right, array);
            break;
        case AST_CALL:
            ast_array_append(allocator, array, ast);
            for (usize i = 0; i < ast->func_call.args.size; i++) {
                _ast_to_flat_array(allocator, ast->func_call.args.data[i], array);
            }
            break;
        default: 
            printf("type %s not implemented\n", ast_type_to_debug_string(ast->type));
            assert(false);
    }

}

ASTArray ast_to_flat_array(Allocator* allocator, AST* ast) {
    ASTArray array = {0};

    _ast_to_flat_array(allocator, ast, &array);
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
        
        case AST_INTEGER:
            return (f64) node->integer.value;
        
        case AST_REAL:
            return node->real.value;
        
        case AST_BINOP: {
            assert(ast_is_numeric(node));
            assert(node->binop.left->type == AST_INTEGER);
            assert(node->binop.right->type == AST_INTEGER);
            return (f64) node->binop.left->integer.value / (f64) node->binop.right->integer.value;
        }

        case AST_CONSTANT: {
            if (string_eq(node->constant.name, init_string("e"))) {
                return M_E;
            } else if (string_eq(node->constant.name, init_string("pi"))) {
                return M_PI;
            } else {
                todo()
            }
        }

        default: 
            panic("not allowed");
    }
}

inline bool ast_is_fraction(AST* node) {
    if (node->type == AST_BINOP) {
        if (node->binop.left->type == AST_INTEGER && node->binop.right->type == AST_INTEGER) {
            return true;
        }
    }

    return false;
}

bool ast_is_numeric(AST* node) {
    if (node->type == AST_INTEGER || node->type == AST_REAL) {
        return true;
    }

    if (node->type == AST_BINOP) {
        if (node->binop.op == OP_DIV) {
            if (ast_is_numeric(node->binop.left) && ast_is_numeric(node->binop.right)) {
                return true;
            }
        }
    }

    if (node->type == AST_CONSTANT) {
        return true;
    }

    return false;
}

void list_append(Allocator *allocator, AST *list, AST *node) {
    ast_array_append(allocator, &list->list.nodes, node);
}
