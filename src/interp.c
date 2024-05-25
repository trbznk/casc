#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>
#include <string.h>

#include "parser.h"
#include "interp.h"

// forward declarations
AST *interp_binop_div(AST*);
AST *interp_binop(AST*);


bool ast_match(AST* left, AST* right) {
    if (left->type == right->type) {
        switch (left->type) {
            case AST_INTEGER:
                return left->integer.value == right->integer.value;
            case AST_SYMBOL:
                return !strcmp(left->symbol.name.text, right->symbol.name.text);
            case AST_CONSTANT:
                return !strcmp(left->constant.name.text, right->constant.name.text);
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
        case AST_INTEGER: return true;
        case AST_BINOP: {
            if (left->binop.type == right->binop.type) {
                AST *ll = left->binop.left;;
                AST *lr = left->binop.right;;
                AST *rl = right->binop.left;;
                AST *rr = right->binop.right;;
                return ast_match_type(ll, rl) && ast_match_type(lr, rr);
            }
            return false;
        }
        default: printf("ERROR: In ast_match_type, node type %s not implemented.\n", ast_type_to_debug_string(left->type)); break;
    }

    return false;
}

char *ast_to_debug_string(AST* node) {
    // TODO: memory management needed here
    char *output = malloc(1024);
    switch (node->type) {
        case AST_INTEGER: sprintf(output, "%s(%lld)", ast_type_to_debug_string(node->type), node->integer.value); break;
        case AST_SYMBOL: sprintf(output, "%s(%s)", ast_type_to_debug_string(node->type), node->symbol.name.text); break;
        case AST_CONSTANT: sprintf(output, "%s(%s)", ast_type_to_debug_string(node->type), node->constant.name.text); break;
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
        case AST_SYMBOL:
        case AST_CONSTANT:
            sprintf(output, "%s", node->symbol.name.text); break;
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
    } else if (ast_match_type(left, parse_from_string("1/1")) && ast_match_type(right, parse_from_string("1/1"))) {
        // a/b + c/d
        int64_t a = left->binop.left->integer.value;
        int64_t b = left->binop.right->integer.value;
        int64_t c = right->binop.left->integer.value;
        int64_t d = right->binop.right->integer.value;
        AST* r = create_ast_binop(create_ast_integer(a*d+c*b), create_ast_integer(b*d), OP_DIV);
        return interp_binop_div(r);
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
    } else if (ast_match_type(left, parse_from_string("1")) && ast_match_type(right, parse_from_string("1/1"))) {
        // a - b/c -> ac/c - b/c = (ac-b)/c
        AST* a = left;
        AST* b = right->binop.left;
        AST* c = right->binop.right;
        return interp_binop_div(
            create_ast_binop( 
                create_ast_binop(
                    create_ast_binop(a, c, OP_MUL),
                    b,
                    OP_SUB
                ),
                c,
                OP_DIV
            )
        );
    } else if (ast_match_type(left, parse_from_string("1/1")) && ast_match_type(right, parse_from_string("1"))) {
        // a/b - c -> a/b - cb/b = (a-cb)/b
        AST *a = left->binop.left;
        AST *b = left->binop.right;
        AST *c = right;
        AST *cb = create_ast_binop(c, b, OP_MUL);
        AST *numerator = create_ast_binop(a, cb, OP_SUB);
        return interp_binop_div(create_ast_binop(numerator, b, OP_DIV));
    }
    return create_ast_binop(left, right, OP_SUB);
}

AST* interp_binop_mul(AST* node) {
    AST* left = interp(node->binop.left);
    AST* right = interp(node->binop.right);
    if (left->type == AST_INTEGER && right->type == AST_INTEGER) {
        return create_ast_integer(left->integer.value * right->integer.value);
    } else if (ast_match(left, create_ast_integer(0)) || ast_match(create_ast_integer(0), right)) {
        return create_ast_integer(0);
    } else if (ast_match(left, right)) {
        return interp_binop_pow(create_ast_binop(left, create_ast_integer(2), OP_POW));
    } else if (ast_match_type(left, parse_from_string("1/1")) && ast_match_type(right, parse_from_string("1/1"))) {
        // a/b * c/d -> ac/bd
        AST *a = left->binop.left;
        AST *b = left->binop.right;
        AST *c = right->binop.left;
        AST *d = right->binop.right;
        AST *ac = create_ast_binop(a, c, OP_MUL);
        AST *bd = create_ast_binop(b, d, OP_MUL);
        return interp_binop_div(
            create_ast_binop(ac, bd, OP_DIV)
        );
    }
    return create_ast_binop(left, right, OP_MUL);
}

AST* interp_binop_div(AST* node) {
    AST* left = interp(node->binop.left);
    AST* right = interp(node->binop.right);
    if (left->type == AST_INTEGER && right->type == AST_INTEGER) {
        if (left->integer.value % right->integer.value == 0) {
            return create_ast_integer(left->integer.value / right->integer.value);
        } else {
            return create_ast_binop(left, right, OP_DIV);
        }
    } else if (left->type == AST_INTEGER && right->type == AST_BINOP && right->binop.type == OP_DIV) {
        AST* r = create_ast_binop(create_ast_binop(left, right->binop.left, OP_MUL), right->binop.right, OP_DIV);
        return interp_binop(r);
    }
    return create_ast_binop(left, right, OP_DIV);
}

AST *interp_binop_pow(AST *node) {
    AST* left = interp(node->binop.left);
    AST* right = interp(node->binop.right);
    if (left->type == AST_INTEGER && right->type == AST_INTEGER) {
        long double l = (long double)left->integer.value;
        long double r = (long double)right->integer.value;
        int64_t result = (int64_t)powl(l, r);
        return create_ast_integer(result);
    } else if (ast_match(right, create_ast_integer(0))) {
        return create_ast_integer(1);
    } else if (ast_match(right, create_ast_integer(1))) {
        return left;
    }
    return create_ast_binop(left, right, OP_POW);
}

AST* interp_binop(AST* node) {
    switch (node->binop.type) {
        case OP_ADD: return interp_binop_add(node);
        case OP_SUB: return interp_binop_sub(node);
        case OP_MUL: return interp_binop_mul(node);
        case OP_DIV: return interp_binop_div(node);
        case OP_POW: return interp_binop_pow(node);
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
    Token name = node->func_call.name;
    AST* arg = interp(node->func_call.arg);
    if (!strcmp(name.text, "sqrt") && arg->type == AST_INTEGER) {
        double result = sqrt((double)arg->integer.value);
        if (fmod((double)arg->integer.value, result) == 0.0) {
            return create_ast_integer((int)result);
        }

        // check for perfect square
        // @Speed: this is probably an inefficient way to compute the biggest perfect sqaure factor of a given number
        for (int q = arg->integer.value; q > 1; q--) {
            double sqrt_of_q = sqrtf((double)q);
            bool is_perfect_square = sqrt_of_q == floor(sqrt_of_q);
            if (is_perfect_square && arg->integer.value % q == 0) {
                int p = arg->integer.value / q;
                return create_ast_binop(create_ast_integer((int64_t)sqrt_of_q), create_ast_func_call(name, create_ast_integer(p)), OP_MUL);
            }
        }
    } else if (ast_match(node, parse_from_string("sin(pi)"))) {
        return create_ast_integer(0);
    } else if (ast_match(node, parse_from_string("cos(0)"))) {
        return create_ast_integer(1);
    } else if (ast_match(node, parse_from_string("cos(pi/2)"))) {
        return create_ast_integer(0);
    } else if (ast_match(node, parse_from_string("cos(pi)"))) {
        return create_ast_integer(-1);
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
        case AST_CONSTANT:
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
    AST* ast = parse(&parser);
    AST* output = interp(ast);
    return output;
}
