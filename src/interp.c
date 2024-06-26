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

const FunctionSignature BUILTIN_FUNCTIONS[] =  {
    {"pow", 2}, {"exp", 1},
    {"sqrt", 1},
    {"sin", 1}, {"cos", 1}, {"tan", 1},
    {"asin", 1}, {"acos", 1}, {"atan", 1},
    {"ln", 1},
    {"log", 2},
    {"exp", 1},
    {"abs", 1},
    {"factorial", 1},
    {"npr", 2}, {"ncr", 2},
    {"gcd", 2}, {"lcm", 2},
    {"diff", 1}, {"diff", 2}
};
const usize BUILTIN_FUNCTIONS_COUNT = sizeof(BUILTIN_FUNCTIONS) / sizeof(FunctionSignature);

const char *BUILTIN_CONSTANTS[] = {
    "pi", "e"
};
const usize BUILTIN_CONSTANTS_COUNT = sizeof(BUILTIN_CONSTANTS) / sizeof(BUILTIN_CONSTANTS[0]);

bool is_builtin_function(String s) {
    for (usize i = 0; i < BUILTIN_FUNCTIONS_COUNT; i++) {
        if (!strcmp(s.str, BUILTIN_FUNCTIONS[i].name)) {
            return true;
        }
    }

    return false;
}

bool check_builtin_function_signature(String name, ASTArray args) {
    for (usize i = 0; i < BUILTIN_FUNCTIONS_COUNT; i++) {
        FunctionSignature signature = BUILTIN_FUNCTIONS[i];
        if (string_eq(init_string(signature.name), name) && signature.args_count == args.size) {
            // we found a signature that fits to the given one
            return true;
        } 
    }

    return false;
}

bool is_builtin_constant(String s) {
    for (usize i = 0; i < BUILTIN_CONSTANTS_COUNT; i++) {
        if (!strcmp(s.str, BUILTIN_CONSTANTS[i])) return true;
    }  
    return false;
}

//
// math
//

i64 factorial(i64 n) {
    i64 result = 1;
    for (i64 m = 1; m <= n; m++) {
        result *= m;
    }
    return result;
}

i64 gcd(i64 a, i64 b) {
    // Euclidean algorithm - https://en.wikipedia.org/wiki/Euclidean_algorithm
    
    while (b != 0) {
        i64 t = b;
        b = a % b;
        a = t;
    }

    return a;
}

