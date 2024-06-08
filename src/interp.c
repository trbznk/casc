#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>

#include "casc.h"

// forward declarations
AST *interp_binop_div(Worker*, AST*);
AST *interp_binop(Worker*, AST*);

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

AST* interp_binop_add(Worker *worker, AST* node) {
    AST* left = interp(worker, node->binop.left);
    AST* right = interp(worker, node->binop.right);

    if (ast_is_numeric(left) && ast_is_numeric(right)) {
        return interp(worker, REAL(ast_to_f64(left)+ast_to_f64(right)));
    } else if (ast_match(left, create_ast_integer(0))) {
        return right;
    } else if (ast_match(right, create_ast_integer(0))) {
        return left;
    } else if (ast_match(left, right)) {
        return _create_ast_binop(&worker->arena, _create_ast_integer(&worker->arena, 2), left, OP_MUL);
    } else if (ast_match_type(left, _create_ast_binop(&worker->arena, _create_ast_integer(&worker->arena, 1), _create_ast_integer(&worker->arena, 1), OP_DIV)) && ast_match_type(right, _create_ast_binop(&worker->arena, _create_ast_integer(&worker->arena, 1), _create_ast_integer(&worker->arena, 1), OP_DIV))) {
        // a/b + c/d
        i64 a = left->binop.left->integer.value;
        i64 b = left->binop.right->integer.value;
        i64 c = right->binop.left->integer.value;
        i64 d = right->binop.right->integer.value;
        AST* r = _create_ast_binop(&worker->arena, _create_ast_integer(&worker->arena, a*d+c*b), _create_ast_integer(&worker->arena, b*d), OP_DIV);
        return interp_binop_div(worker, r);
    } else if (left->type == AST_INTEGER && right->type == AST_BINOP && right->binop.type == OP_DIV) {
        // c + a/b
        // -> (cb)/b + a/b
        AST* r = _create_ast_binop(&worker->arena, _create_ast_binop(&worker->arena, _create_ast_binop(&worker->arena, left, right->binop.right, OP_MUL), right->binop.right, OP_DIV), right, OP_ADD);
        return interp_binop(worker, r);
    } else if (left->type == AST_BINOP && left->binop.type == OP_MUL) {
        if (ast_match(left->binop.right, right)) {
            AST* new_left = interp_binop(worker, _create_ast_binop(&worker->arena, _create_ast_integer(&worker->arena, 1), left->binop.left, OP_ADD));
            return _create_ast_binop(&worker->arena, new_left, right, OP_MUL);
        }
    }
    return _create_ast_binop(&worker->arena, left, right, OP_ADD);
}

AST* interp_binop_sub(Worker *worker, AST* node) {
    AST* left = interp(worker, node->binop.left);
    AST* right = interp(worker, node->binop.right);

    if (ast_is_numeric(left) && ast_is_numeric(right)) {
        return interp(worker, REAL(ast_to_f64(left)-ast_to_f64(right)));
    } else if (ast_match_type(left, create_ast_integer(1)) && ast_match_type(right, create_ast_div(create_ast_integer(1), create_ast_integer(1)))) {
        // a - b/c -> ac/c - b/c = (ac-b)/c
        AST* a = left;
        AST* b = right->binop.left;
        AST* c = right->binop.right;
        return interp_binop_div(worker,
            _create_ast_binop(&worker->arena, 
                _create_ast_binop(&worker->arena,
                    _create_ast_binop(&worker->arena, a, c, OP_MUL),
                    b,
                    OP_SUB
                ),
                c,
                OP_DIV
            )
        );
    } else if (ast_match_type(left, create_ast_div(create_ast_integer(1), create_ast_integer(1))) && ast_match_type(right, create_ast_integer(1))) {
        // a/b - c -> a/b - cb/b = (a-cb)/b
        AST *a = left->binop.left;
        AST *b = left->binop.right;
        AST *c = right;
        AST *cb = _create_ast_binop(&worker->arena, c, b, OP_MUL);
        AST *numerator = _create_ast_binop(&worker->arena, a, cb, OP_SUB);
        return interp_binop_div(worker, _create_ast_binop(&worker->arena, numerator, b, OP_DIV));
    } else if (ast_match(right, _create_ast_integer(&worker->arena, 0))) {
        return left;
    }

    return node;
}

