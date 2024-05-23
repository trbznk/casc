#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>
#include <string.h>

#include "parser.h"
#include "interp.h"

bool ast_match(AST* left, AST* right) {
    if (left->type == right->type) {
        switch (left->type) {
            case AST_INTEGER:
                return left->integer.value == right->integer.value;
            case AST_SYMBOL:
                return !strcmp(left->symbol.name.text, right->symbol.name.text);
            case AST_BINOP:
                return ast_match(left->binop.left, right->binop.left) && ast_match(left->binop.right, right->binop.right);
            case AST_FUNC_CALL:
                return !strcmp(left->func_call.name.text, right->func_call.name.text) && ast_match(left->func_call.arg, right->func_call.arg);
            default: fprintf(stderr, "ERROR: Cannot do 'ast_match' because node type '%s' is not implemented.\n", ast_type_to_debug_string(left->type)); exit(1);
        }
    }
    return false;
}

bool ast_match_type(AST *left, AST *right) {
    if (left->type != right->type) {
        return false;
    }

    switch (left->type) {
        default: printf("ERROR: node type %s not implemented.\n", ast_type_to_debug_string(left->type)); break;
    }

    return false;
}

char *ast_to_debug_string(AST* node) {
    // TODO: memory management needed here
    char *output = malloc(1024);
    switch (node->type) {
        case AST_INTEGER: sprintf(output, "%s(%lld)", ast_type_to_debug_string(node->type), node->integer.value); break;
        case AST_SYMBOL: sprintf(output, "%s(%s)", ast_type_to_debug_string(node->type), node->symbol.name.text); break;
        case AST_BINOP: sprintf(output, "%s(%s, %s)", op_type_to_debug_string(node->binop.type), ast_to_debug_string(node->binop.left), ast_to_debug_string(node->binop.right)); break;
        case AST_UNARYOP: sprintf(output, "%s(%s)", op_type_to_debug_string(node->unaryop.type), ast_to_debug_string(node->unaryop.expr)); break;
        case AST_FUNC_CALL: sprintf(output, "%s(%s)", node->func_call.name.text, ast_to_debug_string(node->func_call.arg)); break;
        case AST_EMPTY: sprintf(output, "Empty()"); break;
        default: fprintf(stderr, "ERROR: Cannot do 'ast_to_debug_string' because node type '%s' is not implemented.\n", ast_type_to_debug_string(node->type)); exit(1);
    }
    return output;
}

char *ast_to_string(AST* node) {
    // TODO: find a better solution to recursively build a string
    //       same goes for ast_to_debug_string function
    char *output = malloc(1024);
    switch (node->type) {
        case AST_INTEGER: sprintf(output, "%lld", node->integer.value); break;
        case AST_SYMBOL: sprintf(output, "%s", node->symbol.name.text); break;
        case AST_BINOP: {
            sprintf(output, "(%s%s%s)", ast_to_string(node->binop.left), op_type_to_string(node->binop.type), ast_to_string(node->binop.right));
            break;
        }
        case AST_UNARYOP: sprintf(output, "%s%s", op_type_to_string(node->unaryop.type), ast_to_string(node->unaryop.expr)); break;
        case AST_FUNC_CALL: sprintf(output, "%s(%s)", node->func_call.name.text, ast_to_string(node->func_call.arg)); break;
        case AST_EMPTY: break;
        default: fprintf(stderr, "ERROR: Cannot do 'ast_to_string' because node type '%s' is not implemented.\n", ast_type_to_debug_string(node->type)); exit(1);
    }
    return output;
}

AST* interp_binop_add(AST* node) {
    AST* left = interp(node->binop.left);
    AST* right = interp(node->binop.right);
    if (left->type == AST_INTEGER && right->type == AST_INTEGER) {
        return create_ast_integer(left->integer.value + right->integer.value);
    } else if (ast_match(left, right)) {
        return create_ast_binop(create_ast_integer(2), left, OP_MUL);
    } else if (left->type == AST_INTEGER && right->type == AST_BINOP && right->binop.type == OP_DIV) {
        // c + a/b
        // -> (cb)/b + a/b
        AST* r = create_ast_binop(create_ast_binop(create_ast_binop(left, right->binop.right, OP_MUL), right->binop.right, OP_DIV), right, OP_ADD);
        return interp_binop(r);
    } else if (left->type == AST_BINOP && left->binop.type == OP_MUL) {
        if (ast_match(left->binop.right, right)) {
            AST* new_left = interp_binop(create_ast_binop(create_ast_integer(1), left->binop.left, OP_ADD));
            return create_ast_binop(new_left, right, OP_MUL);
        }
    }
    return create_ast_binop(left, right, OP_ADD);
}

