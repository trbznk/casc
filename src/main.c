#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#include <math.h>

#include "parser.h"
#include "interp.h"
#include "gui.h"

void test() {
    size_t test_id = 1;

    #define TEST_SOURCE_TO_PARSE(source, test_case) {{ \
        Tokens tokens = tokenize(source); \
        Parser parser = { .tokens = tokens, .pos=0 }; \
        AST* ast = parse_expr(&parser); \
        printf("test %02zu...........................", test_id);\
        if (!ast_match(ast, test_case)) {\
            printf("failed\n");\
            printf("%s\n", ast_to_string(ast));\
            printf("!=\n");\
            printf("%s\n", ast_to_string(test_case));\
            exit(1); \
        } else { \
            printf("passed\n");\
        }\
        test_id++;\
    }}

    #define TEST_SOURCE_TO_INTERP(source, test_case) {{ \
        Tokens tokens = tokenize(source); \
        Parser parser = { .tokens = tokens, .pos=0 }; \
        AST* ast = parse_expr(&parser); \
        AST* output = interp(ast); \
        printf("test %02zu...........................", test_id);\
        if (!ast_match(output, test_case)) {\
            printf("failed\n");\
            printf("%s\n", ast_to_string(output));\
            printf("!=\n");\
            printf("%s\n", ast_to_string(test_case));\
            exit(1); \
        } else { \
            printf("passed\n");\
        }\
        test_id++;\
    }}

    // ruslanspivak interpreter tutorial tests
    TEST_SOURCE_TO_INTERP("27 + 3", create_ast_integer(30));
    TEST_SOURCE_TO_INTERP("27 - 7", create_ast_integer(20));
    TEST_SOURCE_TO_INTERP("7 - 3 + 2 - 1", create_ast_integer(5));
    TEST_SOURCE_TO_INTERP("10 + 1 + 2 - 3 + 4 + 6 - 15", create_ast_integer(5));
    TEST_SOURCE_TO_INTERP("7 * 4 / 2", create_ast_integer(14));
    TEST_SOURCE_TO_INTERP("7 * 4 / 2 * 3", create_ast_integer(42));
    TEST_SOURCE_TO_INTERP("10 * 4  * 2 * 3 / 8", create_ast_integer(30));
    TEST_SOURCE_TO_INTERP("7 - 8 / 4", create_ast_integer(5));
    TEST_SOURCE_TO_INTERP("14 + 2 * 3 - 6 / 2", create_ast_integer(17));
    TEST_SOURCE_TO_INTERP("7 + 3 * (10 / (12 / (3 + 1) - 1))", create_ast_integer(22));
    TEST_SOURCE_TO_INTERP("7 + 3 * (10 / (12 / (3 + 1) - 1)) / (2 + 3) - 5 - 3 + (8)", create_ast_integer(10));
    TEST_SOURCE_TO_INTERP("7 + (((3 + 2)))", create_ast_integer(12));
    TEST_SOURCE_TO_INTERP("- 3", create_ast_integer(-3));
    TEST_SOURCE_TO_INTERP("+ 3", create_ast_integer(3));
    TEST_SOURCE_TO_INTERP("5 - - - + - 3", create_ast_integer(8));
    TEST_SOURCE_TO_INTERP("5 - - - + - (3 + 4) - +2", create_ast_integer(10));

    // things from the sympy tutorial
    TEST_SOURCE_TO_PARSE("sqrt(3)", create_ast_func_call((Token){ .text="sqrt" }, create_ast_integer(3)));
    TEST_SOURCE_TO_INTERP("sqrt(8)", create_ast_binop(create_ast_integer(2), create_ast_func_call((Token){ .text="sqrt" }, create_ast_integer(2)), OP_MUL));

    // misc
    TEST_SOURCE_TO_INTERP("sqrt(9)", create_ast_integer(3));
    TEST_SOURCE_TO_INTERP("1/3 * sin(pi) - cos(pi/2) / cos(pi) + 54/2 * sqrt(9)", create_ast_integer(81));
}

int main(int argc, char *argv[]) {
    bool do_test = false;
    bool do_cli = true;
    bool do_gui = false;

    for (int i = 0; i < argc; i++) {
        if (i == 0) {
            assert(!strcmp(argv[i], "./casc"));
        } else if (!strcmp(argv[i], "--test")) {
            do_test = true;
        } else if (!strcmp(argv[i], "--gui")) {
            do_gui = true;
            do_cli = false;
        } else {
            fprintf(stderr, "ERROR: Parse wrong argument '%s'.\n", argv[i]);
            exit(1);
        }
    }
    assert(do_cli != do_gui);

    if (do_test) {
        test();
    }

    if (do_cli) {
        // ast_match_type()
        AST* output = interp_from_string("1/3 * sin(pi) - cos(pi/2) / cos(pi) + 54/2 * sqrt(9)"); // = 81 (desmos)
        printf("%s\n", ast_to_string(output));
        printf("%s\n", ast_to_debug_string(output));
        // bool r = ast_match_type(output, parse_from_string("1/1 + 1/1"));
        // printf("r=%d\n", r);
        
    } else if (do_gui) {
        init_gui();
    } else {
        assert(false);
        // unreachable
    }

    return 0;
}