AST* interp_binop_mul(Worker *worker, AST* node) {
    AST* left = interp(worker, node->binop.left);
    AST* right = interp(worker, node->binop.right);

    if (ast_is_numeric(left) && ast_is_numeric(right)) {
        return interp(worker, REAL(ast_to_f64(left)*ast_to_f64(right)));
    } else if (ast_match(left, _create_ast_integer(&worker->arena, 0)) || ast_match(_create_ast_integer(&worker->arena, 0), right)) {
        return _create_ast_integer(&worker->arena, 0);
    } else if (ast_match(left, right)) {
        return interp_binop_pow(worker, _create_ast_binop(&worker->arena, left, _create_ast_integer(&worker->arena, 2), OP_POW));
    } else if (ast_match_type(left, create_ast_div(create_ast_integer(1), create_ast_integer(1))) && ast_match_type(right, create_ast_div(create_ast_integer(1), create_ast_integer(1)))) {
        // a/b * c/d -> ac/bd
        AST *a = left->binop.left;
        AST *b = left->binop.right;
        AST *c = right->binop.left;
        AST *d = right->binop.right;
        AST *ac = _create_ast_binop(&worker->arena, a, c, OP_MUL);
        AST *bd = _create_ast_binop(&worker->arena, b, d, OP_MUL);
        return interp_binop_div(worker, _create_ast_binop(&worker->arena, ac, bd, OP_DIV));
    } else if (ast_match(left, _create_ast_integer(&worker->arena, 1)) || ast_match(right, _create_ast_integer(&worker->arena, 1))) {
        if (ast_match(left, _create_ast_integer(&worker->arena, 1))) {
            return right;
        } else {
            return left;
        }
    }
    return _create_ast_binop(&worker->arena, left, right, OP_MUL);
}

AST* interp_binop_div(Worker *worker, AST* node) {
    AST* left = interp(worker, node->binop.left);
    AST* right = interp(worker, node->binop.right);
    if (left->type == AST_INTEGER && right->type == AST_INTEGER) {
        if (left->integer.value % right->integer.value == 0) {
            return _create_ast_integer(&worker->arena, left->integer.value / right->integer.value);
        } else {
            return _create_ast_binop(&worker->arena, left, right, OP_DIV);
        }
    } else if (ast_is_numeric(left) && ast_is_numeric(right)) {
        f64 l = ast_to_f64(left);
        f64 r = ast_to_f64(right);
        return interp(worker, create_ast_real(l/r));
    } else if (left->type == AST_INTEGER && right->type == AST_BINOP && right->binop.type == OP_DIV) {
        AST* r = _create_ast_binop(&worker->arena, _create_ast_binop(&worker->arena, left, right->binop.left, OP_MUL), right->binop.right, OP_DIV);
        return interp_binop(worker, r);
    }
    return _create_ast_binop(&worker->arena, left, right, OP_DIV);
}

AST *interp_binop_pow(Worker *worker, AST *node) {
    AST* left = interp(worker, node->binop.left);
    AST* right = interp(worker, node->binop.right);
    if (left->type == AST_INTEGER && right->type == AST_INTEGER) {
        long double l = (long double)left->integer.value;
        long double r = (long double)right->integer.value;
        int64_t result = (int64_t)powl(l, r);
        return _create_ast_integer(&worker->arena, result);
    } else if (ast_match(right, _create_ast_integer(&worker->arena, 0))) {
        return _create_ast_integer(&worker->arena, 1);
    } else if (ast_match(right, _create_ast_integer(&worker->arena, 1))) {
        return left;
    }
    return _create_ast_binop(&worker->arena, left, right, OP_POW);
}

AST* interp_binop(Worker *worker, AST* node) {
    switch (node->binop.type) {
        case OP_ADD: return interp_binop_add(worker, node);
        case OP_SUB: return interp_binop_sub(worker, node);
        case OP_MUL: return interp_binop_mul(worker, node);
        case OP_DIV: return interp_binop_div(worker, node);
        case OP_POW: return interp_binop_pow(worker, node);
        default: assert(false);
    }
}

AST* interp_unaryop(Worker *worker, AST* node) {
    AST* expr = interp(worker, node->unaryop.expr);
    OpType op_type = node->unaryop.type;

    if (expr->type == AST_INTEGER) {
        switch (op_type) {
            case OP_UADD: return _create_ast_integer(&worker->arena, +expr->integer.value);
            case OP_USUB: return _create_ast_integer(&worker->arena, -expr->integer.value);
            default: assert(false);
        }
    }

    return create_ast_unaryop(&worker->arena, expr, op_type);
}

