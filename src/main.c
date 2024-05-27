#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#include <math.h>

#include "casc.h"

Allocator create_allocator(size_t size) {
    return (Allocator){ .memory=malloc(size), .head=0 };
}

void allocator_free(Allocator *allocator) {
    if (allocator->memory != NULL) {
        free(allocator->memory);
    }
    allocator->memory = NULL;
    allocator->head = 0;
}

void *mmalloc(Allocator *allocator, size_t size) {
    void *memory = allocator->memory + allocator->head;
    allocator->head = allocator->head + size;
    return memory;
}

static size_t test_counter = 1;

void test_ast(char *source, AST *test_source) {
    Lexer lexer;
    lexer.source = source;
    lexer.pos = 0;

    Parser parser;
    parser.lexer = lexer;

    AST* output = parse(&parser);
    output = interp(output);

    printf("test %02zu ... ", test_counter);
    
    if (!ast_match(output, test_source)) {
        printf("FAILED\n");
        printf("%s\n", ast_to_string(output));
        printf("!=\n");
        printf("%s\n", ast_to_string(test_source));
        // exit(1);
    } else { 
        printf("OK\n");
    }

    test_counter += 1;
}

void test() {
    // ruslanspivak interpreter tutorial tests
    test_ast("27 + 3", create_ast_integer(30));
    test_ast("27 - 7", create_ast_integer(20));
    test_ast("7 - 3 + 2 - 1", create_ast_integer(5));
    test_ast("10 + 1 + 2 - 3 + 4 + 6 - 15", create_ast_integer(5));
    test_ast("7 * 4 / 2", create_ast_integer(14));
    test_ast("7 * 4 / 2 * 3", create_ast_integer(42));
    test_ast("10 * 4  * 2 * 3 / 8", create_ast_integer(30));
    test_ast("7 - 8 / 4", create_ast_integer(5));
    test_ast("14 + 2 * 3 - 6 / 2", create_ast_integer(17));
    test_ast("7 + 3 * (10 / (12 / (3 + 1) - 1))", create_ast_integer(22));
    test_ast("7 + 3 * (10 / (12 / (3 + 1) - 1)) / (2 + 3) - 5 - 3 + (8)", create_ast_integer(10));
    test_ast("7 + (((3 + 2)))", create_ast_integer(12));
    test_ast("- 3", create_ast_integer(-3));
    test_ast("+ 3", create_ast_integer(3));
    test_ast("5 - - - + - 3", create_ast_integer(8));
    test_ast("5 - - - + - (3 + 4) - +2", create_ast_integer(10));

    // things from the sympy tutorial
    test_ast("sqrt(8)", parse_from_string("2*sqrt(2)"));
    // TEST_SOURCE_TO_INTERP("sqrt(8)", create_ast_binop(create_ast_integer(2), create_ast_func_call((Token){ .text="sqrt" }, create_ast_integer(2)), OP_MUL));

    // misc
    test_ast("sqrt(9)", create_ast_integer(3));
    test_ast("1/3 * sin(pi) - cos(pi/2) / cos(pi) + 54/2 * sqrt(9)", create_ast_integer(81));
    test_ast("3^2^3", create_ast_integer(6561));
    test_ast("3^(2^3)", create_ast_integer(6561));
    test_ast("(3^2)^3", create_ast_integer(729));
    test_ast("3^4 - (-100)^2", create_ast_integer(-9919));
    test_ast("-3^2", create_ast_integer(-9));
    test_ast("(-3)^2", create_ast_integer(9));
    test_ast("-2*-3", create_ast_integer(6));
    test_ast("2*-3", create_ast_integer(-6));
    test_ast("cos(pi)^2+1", create_ast_integer(2));
    test_ast("x^0", create_ast_integer(1));
    test_ast("x^1", parse_from_string("x"));
    test_ast("1/8 * (-4-2*4/3-1)", interp_from_string("-23/24"));
    test_ast("a+0", interp_from_string("a"));
    test_ast("0+a", interp_from_string("a"));
    test_ast("a-0", interp_from_string("a"));
    test_ast("a*1", interp_from_string("a"));
    test_ast("ln(e)", interp_from_string("1"));
    test_ast("ln(1)", interp_from_string("0"));
    test_ast("log(1, 10)", interp_from_string("0"));
    test_ast("log(10, 10)", interp_from_string("1"));

    // auto generated
    test_ast("48+-75", create_ast_integer(-27));
    test_ast("(-70+-98)", create_ast_integer(-168));
    test_ast("-19-10", create_ast_integer(-29));
    test_ast("(29-43)", create_ast_integer(-14));
    test_ast("(50*41)", create_ast_integer(2050));
    test_ast("-71+-73", create_ast_integer(-144));
    test_ast("20*-67", create_ast_integer(-1340));
    test_ast("(27+97)", create_ast_integer(124));
    test_ast("(60*-55)", create_ast_integer(-3300));
    test_ast("(-97--37)", create_ast_integer(-60));
    test_ast("20*-74", create_ast_integer(-1480));
    test_ast("45*-45", create_ast_integer(-2025));
    test_ast("(7*63)", create_ast_integer(441));
    test_ast("-11*-51", create_ast_integer(561));
    test_ast("(94*64)", create_ast_integer(6016));
    test_ast("-57*-78", create_ast_integer(4446));
    test_ast("27*17", create_ast_integer(459));
    test_ast("-50*75", create_ast_integer(-3750));
    test_ast("-81*-36", create_ast_integer(2916));
    test_ast("-37+22", create_ast_integer(-15));
    test_ast("-29*50", create_ast_integer(-1450));
    test_ast("-93-4", create_ast_integer(-97));
    test_ast("(-50--3)", create_ast_integer(-47));
    test_ast("60*-57", create_ast_integer(-3420));
    test_ast("-21+43", create_ast_integer(22));
    test_ast("50*90", create_ast_integer(4500));
    test_ast("34*58", create_ast_integer(1972));
    test_ast("(-92-35)", create_ast_integer(-127));
    test_ast("(34+-4)", create_ast_integer(30));
    test_ast("(-84+-49)", create_ast_integer(-133));
    test_ast("(48+85)", create_ast_integer(133));
    test_ast("(90+-31)", create_ast_integer(59));
    test_ast("-96+53", create_ast_integer(-43));
    test_ast("(-21-22)", create_ast_integer(-43));
    test_ast("(-11*-6)", create_ast_integer(66));
    test_ast("-15*-65", create_ast_integer(975));
    test_ast("-65--97", create_ast_integer(32));
    test_ast("-74--21", create_ast_integer(-53));
    test_ast("(-64+-58)", create_ast_integer(-122));
    test_ast("(94-83)", create_ast_integer(11));
    test_ast("22--75", create_ast_integer(97));
    test_ast("-86-48", create_ast_integer(-134));
    test_ast("-50+-43", create_ast_integer(-93));
    test_ast("-50--2", create_ast_integer(-48));
    test_ast("(81+1)", create_ast_integer(82));
    test_ast("-11-83", create_ast_integer(-94));
    test_ast("25+-43", create_ast_integer(-18));
    test_ast("-22--18", create_ast_integer(-4));
    test_ast("(-45*-23)", create_ast_integer(1035));
    test_ast("(-81+0)", create_ast_integer(-81));
    test_ast("(-52-80)", create_ast_integer(-132));
    test_ast("-8^8", create_ast_integer(-16777216));
    test_ast("(-29*-17)", create_ast_integer(493));
    test_ast("(-24--40)", create_ast_integer(16));

    printf("\n\n");
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
        printf("casc\n");
        char *source = "5 - - - + - (3 + 4) - +2";
        AST *output = interp_from_string(source);
        printf("%s\n", ast_to_debug_string(output));
    } else if (do_gui) {
        init_gui();
    } else {
        assert(false);
        // unreachable
    }

    return 0;
}
