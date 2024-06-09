#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>

#include "casc.h"

// forward declarations
AST *interp_binop_div(Interp*, AST*);
AST *interp_binop(Interp*, AST*);

bool ast_match(AST* left, AST* right) {

    if (left->type == right->type) {
        switch (left->type) {
            case AST_INTEGER:
                return left->integer.value == right->integer.value;
            case AST_SYMBOL:
                return !strcmp(left->symbol.name, right->symbol.name);
            case AST_BINOP:
                return ast_match(left->binop.left, right->binop.left) && ast_match(left->binop.right, right->binop.right);
            case AST_CALL: {
                if (
                    !strcmp(left->func_call.name, right->func_call.name) &&
                    left->func_call.args.size == right->func_call.args.size
                ) {
                    for (usize i = 0; i < left->func_call.args.size; i++) {
                        if (!ast_match(left->func_call.args.data[i], right->func_call.args.data[i])) {
                            return false;
                        }
                    }
                    return true;
                }
                return false;
            }
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
        default: {
            printf("ERROR: In ast_match_type, node type %s not implemented.\n", ast_type_to_debug_string(left->type));
            exit(1);
            break;
        }
    }

    return false;
}

char *ast_to_debug_string(Arena *arena, AST* node) {
    char *output = arena_alloc(arena, 1024);
    switch (node->type) {
        case AST_INTEGER: sprintf(output, "%s(%lld)", ast_type_to_debug_string(node->type), node->integer.value); break;
        case AST_REAL: sprintf(output, "%s(%f)", ast_type_to_debug_string(node->type), node->real.value); break;
        case AST_SYMBOL: sprintf(output, "%s(%s)", ast_type_to_debug_string(node->type), node->symbol.name); break;
        case AST_BINOP: sprintf(output, "%s(%s, %s)", op_type_to_debug_string(node->binop.type), ast_to_debug_string(arena, node->binop.left), ast_to_debug_string(arena, node->binop.right)); break;
        case AST_UNARYOP: sprintf(output, "%s(%s)", op_type_to_debug_string(node->unaryop.type), ast_to_debug_string(arena, node->unaryop.expr)); break;
        // args to debug string @todo
        case AST_CALL: {
            if (node->func_call.args.size == 1) {
                sprintf(output, "FuncCall(%s, %s)", node->func_call.name, ast_to_debug_string(arena, node->func_call.args.data[0])); break;
            } else {
                // multiple args to string @todo
                sprintf(output, "FuncCall(%s, args)", node->func_call.name); break;
            }
        }
        case AST_EMPTY: sprintf(output, "Empty()"); break;
        default: fprintf(stderr, "ERROR: Cannot do 'ast_to_debug_string' because node type '%s' is not implemented.\n", ast_type_to_debug_string(node->type)); exit(1);
    }
    return output;
}

char *_ast_to_string(Arena *arena, AST* node, uint8_t op_precedence) {
    char *output = arena_alloc(arena, 1024);
    switch (node->type) {
        case AST_INTEGER: sprintf(output, "%lld", node->integer.value); break;
        case AST_REAL: sprintf(output, "%f", node->real.value); break;
        case AST_SYMBOL:
            sprintf(output, "%s", node->symbol.name); break;
        case AST_BINOP: {

            uint8_t current_op_precedence = op_type_precedence(node->binop.type);

            char *left_string = _ast_to_string(arena, node->binop.left, current_op_precedence);
            char *right_string = _ast_to_string(arena, node->binop.right, current_op_precedence);
            const char *op_type_string = op_type_to_string(node->binop.type);

            if (current_op_precedence < op_precedence) {
                sprintf(output, "(%s%s%s)", left_string, op_type_string, right_string);
            } else {
                sprintf(output, "%s%s%s", left_string, op_type_string, right_string);
            }

            break;
        }
        case AST_UNARYOP: {
            uint8_t current_op_precedence = op_type_precedence(node->binop.type);

            char *expr_string = _ast_to_string(arena, node->unaryop.expr, op_precedence);
            const char *op_type_string = op_type_to_string(node->unaryop.type);

            if (current_op_precedence < op_precedence) {
                sprintf(output, "(%s%s)", op_type_string, expr_string); break;
            } else {
                sprintf(output, "%s%s", op_type_string, expr_string); break;
            }
            
        }
        case AST_CALL: {
            if (node->func_call.args.size == 1) {
                sprintf(output, "%s(%s)", node->func_call.name, _ast_to_string(arena, node->func_call.args.data[0], op_precedence)); break;
            } else {
                // multiple args to string @todo
                sprintf(output, "%s(args)", node->func_call.name); break;
            }
        }
        case AST_EMPTY: break;
        default: fprintf(stderr, "ERROR: Cannot do 'ast_to_string' because node type '%s' is not implemented.\n", ast_type_to_debug_string(node->type)); exit(1);
    }
    return output;
}

AST* interp_binop_add(Interp *ip, AST* node) {
    AST* left = interp(ip, node->binop.left);
    AST* right = interp(ip, node->binop.right);

    if (ast_is_numeric(left) && ast_is_numeric(right)) {
        return interp(ip, REAL(ast_to_f64(left)+ast_to_f64(right)));
    } else if (ast_match(left, INTEGER(0))) {
        return right;
    } else if (ast_match(right, INTEGER(0))) {
        return left;
    } else if (ast_match(left, right)) {
        return MUL(INTEGER(2), left);
    } else if (ast_match_type(left, DIV(INTEGER(1), INTEGER(1))) && ast_match_type(right, DIV(INTEGER(1), INTEGER(1)))) {
        // a/b + c/d
        i64 a = left->binop.left->integer.value;
        i64 b = left->binop.right->integer.value;
        i64 c = right->binop.left->integer.value;
        i64 d = right->binop.right->integer.value;
        AST* r = DIV(INTEGER(a*d+c*b), INTEGER(b*d));
        return interp_binop_div(ip, r);
    } else if (left->type == AST_INTEGER && right->type == AST_BINOP && right->binop.type == OP_DIV) {
        // c + a/b
        // -> (cb)/b + a/b
        AST* r = ADD(DIV(MUL(left, right->binop.right), right->binop.right), right);
        return interp_binop(ip, r);
    } else if (left->type == AST_BINOP && left->binop.type == OP_MUL) {
        if (ast_match(left->binop.right, right)) {
            AST* new_left = interp_binop(ip, ADD(INTEGER(1), left->binop.left));
            return MUL(new_left, right);
        }
    }
    return ADD(left, right);
}

AST* interp_binop_sub(Interp *ip, AST* node) {
    AST* left = interp(ip, node->binop.left);
    AST* right = interp(ip, node->binop.right);

    if (ast_is_numeric(left) && ast_is_numeric(right)) {
        return interp(ip, REAL(ast_to_f64(left)-ast_to_f64(right)));
    } else if (ast_match_type(left, INTEGER(1)) && ast_match_type(right, DIV(INTEGER(1), INTEGER(1)))) {
        // a - b/c -> ac/c - b/c = (ac-b)/c
        AST* a = left;
        AST* b = right->binop.left;
        AST* c = right->binop.right;
        return interp_binop_div(ip,
            DIV(SUB(MUL(a, c), b), c)
        );
    } else if (ast_match_type(left, DIV(INTEGER(1), INTEGER(1))) && ast_match_type(right, INTEGER(1))) {
        // a/b - c -> a/b - cb/b = (a-cb)/b
        AST *a = left->binop.left;
        AST *b = left->binop.right;
        AST *c = right;
        AST *cb = MUL(c, b);
        AST *numerator = SUB(a, cb);
        return interp_binop_div(ip, DIV(numerator, b));
    } else if (ast_match(right, INTEGER(0))) {
        return left;
    }

    return node;
}

AST* interp_binop_mul(Interp *ip, AST* node) {
    AST* left = interp(ip, node->binop.left);
    AST* right = interp(ip, node->binop.right);

    if (ast_is_numeric(left) && ast_is_numeric(right)) {
        return interp(ip, REAL(ast_to_f64(left)*ast_to_f64(right)));
    } else if (ast_match(left, INTEGER(0)) || ast_match(INTEGER(0), right)) {
        return INTEGER(0);
    } else if (ast_match(left, right)) {
        return interp_binop_pow(ip, POW(left, INTEGER(2)));
    } else if (ast_match_type(left, DIV(INTEGER(1), INTEGER(1))) && ast_match_type(right, DIV(INTEGER(1), INTEGER(1)))) {
        // a/b * c/d -> ac/bd
        AST *a = left->binop.left;
        AST *b = left->binop.right;
        AST *c = right->binop.left;
        AST *d = right->binop.right;
        AST *ac = MUL(a, c);
        AST *bd = MUL(b, d);
        return interp_binop_div(ip, DIV(ac, bd));
    } else if (ast_match(left, INTEGER(1)) || ast_match(right, INTEGER(1))) {
        if (ast_match(left, INTEGER(1))) {
            return right;
        } else {
            return left;
        }
    }
    return MUL(left, right);
}

AST* interp_binop_div(Interp *ip, AST* node) {
    AST* left = interp(ip, node->binop.left);
    AST* right = interp(ip, node->binop.right);
    if (left->type == AST_INTEGER && right->type == AST_INTEGER) {
        if (left->integer.value % right->integer.value == 0) {
            return INTEGER(left->integer.value / right->integer.value);
        } else {
            return DIV(left, right);
        }
    } else if (ast_is_numeric(left) && ast_is_numeric(right)) {
        f64 l = ast_to_f64(left);
        f64 r = ast_to_f64(right);
        return interp(ip, REAL(l/r));
    } else if (left->type == AST_INTEGER && right->type == AST_BINOP && right->binop.type == OP_DIV) {
        AST* r = DIV(MUL(left, right->binop.left), right->binop.right);
        return interp_binop(ip, r);
    }
    return DIV(left, right);
}

AST *interp_binop_pow(Interp *ip, AST *node) {
    AST* left = interp(ip, node->binop.left);
    AST* right = interp(ip, node->binop.right);
    if (left->type == AST_INTEGER && right->type == AST_INTEGER) {
        long double l = (long double)left->integer.value;
        long double r = (long double)right->integer.value;
        int64_t result = (int64_t)powl(l, r);
        return INTEGER(result);
    } else if (ast_match(right, INTEGER(0))) {
        return INTEGER(1);
    } else if (ast_match(right, INTEGER(1))) {
        return left;
    }
    return POW(left, right);
}

AST* interp_binop(Interp *ip, AST* node) {
    switch (node->binop.type) {
        case OP_ADD: return interp_binop_add(ip, node);
        case OP_SUB: return interp_binop_sub(ip, node);
        case OP_MUL: return interp_binop_mul(ip, node);
        case OP_DIV: return interp_binop_div(ip, node);
        case OP_POW: return interp_binop_pow(ip, node);
        default: assert(false);
    }
}

AST* interp_unaryop(Interp *ip, AST* node) {
    AST* expr = interp(ip, node->unaryop.expr);
    OpType op_type = node->unaryop.type;

    if (expr->type == AST_INTEGER) {
        switch (op_type) {
            case OP_UADD: return INTEGER(+expr->integer.value);
            case OP_USUB: return INTEGER(-expr->integer.value);
            default: assert(false);
        }
    }

    return init_ast_unaryop(ip->arena, expr, op_type);
}

AST* diff(Interp *ip, AST *expr, AST *var) {
    switch (expr->type) {
        case AST_SYMBOL: {
            if (ast_match(expr, var)) {
                return INTEGER(1);
            } else {
                return INTEGER(0);
            }
        }
        case AST_BINOP: {
            switch (expr->binop.type) {
                case OP_POW: {
                    assert(ast_contains(expr->binop.left, var)); // check if base is function of 'var'
                    assert(!ast_contains(expr->binop.right, var)); // exponent must be constant for now
                    
                    AST* b = expr->binop.left;
                    AST* n = expr->binop.right;
                    // x^n -> n * x^(n-1) * x'
                    AST* result = MUL(MUL(n, POW(b, SUB(n, INTEGER(1)))), diff(ip, b, var));
                    return interp_binop_mul(ip, result);
                }
                case OP_ADD: {
                    // a + b -> a' + b'
                    AST *dl = diff(ip, expr->binop.left, var);
                    AST *dr = diff(ip, expr->binop.right, var);
                    AST *result = ADD(dl, dr);
                    return interp_binop_add(ip, result);
                }
                default:
                    printf("op '%s' not implemented\n", op_type_to_debug_string(expr->binop.type));
                    assert(false);
            }
        }
        default:
            printf("type '%s' not implemented\n", ast_type_to_debug_string(expr->type));
            assert(false);
    }
}

AST *interp_sin(Interp *ip, AST* node) {
    assert(node->func_call.args.size == 1);
    AST *arg = node->func_call.args.data[0];

    if (ast_match(arg, SYMBOL("pi"))) {
        return INTEGER(0);
    } else if (arg->type == AST_INTEGER || arg->type == AST_REAL) {
        f64 value = ast_to_f64(arg);
        return interp(ip, REAL(sin(value)));
    }
    
    return node;
}

AST *interp_cos(Interp *ip, AST* node) {
    assert(node->func_call.args.size == 1);
    AST *arg = node->func_call.args.data[0];
    
    if (ast_match(arg, INTEGER(0))) {
        return INTEGER(1);
    } else if (ast_match(arg, DIV(SYMBOL("pi"), INTEGER(2)))) {
        return INTEGER(0);
    } else if (ast_match(arg, SYMBOL("pi"))) {
        return INTEGER(-1);
    } else if (arg->type == AST_INTEGER || arg->type == AST_REAL) {
        f64 value = ast_to_f64(arg);
        return interp(ip, REAL(cos(value)));
    }

    return node;
}

AST* interp_call(Interp *ip, AST* node) {
    // we need a proper system for checking and using args (function definitions and function signatures) @ƒeature

    char *func_name = node->func_call.name;

    for (usize i = 0; i < node->func_call.args.size; i++) {
        node->func_call.args.data[i] = interp(ip, node->func_call.args.data[i]);
    }

    ASTArray args = node->func_call.args;

    if (!strcmp(func_name, "sqrt") && args.data[0]->type == AST_INTEGER) {
        double result = sqrt((double)args.data[0]->integer.value);

        if (fmod((double)args.data[0]->integer.value, result) == 0.0) {
            return INTEGER((i64)result);
        }

        // check for perfect square
        // @Speed: this is probably an inefficient way to compute the biggest perfect sqaure factor of a given number
        for (i32 q = args.data[0]->integer.value; q > 1; q--) {
            double sqrt_of_q = sqrtf((double)q);
            bool is_perfect_square = sqrt_of_q == floor(sqrt_of_q);
            if (is_perfect_square && args.data[0]->integer.value % q == 0) {
                i32 p = args.data[0]->integer.value / q;
                ASTArray new_args = {0};
                ast_array_append(ip->arena, &new_args, INTEGER(p));
                return MUL(INTEGER((i64)sqrt_of_q), CALL(func_name, new_args));
            }
        }
    } else if (!strcmp(func_name, "ln")) {
        if (ast_match(args.data[0], SYMBOL("e"))) {
            return INTEGER(1);
        } else if (ast_match(args.data[0], INTEGER(1))) {
            return INTEGER(0);
        }
    } else if (!strcmp(func_name, "log")) {
        if (node->func_call.args.size == 2) {
            // log_b(y) = x
            // b^x = y
            AST* y = node->func_call.args.data[0];
            AST* b = node->func_call.args.data[1];
            if (ast_match(y, INTEGER(1))) {
                return INTEGER(0);
            } else if (ast_match(y, b)) {
                return INTEGER(1);
            }
        } else {
            assert(false);
        }
    } else if (!strcmp(func_name, "sin")) {
        return interp_sin(ip, node);
    } else if (!strcmp(func_name, "cos")) {
        return interp_cos(ip, node);
    } else if (!strcmp(func_name, "diff")) {
        AST* diff_var;

        if (node->func_call.args.size == 1) {
            ASTArray nodes = ast_to_flat_array(ip->arena, node);
            bool symbol_seen = false;
            AST *symbol;
            for (usize i = 0; i < nodes.size; i++) {
                if (nodes.data[i]->type == AST_SYMBOL) {
                    if (symbol_seen) {
                        assert(ast_match(symbol, nodes.data[i]));
                    } else {
                        symbol_seen = true;
                        symbol = nodes.data[i];
                    }
                }
            }
            assert(symbol_seen);
            diff_var = symbol;
        } else if (node->func_call.args.size == 2) {
            assert(node->func_call.args.data[1]->type == AST_SYMBOL);
            diff_var = node->func_call.args.data[1];
        } else {
            assert(false);
        }

        return diff(ip, node->func_call.args.data[0], diff_var);
    }

    return node;
}

AST* interp(Interp *ip, AST* node) {
    switch (node->type) {
        case AST_BINOP:
            return interp_binop(ip, node);
        case AST_UNARYOP:
            return interp_unaryop(ip, node);
        case AST_INTEGER:
        case AST_SYMBOL:
            return node;
        case AST_REAL: {
            f64 value = node->real.value;
            if (value - floor(value) == 0.0) {
                return INTEGER((i64)value);
            }
            return node;
        }
        case AST_CALL:
            return interp_call(ip, node);
        case AST_EMPTY:
            return node;
        default:
            fprintf(stderr, "%s:%d: Cannot interp node type '%s'\n", __FILE__, __LINE__, ast_type_to_debug_string(node->type));
            exit(1);
    }
}
