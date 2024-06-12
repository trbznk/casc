#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>

#include "casc.h"

// forward declarations
AST *interp_binop_div(Interp*, AST*, AST*);
AST *interp_binop(Interp*, AST*, AST*, OpType);

bool ast_match(AST* left, AST* right) {

    if (left->type == right->type) {
        switch (left->type) {
            case AST_INTEGER:
                return left->integer.value == right->integer.value;
            case AST_REAL:
                return left->real.value == right->real.value;
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
            if (left->binop.op == right->binop.op) {
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
        case AST_BINOP: sprintf(output, "%s(%s, %s)", op_type_to_debug_string(node->binop.op), ast_to_debug_string(arena, node->binop.left), ast_to_debug_string(arena, node->binop.right)); break;
        case AST_UNARYOP: sprintf(output, "%s(%s)", op_type_to_debug_string(node->unaryop.op), ast_to_debug_string(arena, node->unaryop.operand)); break;
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

            uint8_t current_op_precedence = op_type_precedence(node->binop.op);

            char *left_string = _ast_to_string(arena, node->binop.left, current_op_precedence);
            char *right_string = _ast_to_string(arena, node->binop.right, current_op_precedence);
            const char *op_type_string = op_type_to_string(node->binop.op);

            if (current_op_precedence < op_precedence) {
                sprintf(output, "(%s%s%s)", left_string, op_type_string, right_string);
            } else {
                sprintf(output, "%s%s%s", left_string, op_type_string, right_string);
            }

            break;
        }
        case AST_UNARYOP: {
            uint8_t current_op_precedence = op_type_precedence(node->binop.op);

            char *expr_string = _ast_to_string(arena, node->unaryop.operand, op_precedence);
            const char *op_type_string = op_type_to_string(node->unaryop.op);

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

AST* interp_binop_add(Interp *ip, AST *left, AST *right) {
    // basic rules
    if (ast_match(left, INTEGER(0))) {
        return right;
    } else if (ast_match(right, INTEGER(0))) {
        return left;
    } else if (ast_match(left, right)) {
        return interp(ip, MUL(INTEGER(2), left));
    }
    
    // fractions
    if (ast_is_fraction(left) && ast_is_fraction(right)) {
        // a/b + c/d
        i64 a = left->binop.left->integer.value;
        i64 b = left->binop.right->integer.value;
        i64 c = right->binop.left->integer.value;
        i64 d = right->binop.right->integer.value;
        return interp_binop_div(ip, INTEGER(a*d+c*b), INTEGER(b*d));
    } else if (left->type == AST_INTEGER && ast_is_fraction(right)) {
        // c + a/b
        // -> (cb)/b + a/b
        // -> (cb+a)/b
        AST *c = left;
        AST *a = right->binop.left;
        AST *b = right->binop.right;
        // we need depth again so interp instead of interp_binop_add
        return interp(ip, DIV(ADD(MUL(c, b), a), b));
    } else if (ast_is_fraction(left) && right->type == AST_INTEGER) {
        return interp_binop_add(ip, right, left);
    }
    
    // TODO: remember which rule this is and simplify it
    if (left->type == AST_BINOP && left->binop.op == OP_MUL) {
        if (ast_match(left->binop.right, right)) {
            AST *new_left = interp_binop_add(ip, INTEGER(1), left->binop.left);
            return MUL(new_left, right);
        }
    }
    
    if (ast_is_numeric(left) && ast_is_numeric(right)) {
        return interp(ip, REAL(ast_to_f64(left)+ast_to_f64(right)));
    }
    
    return ADD(left, right);
}

AST* interp_binop_sub(Interp *ip, AST *left, AST *right) {
    
    if (ast_match_type(left, INTEGER(1)) && ast_match_type(right, DIV(INTEGER(1), INTEGER(1)))) {
        //   a - b/c
        // = ac/c - b/c
        // = (ac-b)/c
        AST* a = left;
        AST* b = right->binop.left;
        AST* c = right->binop.right;
        return interp(ip, DIV(SUB(MUL(a, c), b), c));
    } else if (ast_is_fraction(left) && right->type == AST_INTEGER) {
        //   a/b - c 
        // = a/b - cb/b
        // = (a-cb)/b
        AST *a = left->binop.left;
        AST *b = left->binop.right;
        AST *c = right;
        // we need depth again
        return interp(ip, DIV(SUB(a, MUL(c, b)), b));
    } else if (left->type == AST_INTEGER && ast_is_fraction(right)) {
        return interp_binop_sub(ip, right, left);
    } else if (ast_match(right, INTEGER(0))) {
        return left;
    } else if (ast_is_numeric(left) && ast_is_numeric(right)) {
        return interp(ip, REAL(ast_to_f64(left)-ast_to_f64(right)));
    }

    return SUB(left, right);
}

AST* interp_binop_mul(Interp *ip, AST *left, AST *right) {
    if (ast_is_fraction(left) && right->type == AST_INTEGER) {
        // a/b * c
        AST *a = left->binop.left;
        AST *b = left->binop.right;
        AST *c = right;
        AST *num = interp(ip, MUL(a, c));
        AST *den = b;
        return interp(ip, DIV(num, den));
    } else if (left->type == AST_INTEGER && ast_is_fraction(right)) {
        return interp_binop_mul(ip, right, left);
    } else if (ast_match(left, INTEGER(0)) || ast_match(INTEGER(0), right)) {
        return INTEGER(0);
    } else if (ast_match(left, right)) {
        return interp_binop_pow(ip, left, INTEGER(2));
    } else if (ast_is_fraction(left) && ast_is_fraction(right)) {
        //   a/b * c/d
        // = ac/bd
        AST *a = left->binop.left;
        AST *b = left->binop.right;
        AST *c = right->binop.left;
        AST *d = right->binop.right;
        return interp(ip, DIV(MUL(a, c), MUL(b, d)));
    } else if (ast_match(left, INTEGER(1)) || ast_match(right, INTEGER(1))) {
        if (ast_match(left, INTEGER(1))) {
            return right;
        } else {
            return left;
        }
    } else if (ast_is_numeric(left) && ast_is_numeric(right)) {
        return interp(ip, REAL(ast_to_f64(left)*ast_to_f64(right)));
    }
    return MUL(left, right);
}

AST* interp_binop_div(Interp *ip, AST *left, AST *right) {
    assert(!ast_match(right, INTEGER(0))); // zero division

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
    } else if (left->type == AST_INTEGER && right->type == AST_BINOP && right->binop.op == OP_DIV) {
        return interp_binop_div(ip, MUL(left, right->binop.left), right->binop.right);
    }
    return DIV(left, right);
}

AST *interp_binop_pow(Interp *ip, AST *left, AST *right) {
    if (ast_match(right, INTEGER(0))) {
        return INTEGER(1);
    } else if (ast_match(right, INTEGER(1))) {
        return left;
    } else if (ast_is_numeric(left) && ast_is_numeric(right)) {
        f64 l = ast_to_f64(left);
        f64 r = ast_to_f64(right);
        return interp(ip, REAL(pow(l, r)));
    }

    return POW(left, right);
}

AST* interp_binop(Interp *ip, AST *left, AST *right, OpType op) {
    left = interp(ip, left);
    right = interp(ip, right);

    switch (op) {
        case OP_ADD: return interp_binop_add(ip, left, right);
        case OP_SUB: return interp_binop_sub(ip, left, right);
        case OP_MUL: return interp_binop_mul(ip, left, right);
        case OP_DIV: return interp_binop_div(ip, left, right);
        case OP_POW: return interp_binop_pow(ip, left, right);
        default: assert(false);
    }
}

AST* interp_unaryop(Interp *ip, OpType op, AST *operand) {
    operand = interp(ip, operand);

    if (op == OP_UADD) {
        return operand;
    } else if (ast_is_numeric(operand)) {
        f64 value = ast_to_f64(operand);
        return interp(ip, REAL(-value));
    }

    return init_ast_unaryop(ip->arena, operand, op);
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
            switch (expr->binop.op) {
                case OP_POW: {
                    assert(ast_contains(expr->binop.left, var)); // check if base is function of 'var'
                    assert(!ast_contains(expr->binop.right, var)); // exponent must be constant for now
                    
                    AST* b = expr->binop.left;
                    AST* n = expr->binop.right;
                    // x^n -> n * x^(n-1) * x'
                    return interp_binop_mul(ip, MUL(n, POW(b, SUB(n, INTEGER(1)))), diff(ip, b, var));
                }
                case OP_ADD: {
                    // a + b -> a' + b'
                    AST *dl = diff(ip, expr->binop.left, var);
                    AST *dr = diff(ip, expr->binop.right, var);
                    return interp_binop_add(ip, dl, dr);
                }
                default:
                    printf("op '%s' not implemented\n", op_type_to_debug_string(expr->binop.op));
                    assert(false);
            }
        }
        default:
            printf("type '%s' not implemented\n", ast_type_to_debug_string(expr->type));
            assert(false);
    }
}

AST *interp_sin(Interp *ip, AST* x) {
    if (ast_match(x, SYMBOL("pi"))) {
        return INTEGER(0);
    } else if (ast_is_numeric(x)) {
        f64 value = ast_to_f64(x);
        return interp(ip, REAL(sin(value)));
    }
    
    ASTArray args = {0};
    ast_array_append(ip->arena, &args, x);
    return CALL("sin", args);
}

AST *interp_cos(Interp *ip, AST* x) {    
    if (ast_match(x, INTEGER(0))) {
        return INTEGER(1);
    } else if (ast_match(x, DIV(SYMBOL("pi"), INTEGER(2)))) {
        return INTEGER(0);
    } else if (ast_match(x, SYMBOL("pi"))) {
        return INTEGER(-1);
    } else if (ast_is_numeric(x)) {
        f64 value = ast_to_f64(x);
        return interp(ip, REAL(cos(value)));
    }

    ASTArray args = {0};
    ast_array_append(ip->arena, &args, x);
    return CALL("cos", args);
}

AST *interp_tan(Interp *ip, AST* x) {
    if (ast_is_numeric(x)) {
        f64 value = ast_to_f64(x);
        return interp(ip, REAL(tan(value)));
    }
    
    ASTArray args = {0};
    ast_array_append(ip->arena, &args, x);
    return CALL("tan", args);
}

AST *interp_log(Interp *ip, AST *y, AST *b) {
    // log_b(y) = x
    // b^x = y
    if (ast_match(y, INTEGER(1))) {
        return INTEGER(0);
    } else if (ast_match(y, b)) {
        return INTEGER(1);
    }


    // TODO: implement maybe variadic function or macro to simplify ast array
    //       creation from with args given.
    ASTArray args = {0};
    ast_array_append(ip->arena, &args, y);
    ast_array_append(ip->arena, &args, b);
    return CALL("log", args);
}

AST* interp_call(Interp *ip, char *name, ASTArray args) {

    for (usize i = 0; i < args.size; i++) {
        args.data[i] = interp(ip, args.data[i]);
    }

    if (!strcmp(name, "sqrt") && args.data[0]->type == AST_INTEGER) {
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
                return MUL(INTEGER((i64)sqrt_of_q), CALL(name, new_args));
            }
        }
    } else if (!strcmp(name, "ln")) {
        if (ast_match(args.data[0], SYMBOL("e"))) {
            return INTEGER(1);
        } else if (ast_match(args.data[0], INTEGER(1))) {
            return INTEGER(0);
        }
    } else if (!strcmp(name, "log")) {
        assert(args.size == 2);
        return interp_log(ip, args.data[0], args.data[1]);
    } else if (!strcmp(name, "sin")) {
        assert(args.size == 1);
        return interp_sin(ip, args.data[0]);
    } else if (!strcmp(name, "cos")) {
        assert(args.size == 1);
        return interp_cos(ip, args.data[0]);
    } else if (!strcmp(name, "tan")) {
        assert(args.size == 1);
        return interp_tan(ip, args.data[0]);
    } else if (!strcmp(name, "diff")) {
        AST* diff_var;

        if (args.size == 1) {
            ASTArray nodes = ast_to_flat_array(ip->arena, args.data[0]);
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
        } else if (args.size == 2) {
            assert(args.data[1]->type == AST_SYMBOL);
            diff_var = args.data[1];
        } else {
            assert(false);
        }

        return diff(ip, args.data[0], diff_var);
    }

    return CALL(name, args);
}

AST* interp(Interp *ip, AST* node) {
    switch (node->type) {
        case AST_BINOP:
            return interp_binop(ip, node->binop.left, node->binop.right, node->binop.op);
        case AST_UNARYOP:
            return interp_unaryop(ip, node->unaryop.op, node->unaryop.operand);
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
            return interp_call(ip, node->func_call.name, node->func_call.args);
        case AST_EMPTY:
            return node;
        default:
            fprintf(stderr, "%s:%d: Cannot interp node type '%s'\n", __FILE__, __LINE__, ast_type_to_debug_string(node->type));
            exit(1);
    }
}
