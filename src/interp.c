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
            case AST_FUNC_CALL: {
                if (
                    !strcmp(left->func_call.name.text, right->func_call.name.text) &&
                    left->func_call.args.size == right->func_call.args.size
                ) {
                    for (size_t i = 0; i < left->func_call.args.size; i++) {
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
        default: printf("ERROR: In ast_match_type, node type %s not implemented.\n", ast_type_to_debug_string(left->type)); break;
    }

    return false;
}

char *ast_to_debug_string(AST* node) {
    // TODO: memory management needed here
    char *output = malloc(1024);
    switch (node->type) {
        case AST_INTEGER: sprintf(output, "%s(%lld)", ast_type_to_debug_string(node->type), node->integer.value); break;
        case AST_SYMBOL: sprintf(output, "%s(%s)", ast_type_to_debug_string(node->type), node->symbol.name); break;
        case AST_BINOP: sprintf(output, "%s(%s, %s)", op_type_to_debug_string(node->binop.type), ast_to_debug_string(node->binop.left), ast_to_debug_string(node->binop.right)); break;
        case AST_UNARYOP: sprintf(output, "%s(%s)", op_type_to_debug_string(node->unaryop.type), ast_to_debug_string(node->unaryop.expr)); break;
        // TODO: args to debug string
        case AST_FUNC_CALL: {
            if (node->func_call.args.size == 1) {
                sprintf(output, "FuncCall(%s, %s)", node->func_call.name.text, ast_to_debug_string(node->func_call.args.data[0])); break;
            } else {
                // TODO: multiple args to string
                sprintf(output, "FuncCall(%s, args)", node->func_call.name.text); break;
            }
        }
        case AST_EMPTY: sprintf(output, "Empty()"); break;
        default: fprintf(stderr, "ERROR: Cannot do 'ast_to_debug_string' because node type '%s' is not implemented.\n", ast_type_to_debug_string(node->type)); exit(1);
    }
    return output;
}

char *_ast_to_string(Arena *arena, AST* node, uint8_t op_precedence) {
    // TODO: parse arena here for the malloc
    char *output = arena_alloc(arena, 1024);
    switch (node->type) {
        case AST_INTEGER: sprintf(output, "%lld", node->integer.value); break;
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
        case AST_FUNC_CALL: {
            if (node->func_call.args.size == 1) {
                sprintf(output, "%s(%s)", node->func_call.name.text, _ast_to_string(arena, node->func_call.args.data[0], op_precedence)); break;
            } else {
                // TODO: multiple args to string
                sprintf(output, "%s(args)", node->func_call.name.text); break;
            }
        }
        case AST_EMPTY: break;
        default: fprintf(stderr, "ERROR: Cannot do 'ast_to_string' because node type '%s' is not implemented.\n", ast_type_to_debug_string(node->type)); exit(1);
    }
    return output;
}

AST* interp_binop_add(Worker *w, AST* node) {
    AST* left = interp(w, node->binop.left);
    AST* right = interp(w, node->binop.right);

    if (left->type == AST_INTEGER && right->type == AST_INTEGER) {
        return create_ast_integer(&w->arena, left->integer.value + right->integer.value);
    } else if (ast_match(left, INTEGER(0))) {
        return right;
    } else if (ast_match(right, INTEGER(0))) {
        return left;
    } else if (ast_match(left, right)) {
        return create_ast_binop(&w->arena, create_ast_integer(&w->arena, 2), left, OP_MUL);
    } else if (ast_match_type(left, create_ast_binop(&w->arena, create_ast_integer(&w->arena, 1), create_ast_integer(&w->arena, 1), OP_DIV)) && ast_match_type(right, create_ast_binop(&w->arena, create_ast_integer(&w->arena, 1), create_ast_integer(&w->arena, 1), OP_DIV))) {
        // a/b + c/d
        int64_t a = left->binop.left->integer.value;
        int64_t b = left->binop.right->integer.value;
        int64_t c = right->binop.left->integer.value;
        int64_t d = right->binop.right->integer.value;
        AST* r = create_ast_binop(&w->arena, create_ast_integer(&w->arena, a*d+c*b), create_ast_integer(&w->arena, b*d), OP_DIV);
        return interp_binop_div(w, r);
    } else if (left->type == AST_INTEGER && right->type == AST_BINOP && right->binop.type == OP_DIV) {
        // c + a/b
        // -> (cb)/b + a/b
        AST* r = create_ast_binop(&w->arena, create_ast_binop(&w->arena, create_ast_binop(&w->arena, left, right->binop.right, OP_MUL), right->binop.right, OP_DIV), right, OP_ADD);
        return interp_binop(w, r);
    } else if (left->type == AST_BINOP && left->binop.type == OP_MUL) {
        if (ast_match(left->binop.right, right)) {
            AST* new_left = interp_binop(w, create_ast_binop(&w->arena, create_ast_integer(&w->arena, 1), left->binop.left, OP_ADD));
            return create_ast_binop(&w->arena, new_left, right, OP_MUL);
        }
    }
    return create_ast_binop(&w->arena, left, right, OP_ADD);
}

AST* interp_binop_sub(Worker *w, AST* node) {
    AST* left = interp(w, node->binop.left);
    AST* right = interp(w, node->binop.right);

    if (left->type == AST_INTEGER && right->type == AST_INTEGER) {
        return create_ast_integer(&w->arena, left->integer.value - right->integer.value);
    } else if (ast_match_type(left, INTEGER(1)) && ast_match_type(right, DIV(INTEGER(1), INTEGER(1)))) {
        // a - b/c -> ac/c - b/c = (ac-b)/c
        AST* a = left;
        AST* b = right->binop.left;
        AST* c = right->binop.right;
        return interp_binop_div(w,
            create_ast_binop(&w->arena, 
                create_ast_binop(&w->arena,
                    create_ast_binop(&w->arena, a, c, OP_MUL),
                    b,
                    OP_SUB
                ),
                c,
                OP_DIV
            )
        );
    } else if (ast_match_type(left, DIV(INTEGER(1), INTEGER(1))) && ast_match_type(right, INTEGER(1))) {
        // a/b - c -> a/b - cb/b = (a-cb)/b
        AST *a = left->binop.left;
        AST *b = left->binop.right;
        AST *c = right;
        AST *cb = create_ast_binop(&w->arena, c, b, OP_MUL);
        AST *numerator = create_ast_binop(&w->arena, a, cb, OP_SUB);
        return interp_binop_div(w, create_ast_binop(&w->arena, numerator, b, OP_DIV));
    } else if (ast_match(right, create_ast_integer(&w->arena, 0))) {
        return left;
    }

    return node;
}

AST* interp_binop_mul(Worker *w, AST* node) {
    AST* left = interp(w, node->binop.left);
    AST* right = interp(w, node->binop.right);
    if (left->type == AST_INTEGER && right->type == AST_INTEGER) {
        return create_ast_integer(&w->arena, left->integer.value * right->integer.value);
    } else if (ast_match(left, create_ast_integer(&w->arena, 0)) || ast_match(create_ast_integer(&w->arena, 0), right)) {
        return create_ast_integer(&w->arena, 0);
    } else if (ast_match(left, right)) {
        return interp_binop_pow(w, create_ast_binop(&w->arena, left, create_ast_integer(&w->arena, 2), OP_POW));
    } else if (ast_match_type(left, DIV(INTEGER(1), INTEGER(1))) && ast_match_type(right, DIV(INTEGER(1), INTEGER(1)))) {
        // a/b * c/d -> ac/bd
        AST *a = left->binop.left;
        AST *b = left->binop.right;
        AST *c = right->binop.left;
        AST *d = right->binop.right;
        AST *ac = create_ast_binop(&w->arena, a, c, OP_MUL);
        AST *bd = create_ast_binop(&w->arena, b, d, OP_MUL);
        return interp_binop_div(w, create_ast_binop(&w->arena, ac, bd, OP_DIV));
    } else if (ast_match(left, create_ast_integer(&w->arena, 1)) || ast_match(right, create_ast_integer(&w->arena, 1))) {
        if (ast_match(left, create_ast_integer(&w->arena, 1))) {
            return right;
        } else {
            return left;
        }
    }
    return create_ast_binop(&w->arena, left, right, OP_MUL);
}

AST* interp_binop_div(Worker *w, AST* node) {
    AST* left = interp(w, node->binop.left);
    AST* right = interp(w, node->binop.right);
    if (left->type == AST_INTEGER && right->type == AST_INTEGER) {
        if (left->integer.value % right->integer.value == 0) {
            return create_ast_integer(&w->arena, left->integer.value / right->integer.value);
        } else {
            return create_ast_binop(&w->arena, left, right, OP_DIV);
        }
    } else if (left->type == AST_INTEGER && right->type == AST_BINOP && right->binop.type == OP_DIV) {
        AST* r = create_ast_binop(&w->arena, create_ast_binop(&w->arena, left, right->binop.left, OP_MUL), right->binop.right, OP_DIV);
        return interp_binop(w, r);
    }
    return create_ast_binop(&w->arena, left, right, OP_DIV);
}

AST *interp_binop_pow(Worker *w, AST *node) {
    AST* left = interp(w, node->binop.left);
    AST* right = interp(w, node->binop.right);
    if (left->type == AST_INTEGER && right->type == AST_INTEGER) {
        long double l = (long double)left->integer.value;
        long double r = (long double)right->integer.value;
        int64_t result = (int64_t)powl(l, r);
        return create_ast_integer(&w->arena, result);
    } else if (ast_match(right, create_ast_integer(&w->arena, 0))) {
        return create_ast_integer(&w->arena, 1);
    } else if (ast_match(right, create_ast_integer(&w->arena, 1))) {
        return left;
    }
    return create_ast_binop(&w->arena, left, right, OP_POW);
}

AST* interp_binop(Worker *w, AST* node) {
    switch (node->binop.type) {
        case OP_ADD: return interp_binop_add(w, node);
        case OP_SUB: return interp_binop_sub(w, node);
        case OP_MUL: return interp_binop_mul(w, node);
        case OP_DIV: return interp_binop_div(w, node);
        case OP_POW: return interp_binop_pow(w, node);
        default: assert(false);
    }
}

AST* interp_unaryop(Worker *w, AST* node) {
    AST* expr = interp(w, node->unaryop.expr);
    OpType op_type = node->unaryop.type;

    if (expr->type == AST_INTEGER) {
        switch (op_type) {
            case OP_UADD: return create_ast_integer(&w->arena, +expr->integer.value);
            case OP_USUB: return create_ast_integer(&w->arena, -expr->integer.value);
            default: assert(false);
        }
    }

    return create_ast_unaryop(&w->arena, expr, op_type);
}

AST* diff(Worker *w, AST *expr, AST *var) {
    (void)w;
    (void)var;

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
                    AST* result = MUL(MUL(n, POW(b, SUB(n, INTEGER(1)))), diff(w, b, var));
                    return interp_binop_mul(w, result);
                }
                case OP_ADD: {
                    // a + b -> a' + b'
                    AST *dl = diff(w, expr->binop.left, var);
                    AST *dr = diff(w, expr->binop.right, var);
                    AST *result = ADD(dl, dr);
                    return interp_binop_add(w, result);
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

AST* interp_func_call(Worker *w, AST* node) {
    // TODO: we need a proper system for checking and using args (function definitions and function signatures)

    Token name = node->func_call.name;

    for (size_t i = 0; i < node->func_call.args.size; i++) {
        node->func_call.args.data[i] = interp(w, node->func_call.args.data[i]);
    }
    // for most cases we only use first arg for now
    // TODO: remove this later
    AST *arg = node->func_call.args.data[0];

    if (!strcmp(name.text, "sqrt") && arg->type == AST_INTEGER) {
        double result = sqrt((double)arg->integer.value);
        if (fmod((double)arg->integer.value, result) == 0.0) {
            return create_ast_integer(&w->arena, (int)result);
        }

        // check for perfect square
        // @Speed: this is probably an inefficient way to compute the biggest perfect sqaure factor of a given number
        for (int q = arg->integer.value; q > 1; q--) {
            double sqrt_of_q = sqrtf((double)q);
            bool is_perfect_square = sqrt_of_q == floor(sqrt_of_q);
            if (is_perfect_square && arg->integer.value % q == 0) {
                int p = arg->integer.value / q;
                ASTArray args = {0};
                ast_array_append(&args, create_ast_integer(&w->arena, p));
                return create_ast_binop(&w->arena, create_ast_integer(&w->arena, (int64_t)sqrt_of_q), create_ast_func_call(&w->arena, name, args), OP_MUL);
            }
        }
    } else if (!strcmp(name.text, "ln")) {
        if (ast_match(arg, create_ast_symbol(&w->arena, "e"))) {
            return create_ast_integer(&w->arena, 1);
        } else if (ast_match(arg, create_ast_integer(&w->arena, 1))) {
            return create_ast_integer(&w->arena, 0);
        }
    } else if (!strcmp(name.text, "log")) {
        if (node->func_call.args.size == 2) {
            // log_b(y) = x
            // b^x = y
            AST* y = node->func_call.args.data[0];
            AST* b = node->func_call.args.data[1];
            if (ast_match(y, create_ast_integer(&w->arena, 1))) {
                return create_ast_integer(&w->arena, 0);
            } else if (ast_match(y, b)) {
                return create_ast_integer(&w->arena, 1);
            }
        } else {
            assert(false);
        }
    } else if (ast_match_string(node, "sin(pi)")) {
        return create_ast_integer(&w->arena, 0);
    } else if (ast_match_string(node, "cos(0)")) {
        return create_ast_integer(&w->arena, 1);
    } else if (ast_match_string(node, "cos(pi/2)")) {
        return create_ast_integer(&w->arena, 0);
    } else if (ast_match_string(node, "cos(pi)")) {
        return create_ast_integer(&w->arena, -1);
    } else if (!strcmp(name.text, "diff")) {
        AST* diff_var;

        if (node->func_call.args.size == 1) {
            ASTArray nodes = ast_to_flat_array(&w->arena, node);
            bool symbol_seen = false;
            AST *symbol;
            for (size_t i = 0; i < nodes.size; i++) {
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

        return diff(w, node->func_call.args.data[0], diff_var);
    }

    return node;
}

AST* interp(Worker *w, AST* node) {
    // TODO: unpack here. For example interp_binop(node.binop)
    switch (node->type) {
        case AST_BINOP:
            return interp_binop(w, node);
        case AST_UNARYOP:
            return interp_unaryop(w, node);
        case AST_INTEGER:
        case AST_SYMBOL:
            return node;
        case AST_FUNC_CALL:
            return interp_func_call(w, node);
        case AST_EMPTY:
            return node;
        default:
            fprintf(stderr, "ERROR: Cannot interp node type '%s'\n", ast_type_to_debug_string(node->type));
            exit(1);
    }
}

AST* interp_from_string(Worker *w, char *source) {
    w->lexer.source = source;

    AST* ast = parse(w);
    AST* output = interp(w, ast);
    
    return output;
}