AST* diff(Worker *worker, AST *expr, AST *var) {
    (void)worker;
    (void)var;

    switch (expr->type) {
        case AST_SYMBOL: {
            if (ast_match(expr, var)) {
                return create_ast_integer(1);
            } else {
                return create_ast_integer(0);
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
                    AST* result = create_ast_mul(create_ast_mul(n, create_ast_pow(b, create_ast_sub(n, create_ast_integer(1)))), diff(worker, b, var));
                    return interp_binop_mul(worker, result);
                }
                case OP_ADD: {
                    // a + b -> a' + b'
                    AST *dl = diff(worker, expr->binop.left, var);
                    AST *dr = diff(worker, expr->binop.right, var);
                    AST *result = create_ast_add(dl, dr);
                    return interp_binop_add(worker, result);
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

AST *interp_sin(Worker *worker, AST* node) {
    assert(node->func_call.args.size == 1);
    AST *arg = node->func_call.args.data[0];

    if (ast_match(arg, create_ast_symbol("pi"))) {
        return create_ast_integer(0);
    } else if (arg->type == AST_INTEGER || arg->type == AST_REAL) {
        f64 value = ast_to_f64(arg);
        return interp(worker, create_ast_real(sin(value)));
    }
    
    return node;
}

AST *interp_cos(Worker *worker, AST* node) {
    assert(node->func_call.args.size == 1);
    AST *arg = node->func_call.args.data[0];
    
    if (ast_match(arg, create_ast_integer(0))) {
        return create_ast_integer(1);
    } else if (ast_match(arg, create_ast_div(create_ast_symbol("pi"), create_ast_integer(2)))) {
        return create_ast_integer(0);
    } else if (ast_match(arg, create_ast_symbol("pi"))) {
        return create_ast_integer(-1);
    } else if (arg->type == AST_INTEGER || arg->type == AST_REAL) {
        f64 value = ast_to_f64(arg);
        return interp(worker, create_ast_real(cos(value)));
    }

    return node;
}

AST* interp_func_call(Worker *worker, AST* node) {
    // we need a proper system for checking and using args (function definitions and function signatures) @ƒeature

    char *func_name = node->func_call.name;

    for (usize i = 0; i < node->func_call.args.size; i++) {
        node->func_call.args.data[i] = interp(worker, node->func_call.args.data[i]);
    }

    ASTArray args = node->func_call.args;

    if (!strcmp(func_name, "sqrt") && args.data[0]->type == AST_INTEGER) {
        double result = sqrt((double)args.data[0]->integer.value);

        if (fmod((double)args.data[0]->integer.value, result) == 0.0) {
            return _create_ast_integer(&worker->arena, (int)result);
        }

        // check for perfect square
        // @Speed: this is probably an inefficient way to compute the biggest perfect sqaure factor of a given number
        for (i32 q = args.data[0]->integer.value; q > 1; q--) {
            double sqrt_of_q = sqrtf((double)q);
            bool is_perfect_square = sqrt_of_q == floor(sqrt_of_q);
            if (is_perfect_square && args.data[0]->integer.value % q == 0) {
                i32 p = args.data[0]->integer.value / q;
                ASTArray new_args = {0};
                ast_array_append(&worker->arena, &new_args, _create_ast_integer(&worker->arena, p));
                return _create_ast_binop(&worker->arena, _create_ast_integer(&worker->arena, (i64)sqrt_of_q), create_ast_call(&worker->arena, func_name, new_args), OP_MUL);
            }
        }
    } else if (!strcmp(func_name, "ln")) {
        if (ast_match(args.data[0], _create_ast_symbol(&worker->arena, "e"))) {
            return _create_ast_integer(&worker->arena, 1);
        } else if (ast_match(args.data[0], _create_ast_integer(&worker->arena, 1))) {
            return _create_ast_integer(&worker->arena, 0);
        }
    } else if (!strcmp(func_name, "log")) {
        if (node->func_call.args.size == 2) {
            // log_b(y) = x
            // b^x = y
            AST* y = node->func_call.args.data[0];
            AST* b = node->func_call.args.data[1];
            if (ast_match(y, _create_ast_integer(&worker->arena, 1))) {
                return _create_ast_integer(&worker->arena, 0);
            } else if (ast_match(y, b)) {
                return _create_ast_integer(&worker->arena, 1);
            }
        } else {
            assert(false);
        }
    } else if (!strcmp(func_name, "sin")) {
        return interp_sin(worker, node);
    } else if (!strcmp(func_name, "cos")) {
        return interp_cos(worker, node);
    } else if (!strcmp(func_name, "diff")) {
        AST* diff_var;

        if (node->func_call.args.size == 1) {
            ASTArray nodes = ast_to_flat_array(&worker->arena, node);
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

        return diff(worker, node->func_call.args.data[0], diff_var);
    }

    return node;
}

AST* interp(Worker *worker, AST* node) {
    switch (node->type) {
        case AST_BINOP:
            return interp_binop(worker, node);
        case AST_UNARYOP:
            return interp_unaryop(worker, node);
        case AST_INTEGER:
        case AST_SYMBOL:
            return node;
        case AST_REAL: {
            f64 value = node->real.value;
            if (value - floor(value) == 0.0) {
                return create_ast_integer((i64)value);
            }
            return node;
        }
        case AST_CALL:
            return interp_func_call(worker, node);
        case AST_EMPTY:
            return node;
        default:
            fprintf(stderr, "%s:%d: Cannot interp node type '%s'\n", __FILE__, __LINE__, ast_type_to_debug_string(node->type));
            exit(1);
    }
}

AST* interp_from_string(Worker *worker, char *source) {
    worker->lexer.source = source;

    AST* ast = parse(worker);
    AST* output = interp(worker, ast);
    
    return output;
}
