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
        AST* ast = parse(&parser); \
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
    TEST_SOURCE_TO_INTERP("3^2^3", create_ast_integer(6561));
    TEST_SOURCE_TO_INTERP("3^(2^3)", create_ast_integer(6561));
    TEST_SOURCE_TO_INTERP("(3^2)^3", create_ast_integer(729));
    TEST_SOURCE_TO_INTERP("3^4 - (-100)^2", create_ast_integer(-9919));
    TEST_SOURCE_TO_INTERP("-3^2", create_ast_integer(-9));
    TEST_SOURCE_TO_INTERP("(-3)^2", create_ast_integer(9));
    TEST_SOURCE_TO_INTERP("-2*-3", create_ast_integer(6));
    TEST_SOURCE_TO_INTERP("2*-3", create_ast_integer(-6));
    TEST_SOURCE_TO_INTERP("cos(pi)^2+1", create_ast_integer(2));
    TEST_SOURCE_TO_INTERP("x^0", create_ast_integer(1));
    TEST_SOURCE_TO_INTERP("x^1", parse_from_string("x"));
    TEST_SOURCE_TO_INTERP("1/8 * (-4-2*4/3-1)", interp_from_string("-23/24"));
    TEST_SOURCE_TO_INTERP("a+0", interp_from_string("a"));
    TEST_SOURCE_TO_INTERP("0+a", interp_from_string("a"));
    TEST_SOURCE_TO_INTERP("a-0", interp_from_string("a"));
    TEST_SOURCE_TO_INTERP("a*1", interp_from_string("a"));

    // auto generated
    TEST_SOURCE_TO_INTERP("48+-75", create_ast_integer(-27));
    TEST_SOURCE_TO_INTERP("(-70+-98)", create_ast_integer(-168));
    TEST_SOURCE_TO_INTERP("-19-10", create_ast_integer(-29));
    TEST_SOURCE_TO_INTERP("(29-43)", create_ast_integer(-14));
    TEST_SOURCE_TO_INTERP("(50*41)", create_ast_integer(2050));
    TEST_SOURCE_TO_INTERP("-71+-73", create_ast_integer(-144));
    TEST_SOURCE_TO_INTERP("20*-67", create_ast_integer(-1340));
    TEST_SOURCE_TO_INTERP("(27+97)", create_ast_integer(124));
    TEST_SOURCE_TO_INTERP("(60*-55)", create_ast_integer(-3300));
    TEST_SOURCE_TO_INTERP("(-97--37)", create_ast_integer(-60));
    TEST_SOURCE_TO_INTERP("20*-74", create_ast_integer(-1480));
    TEST_SOURCE_TO_INTERP("45*-45", create_ast_integer(-2025));
    TEST_SOURCE_TO_INTERP("(7*63)", create_ast_integer(441));
    TEST_SOURCE_TO_INTERP("-11*-51", create_ast_integer(561));
    TEST_SOURCE_TO_INTERP("(94*64)", create_ast_integer(6016));
    TEST_SOURCE_TO_INTERP("-57*-78", create_ast_integer(4446));
    TEST_SOURCE_TO_INTERP("27*17", create_ast_integer(459));
    TEST_SOURCE_TO_INTERP("-50*75", create_ast_integer(-3750));
    TEST_SOURCE_TO_INTERP("-81*-36", create_ast_integer(2916));
    TEST_SOURCE_TO_INTERP("-37+22", create_ast_integer(-15));
    TEST_SOURCE_TO_INTERP("-29*50", create_ast_integer(-1450));
    TEST_SOURCE_TO_INTERP("-93-4", create_ast_integer(-97));
    TEST_SOURCE_TO_INTERP("(-50--3)", create_ast_integer(-47));
    TEST_SOURCE_TO_INTERP("60*-57", create_ast_integer(-3420));
    TEST_SOURCE_TO_INTERP("-21+43", create_ast_integer(22));
    TEST_SOURCE_TO_INTERP("50*90", create_ast_integer(4500));
    TEST_SOURCE_TO_INTERP("34*58", create_ast_integer(1972));
    TEST_SOURCE_TO_INTERP("(-92-35)", create_ast_integer(-127));
    TEST_SOURCE_TO_INTERP("(34+-4)", create_ast_integer(30));
    TEST_SOURCE_TO_INTERP("(-84+-49)", create_ast_integer(-133));
    TEST_SOURCE_TO_INTERP("(48+85)", create_ast_integer(133));
    TEST_SOURCE_TO_INTERP("(90+-31)", create_ast_integer(59));
    TEST_SOURCE_TO_INTERP("-96+53", create_ast_integer(-43));
    TEST_SOURCE_TO_INTERP("(-21-22)", create_ast_integer(-43));
    TEST_SOURCE_TO_INTERP("(-11*-6)", create_ast_integer(66));
    TEST_SOURCE_TO_INTERP("-15*-65", create_ast_integer(975));
    TEST_SOURCE_TO_INTERP("-65--97", create_ast_integer(32));
    TEST_SOURCE_TO_INTERP("-74--21", create_ast_integer(-53));
    TEST_SOURCE_TO_INTERP("(-64+-58)", create_ast_integer(-122));
    TEST_SOURCE_TO_INTERP("(94-83)", create_ast_integer(11));
    TEST_SOURCE_TO_INTERP("22--75", create_ast_integer(97));
    TEST_SOURCE_TO_INTERP("-86-48", create_ast_integer(-134));
    TEST_SOURCE_TO_INTERP("-50+-43", create_ast_integer(-93));
    TEST_SOURCE_TO_INTERP("-50--2", create_ast_integer(-48));
    TEST_SOURCE_TO_INTERP("(81+1)", create_ast_integer(82));
    TEST_SOURCE_TO_INTERP("-11-83", create_ast_integer(-94));
    TEST_SOURCE_TO_INTERP("25+-43", create_ast_integer(-18));
    TEST_SOURCE_TO_INTERP("-22--18", create_ast_integer(-4));
    TEST_SOURCE_TO_INTERP("(-45*-23)", create_ast_integer(1035));
    TEST_SOURCE_TO_INTERP("(-81+0)", create_ast_integer(-81));
    TEST_SOURCE_TO_INTERP("(-52-80)", create_ast_integer(-132));
    TEST_SOURCE_TO_INTERP("-8^8", create_ast_integer(-16777216));
    TEST_SOURCE_TO_INTERP("(-29*-17)", create_ast_integer(493));
    TEST_SOURCE_TO_INTERP("(-24--40)", create_ast_integer(16));
}

int main(int argc, char *argv[]) {
    bool do_test = false;
    bool do_cli = true;
    bool do_gui = false;

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--test")) {
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

        // printf("%s\n", ast_to_debug_string(output));
        char* input = "ln(3)";
        Tokens tokens = tokenize(input);
        tokens_print(&tokens);

        AST* output = interp_from_string(input);
        printf("%s\n", ast_to_string(output));
        printf("%s\n", ast_to_debug_string(output));
        // printf("%s\n", ast_to_debug_string(interp_from_string("-23/24")));
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