AST* interp_binop_sub(AST* node) {
    AST* left = interp(node->binop.left);
    AST* right = interp(node->binop.right);
    if (left->type == AST_INTEGER && right->type == AST_INTEGER) {
        return create_ast_integer(left->integer.value - right->integer.value);
    }
    return create_ast_binop(left, right, OP_SUB);
}

AST* interp_binop_mul(AST* node) {
    AST* left = interp(node->binop.left);
    AST* right = interp(node->binop.right);
    if (left->type == AST_INTEGER && right->type == AST_INTEGER) {
        return create_ast_integer(left->integer.value * right->integer.value);
    }
    return create_ast_binop(left, right, OP_MUL);
}

AST* interp_binop_div(AST* node) {
    AST* left = interp(node->binop.left);
    AST* right = interp(node->binop.right);
    OpType op_type = node->binop.type;
    if (left->type == AST_INTEGER && right->type == AST_INTEGER) {
        if (left->integer.value % right->integer.value == 0) {
            return create_ast_integer(left->integer.value / right->integer.value);
        } else {
            return create_ast_binop(left, right, op_type);
        }
    } else if (left->type == AST_INTEGER && right->type == AST_BINOP && right->binop.type == OP_DIV) {
        AST* r = create_ast_binop(create_ast_binop(left, right->binop.left, OP_MUL), right->binop.right, OP_DIV);
        return interp_binop(r);
    }
    return create_ast_binop(left, right, OP_DIV);
}

AST* interp_binop(AST* node) {
    switch (node->binop.type) {
        case OP_ADD: return interp_binop_add(node);
        case OP_SUB: return interp_binop_sub(node);
        case OP_MUL: return interp_binop_mul(node);
        case OP_DIV: return interp_binop_div(node);
        default: assert(false);
    }
}

AST* interp_unaryop(AST* node) {
    AST* expr = interp(node->unaryop.expr);
    OpType op_type = node->unaryop.type;

    if (expr->type == AST_INTEGER) {
        switch (op_type) {
            case OP_UADD: return create_ast_integer(+expr->integer.value);
            case OP_USUB: return create_ast_integer(-expr->integer.value);
            default: assert(false);
        }
    }

    return create_ast_unaryop(expr, op_type);
}

AST* interp_func_call(AST* node) {
    AST* arg = interp(node->func_call.arg);
    if (arg->type == AST_INTEGER && !strcmp(node->func_call.name.text, "sqrt")) {
        double result = sqrt((double)node->func_call.arg->integer.value);
        if (fmod((double)node->func_call.arg->integer.value, result) == 0.0) {
            return create_ast_integer((int)result);
        }

        // check for perfect square
        // @Speed: this is probably an inefficient way to compute the biggest perfect sqaure factor of a given number
        for (int q = node->func_call.arg->integer.value; q > 1; q--) {
            double sqrt_of_q = sqrtf((double)q);
            bool is_perfect_square = sqrt_of_q == floor(sqrt_of_q);
            if (is_perfect_square && node->func_call.arg->integer.value % q == 0) {
                int p = node->func_call.arg->integer.value / q;
                return create_ast_binop(create_ast_integer((int64_t)sqrt_of_q), create_ast_func_call(node->func_call.name, create_ast_integer(p)), OP_MUL);
            }
        }
    }

    return node;
}

AST* interp(AST* node) {
    // TODO: unpack here. For example interp_binop(node.binop)
    switch (node->type) {
        case AST_BINOP:
            return interp_binop(node);
        case AST_UNARYOP:
            return interp_unaryop(node);
        case AST_INTEGER:
        case AST_SYMBOL:
            return node;
        case AST_FUNC_CALL:
            return interp_func_call(node);
        case AST_EMPTY:
            return node;
        default:
            fprintf(stderr, "ERROR: Cannot interp node type '%s'\n", ast_type_to_debug_string(node->type));
            exit(1);
    }
}

AST* interp_from_string(char* input) {
    Tokens tokens = tokenize(input);
    Parser parser = { .tokens = tokens, .pos=0 };
    AST* ast = parse_expr(&parser);
    AST* output = interp(ast);
    return output;
}