bool ast_match(AST* left, AST* right) {

    if (left->type == right->type) {
        switch (left->type) {
            case AST_INTEGER:
                return left->integer.value == right->integer.value;
            case AST_REAL:
                return left->real.value == right->real.value;
            case AST_SYMBOL:
                return string_eq(left->symbol.name, right->symbol.name);
            case AST_CONSTANT:
                return string_eq(left->constant.name, right->constant.name);
            case AST_BINOP:
                return ast_match(left->binop.left, right->binop.left) && ast_match(left->binop.right, right->binop.right);
            case AST_CALL: {
                if (
                    string_eq(left->func_call.name, right->func_call.name) &&
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

String ast_to_debug_string(Allocator *allocator, AST* node) {
    String output = {0};
    output.str = alloc(allocator, 1024);
    switch (node->type) {

        case AST_INTEGER:
            sprintf(output.str, "%s(%lld)", ast_type_to_debug_string(node->type), node->integer.value);
            break;
        
        case AST_REAL:
            sprintf(output.str, "%s(%f)", ast_type_to_debug_string(node->type), node->real.value);
            break;
        
        case AST_SYMBOL:
            sprintf(output.str, "%s(%s)", ast_type_to_debug_string(node->type), node->symbol.name.str);
            break;
        
        case AST_BINOP: {
            const char *op_string = op_type_to_debug_string(node->binop.op);
            String left_string = ast_to_debug_string(allocator, node->binop.left);
            String right_string = ast_to_debug_string(allocator, node->binop.right);
            sprintf(output.str, "%s(%s, %s)", op_string, left_string.str, right_string.str);
            break;
        }
        
        case AST_UNARYOP: sprintf(output.str, "%s(%s)", op_type_to_debug_string(node->unaryop.op), ast_to_debug_string(allocator, node->unaryop.operand).str); break;
        
        // TODO: args to debug string
        case AST_CALL: {
            if (node->func_call.args.size == 1) {
                sprintf(output.str, "FuncCall(%s, %s)", node->func_call.name.str, ast_to_debug_string(allocator, node->func_call.args.data[0]).str); break;
            } else {
                // multiple args to string @todo
                sprintf(output.str, "FuncCall(%s, args)", node->func_call.name.str); break;
            }
        }

        case AST_EMPTY: sprintf(output.str, "Empty()"); break;
        
        default: fprintf(stderr, "ERROR: Cannot do 'ast_to_debug_string' because node type '%s' is not implemented.\n", ast_type_to_debug_string(node->type)); exit(1);
    
    }
    return output;
}

String _ast_to_string(Allocator *allocator, AST* node, u8 op_precedence) {
    // TODO: The use of String in here is very hacky and should be changed in the future.
    //       One of the problems here is that directly writing and accessing the memory
    //       in output.str doesn't affect the size. So without the hacks, the size of the
    //       result string will be 0 in the end.

    String output = {0};
    output.str = alloc(allocator, 1024);
    switch (node->type) {

        case AST_INTEGER: sprintf(output.str, "%lld", node->integer.value); break;
        
        case AST_REAL: sprintf(output.str, "%f", node->real.value); break;
        
        case AST_SYMBOL:
            sprintf(output.str, "%s", node->symbol.name.str); break;

        case AST_CONSTANT:
            sprintf(output.str, "%s", node->constant.name.str); break;
        
        case AST_BINOP: {

            uint8_t current_op_precedence = op_type_precedence(node->binop.op);

            String left_string = _ast_to_string(allocator, node->binop.left, current_op_precedence);
            String right_string = _ast_to_string(allocator, node->binop.right, current_op_precedence);
            const char *op_type_string = op_type_to_string(node->binop.op);

            if (current_op_precedence < op_precedence) {
                sprintf(output.str, "(%s%s%s)", left_string.str, op_type_string, right_string.str);
            } else {
                sprintf(output.str, "%s%s%s", left_string.str, op_type_string, right_string.str);
            }

            break;
        }

        case AST_UNARYOP: {
            uint8_t current_op_precedence = op_type_precedence(node->binop.op);

            String expr_string = _ast_to_string(allocator, node->unaryop.operand, op_precedence);
            const char *op_type_string = op_type_to_string(node->unaryop.op);

            if (current_op_precedence < op_precedence) {
                sprintf(output.str, "(%s%s)", op_type_string, expr_string.str); break;
            } else {
                sprintf(output.str, "%s%s", op_type_string, expr_string.str); break;
            }
            
        }

        case AST_CALL: {
            if (node->func_call.args.size == 1) {
                String arg_string = _ast_to_string(allocator, node->func_call.args.data[0], op_precedence);
                sprintf(output.str, "%s(%s)", node->func_call.name.str, arg_string.str); break;
            } else {
                // multiple args to string @todo
                sprintf(output.str, "%s(args)", node->func_call.name.str); break;
            }
        }

        case AST_EMPTY: break;
        
        default: fprintf(stderr, "ERROR: Cannot do 'ast_to_string' because node type '%s' is not implemented.\n", ast_type_to_debug_string(node->type)); exit(1);
    
    }

    // TODO: remove this when String has the functionality to remove the hacks in this function
    // 
    // determine the size of the string
    for (usize i = 0; i < 1024; i++) {
        if (output.str[i] == '\0') {
            output.size = i;
            break;
        }
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

    return init_ast_unaryop(ip->allocator, operand, op);
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
    if (ast_match(x, CONSTANT(init_string("pi")))) {
        return INTEGER(0);
    } else if (ast_is_numeric(x)) {
        f64 value = ast_to_f64(x);
        return interp(ip, REAL(sin(value)));
    }
    
    ASTArray args = {0};
    ast_array_append(ip->allocator, &args, x);
    return CALL(init_string("sin"), args);
}

AST *interp_cos(Interp *ip, AST* x) {    
    if (ast_match(x, INTEGER(0))) {
        return INTEGER(1);
    } else if (ast_match(x, DIV(CONSTANT(init_string("pi")), INTEGER(2)))) {
        return INTEGER(0);
    } else if (ast_match(x, CONSTANT(init_string("pi")))) {
        return INTEGER(-1);
    } else if (ast_is_numeric(x)) {
        f64 value = ast_to_f64(x);
        return interp(ip, REAL(cos(value)));
    }

    ASTArray args = {0};
    ast_array_append(ip->allocator, &args, x);
    return CALL(init_string("cos"), args);
}

AST *interp_tan(Interp *ip, AST* x) {
    if (ast_is_numeric(x)) {
        f64 value = ast_to_f64(x);
        return interp(ip, REAL(tan(value)));
    }
    
    ASTArray args = {0};
    ast_array_append(ip->allocator, &args, x);
    return CALL(init_string("tan"), args);
}

AST *interp_asin(Interp *ip, AST* x) {
    if (ast_is_numeric(x)) {
        f64 value = ast_to_f64(x);
        return interp(ip, REAL(asin(value)));
    }
    
    ASTArray args = {0};
    ast_array_append(ip->allocator, &args, x);
    return CALL(init_string("asin"), args);
}

AST *interp_acos(Interp *ip, AST* x) {
    if (ast_is_numeric(x)) {
        f64 value = ast_to_f64(x);
        return interp(ip, REAL(acos(value)));
    }
    
    ASTArray args = {0};
    ast_array_append(ip->allocator, &args, x);
    return CALL(init_string("acos"), args);
}

AST *interp_atan(Interp *ip, AST* x) {
    if (ast_is_numeric(x)) {
        f64 value = ast_to_f64(x);
        return interp(ip, REAL(atan(value)));
    }
    
    ASTArray args = {0};
    ast_array_append(ip->allocator, &args, x);
    return CALL(init_string("atan"), args);
}

AST *interp_abs(Interp *ip, AST* x) {
    if (ast_is_numeric(x)) {
        f64 value = ast_to_f64(x);
        if (value < 0) {
            return interp(ip, REAL(-value));
        } else {
            return interp(ip, REAL(value));
        }
    }
    
    ASTArray args = {0};
    ast_array_append(ip->allocator, &args, x);
    return CALL(init_string("abs"), args);
}

AST *interp_factorial(Interp *ip, AST *n) {
    // 0!
    if (ast_match(n, INTEGER(0))) {
        return INTEGER(1);
    }

    if (n->type == AST_INTEGER) {
        i64 value = n->integer.value;

        // TODO: error handling
        assert(value > 0);

        return interp(ip, INTEGER(factorial(value)));
    }
    
    ASTArray args = {0};
    ast_array_append(ip->allocator, &args, n);
    return CALL(init_string("factorial"), args);
}

AST *interp_npr(Interp *ip, AST *n, AST *k) {

    if (n->type == AST_INTEGER && k->type == AST_INTEGER) {
        i64 n_value = n->integer.value;
        i64 k_value = k->integer.value;
        
        // TODO: proper error handling
        assert(n_value >= 0);
        assert(k_value >= 0);
        assert(k_value <= n_value);

        return interp(ip, DIV(INTEGER(factorial(n_value)), INTEGER(factorial(n_value-k_value))));
    }

    ASTArray args = {0};
    ast_array_append(ip->allocator, &args, n);
    ast_array_append(ip->allocator, &args, k);
    return CALL(init_string("npr"), args);
}

AST *interp_ncr(Interp *ip, AST *n, AST *k) {

    if (n->type == AST_INTEGER && k->type == AST_INTEGER) {
        i64 n_value = n->integer.value;
        i64 k_value = k->integer.value;
        
        // TODO: proper error handling
        assert(n_value >= 0);
        assert(k_value >= 0);
        assert(k_value <= n_value);

        AST *nominator = INTEGER(factorial(n_value));
        AST *denominator = MUL(INTEGER(factorial(n_value-k_value)), INTEGER(factorial(k_value)));

        return interp(ip, DIV(nominator, denominator));
    }

    ASTArray args = {0};
    ast_array_append(ip->allocator, &args, n);
    ast_array_append(ip->allocator, &args, k);
    return CALL(init_string("ncr"), args);
}

AST *interp_gcd(Interp *ip, AST *a_ast, AST *b_ast) {
    // TODO: implement multiple args for this function (see python math module) @same20240626
    
    if (a_ast->type == AST_INTEGER && b_ast->type == AST_INTEGER) {
        return INTEGER(gcd(a_ast->integer.value, b_ast->integer.value));
    }

    ASTArray args = {0};
    ast_array_append(ip->allocator, &args, a_ast);
    ast_array_append(ip->allocator, &args, b_ast);
    return CALL(init_string("gcd"), args);
}

AST *interp_lcm(Interp *ip, AST *a_ast, AST *b_ast) {
    // TODO: implement multiple args for this function (see python math module) @same20240626

    if (a_ast->type == AST_INTEGER && b_ast->type == AST_INTEGER) {
        // LCM formula - https://en.wikipedia.org/wiki/Least_common_multiple
        i64 a = a_ast->integer.value;
        i64 b = b_ast->integer.value;
        i64 result = llabs(a*b) / gcd(a, b);
        return INTEGER(result);
    }

    ASTArray args = {0};
    ast_array_append(ip->allocator, &args, a_ast);
    ast_array_append(ip->allocator, &args, b_ast);
    return CALL(init_string("lcm"), args);
}

AST *interp_log(Interp *ip, AST *y, AST *b) {
    // log_b(y) = x
    // b^x = y
    if (ast_match(y, INTEGER(1))) {
        return INTEGER(0);
    } else if (ast_match(y, b)) {
        return INTEGER(1);
    }

    // compute
    if (ast_is_numeric(y) && ast_is_numeric(b)) {
        f64 y_value = ast_to_f64(y);
        f64 b_value = ast_to_f64(b);
        
        assert(y_value > 0.0);
        // TODO: which values are allowed for b?

        // TODO: for common bases like e, 2, 10 we dont need base change because they are basically implemented
        //       in math.h. So probably we have not the best precision right now, because we always use the
        //       base change formula.
        //
        // base change
        AST *result = REAL(log(y_value) / log(b_value));
        return interp(ip, result);
    }

    // TODO: implement maybe variadic function or macro to simplify ast array
    //       creation from with args given.
    ASTArray args = {0};
    ast_array_append(ip->allocator, &args, y);
    ast_array_append(ip->allocator, &args, b);
    return CALL(init_string("log"), args);
}

AST* interp_sqrt(Interp *ip, AST *x) {
    // check for perfect square for simplifying
    //
    // TODO: this is probably an inefficient way to compute the biggest perfect sqaure factor of a given number
    if (x->type == AST_INTEGER) {
        for (i32 q = x->integer.value; q > 1; q--) {
            double sqrt_of_q = sqrtf((double)q);
            bool is_perfect_square = sqrt_of_q == floor(sqrt_of_q);
            if (is_perfect_square && x->integer.value % q == 0) {
                i32 p = x->integer.value / q;
                ASTArray new_args = {0};
                ast_array_append(ip->allocator, &new_args, INTEGER(p));
                AST *result = MUL(INTEGER((i64)sqrt_of_q), CALL(init_string("sqrt"), new_args));
                return interp(ip, result);
            }
        }
    }

    // sqrt(1)
    if (ast_match(x, INTEGER(1))) {
        return INTEGER(1);
    }

    // compute
    // if (ast_is_numeric(x)) {
    //     f64 value = ast_to_f64(x);
    //     return interp(ip, REAL(sqrt(value)));
    // }

    ASTArray args = {0};
    ast_array_append(ip->allocator, &args, x);
    return CALL(init_string("sqrt"), args);
}

AST* interp_call(Interp *ip, String name, ASTArray args) {

    for (usize i = 0; i < args.size; i++) {
        args.data[i] = interp(ip, args.data[i]);
    }

    // check for builtin function and right signature
    if (is_builtin_function(name)) {
        bool success = check_builtin_function_signature(name, args);
        if (!success) {
            panic("Wrong amount of arguments.");
        }
    }
    // Since here we know that the amount of args for any builtin
    // function is right so we dont need any further checks for now.
    // Maybe this will change in the future if we introduce any kind
    // of typing in the function signatures.

    if (string_eq(name, init_string("sqrt"))) {
        return interp_sqrt(ip, args.data[0]);
    } else if (string_eq(name, init_string("ln"))) {
        return interp_log(ip, args.data[0], CONSTANT(init_string("e")));
    } else if (string_eq(name, init_string("log"))) {
        return interp_log(ip, args.data[0], args.data[1]);
    } else if (string_eq(name, init_string("sin"))) {
        return interp_sin(ip, args.data[0]);
    } else if (string_eq(name, init_string("cos"))) {
        return interp_cos(ip, args.data[0]);
    } else if (string_eq(name, init_string("tan"))) {
        return interp_tan(ip, args.data[0]);
    } else if (string_eq(name, init_string("asin"))) {
        return interp_asin(ip, args.data[0]);
    } else if (string_eq(name, init_string("acos"))) {
        return interp_acos(ip, args.data[0]);
    } else if (string_eq(name, init_string("atan"))) {
        return interp_atan(ip, args.data[0]);
    } else if (string_eq(name, init_string("abs"))) {
        return interp_abs(ip, args.data[0]);
    } else if (string_eq(name, init_string("factorial"))) {
        return interp_factorial(ip, args.data[0]);
    } else if (string_eq(name, init_string("npr"))) {
        return interp_npr(ip, args.data[0], args.data[1]);
    } else if (string_eq(name, init_string("ncr"))) {
        return interp_ncr(ip, args.data[0], args.data[1]);
    } else if (string_eq(name, init_string("gcd"))) {
        return interp_gcd(ip, args.data[0], args.data[1]);
    } else if (string_eq(name, init_string("lcm"))) {
        return interp_lcm(ip, args.data[0], args.data[1]);
    } else if (string_eq(name, init_string("pow"))) {
        return interp(ip, POW(args.data[0], args.data[1]));
    } else if (string_eq(name, init_string("exp"))) {
        return interp(ip, POW(CONSTANT(init_string("e")), args.data[0]));
    } else if (string_eq(name, init_string("diff"))) {
        AST* diff_var;

        if (args.size == 1) {
            ASTArray nodes = ast_to_flat_array(ip->allocator, args.data[0]);
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

AST *interp_symbol(Interp *ip, String name) {
    // here comes the check for defined variables in the future
    if (is_builtin_constant(name)) {
        return CONSTANT(name);
    }

    return SYMBOL(name);
}

AST *interp(Interp *ip, AST* node) {
    switch (node->type) {
        case AST_BINOP:
            return interp_binop(ip, node->binop.left, node->binop.right, node->binop.op);
        case AST_UNARYOP:
            return interp_unaryop(ip, node->unaryop.op, node->unaryop.operand);
        case AST_INTEGER:
            return node;
        case AST_SYMBOL:
            return interp_symbol(ip, node->symbol.name);
        case AST_CONSTANT:
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
