#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#include <math.h>

#include "casc.h"

#define ARENA_SIZE 60*1024

Arena create_arena(size_t size) {
    return (Arena){ .memory=malloc(size), .offset=0, .size=size };
}

void arena_free(Arena *arena) {
    // maybe we need a arena_reset function too. This would reset the arena
    // to offset 0 but not free the memory. So the same arena can be used again
    // without a new malloc.
    if (arena->memory != NULL) {
        free(arena->memory);
    }
    arena->memory = NULL;
    arena->offset = 0;
}

void *arena_alloc(Arena *arena, size_t size) {
    // current position in the memory
    void *memory = arena->memory + arena->offset;

    // compute the new position in the memory
    arena->offset = arena->offset + size;

    double arena_memory_used = (double)arena->offset / (double)arena->size;
    assert(arena_memory_used < 0.5);
    assert(arena->offset < arena->size);
#if 0
    printf("arena_memory_used = %.2f\n", arena_memory_used);
#endif

    return memory;
}

Worker create_worker() {
    Worker w;
    w.arena = create_arena(ARENA_SIZE);
    w.lexer.pos = 0;
    return w;
}

void test_ast(char *source, char *test_source) {

    static size_t test_counter = 1;

    Worker _w = create_worker();
    Worker *w = &_w;

    w->lexer.source = source;

    AST* output = parse(w);

    output = interp(w, output);

    printf("test %02zu ... ", test_counter);

    char *output_string = ast_to_string(output);
    
    if (strcmp(output_string, test_source) != 0) {
        printf("FAILED\n");

#if 0
        printf("source='%s'\n", source);
#endif
        
        printf("'%s'\n", output_string);
        printf("!=\n");
        printf("'%s'\n", test_source);
        // exit(1);
    } else { 
        printf("OK\n");
    }

    arena_free(&w->arena);

    test_counter += 1;
}

void test() {
    // ruslanspivak interpreter tutorial tests
    test_ast("27 + 3", "30");
    test_ast("27 - 7", "20");
    test_ast("7 - 3 + 2 - 1", "5");
    test_ast("10 + 1 + 2 - 3 + 4 + 6 - 15", "5");
    test_ast("7 * 4 / 2", "14");
    test_ast("7 * 4 / 2 * 3", "42");
    test_ast("10 * 4  * 2 * 3 / 8", "30");
    test_ast("7 - 8 / 4", "5");
    test_ast("14 + 2 * 3 - 6 / 2", "17");
    test_ast("7 + 3 * (10 / (12 / (3 + 1) - 1))", "22");
    test_ast("7 + 3 * (10 / (12 / (3 + 1) - 1)) / (2 + 3) - 5 - 3 + (8)", "10");
    test_ast("7 + (((3 + 2)))", "12");
    test_ast("- 3", "-3");
    test_ast("+ 3", "3");
    test_ast("5 - - - + - 3", "8");
    test_ast("5 - - - + - (3 + 4) - +2", "10");

    // things from the sympy tutorial
    test_ast("sqrt(8)", "2*sqrt(2)");
    // TEST_SOURCE_TO_INTERP("sqrt(8)", create_ast_binop(create_ast_integer(2), create_ast_func_call((Token){ .text="sqrt" }, create_ast_integer(2)), OP_MUL));

    // misc
    test_ast("sqrt(9)", "3");
    test_ast("1/3 * sin(pi) - cos(pi/2) / cos(pi) + 54/2 * sqrt(9)", "81");
    test_ast("3^2^3", "6561");
    test_ast("3^(2^3)", "6561");
    test_ast("(3^2)^3", "729");
    test_ast("3^4 - (-100)^2", "-9919");
    test_ast("-3^2", "-9");
    test_ast("(-3)^2", "9");
    test_ast("-2*-3", "6");
    test_ast("2*-3", "-6");
    test_ast("cos(pi)^2+1", "2");
    test_ast("x^0", "1");
    test_ast("x^1", "x");
    test_ast("1/8 * (-4-2*4/3-1)", "-23/24");
    test_ast("a+0", "a");
    test_ast("0+a", "a");
    test_ast("a-0", "a");
    test_ast("a*1", "a");
    test_ast("ln(e)", "1");
    test_ast("ln(1)", "0");
    test_ast("log(1, 10)", "0");
    test_ast("log(10, 10)", "1");
    test_ast("-x^2", "-x^2");
    test_ast("(-x)^2", "(-x)^2");

    // auto generated
    test_ast("48+-75", "-27");
    test_ast("(-70+-98)", "-168");
    test_ast("-19-10", "-29");
    test_ast("(29-43)", "-14");
    test_ast("(50*41)", "2050");
    test_ast("-71+-73", "-144");
    test_ast("20*-67", "-1340");
    test_ast("(27+97)", "124");
    test_ast("(60*-55)", "-3300");
    test_ast("(-97--37)", "-60");
    test_ast("20*-74", "-1480");
    test_ast("45*-45", "-2025");
    test_ast("(7*63)", "441");
    test_ast("-11*-51", "561");
    test_ast("(94*64)", "6016");
    test_ast("-57*-78", "4446");
    test_ast("27*17", "459");
    test_ast("-50*75", "-3750");
    test_ast("-81*-36", "2916");
    test_ast("-37+22", "-15");
    test_ast("-29*50", "-1450");
    test_ast("-93-4", "-97");
    test_ast("(-50--3)", "-47");
    test_ast("60*-57", "-3420");
    test_ast("-21+43", "22");
    test_ast("50*90", "4500");
    test_ast("34*58", "1972");
    test_ast("(-92-35)", "-127");
    test_ast("(34+-4)", "30");
    test_ast("(-84+-49)", "-133");
    test_ast("(48+85)", "133");
    test_ast("(90+-31)", "59");
    test_ast("-96+53", "-43");
    test_ast("(-21-22)", "-43");
    test_ast("(-11*-6)", "66");
    test_ast("-15*-65", "975");
    test_ast("-65--97", "32");
    test_ast("-74--21", "-53");
    test_ast("(-64+-58)", "-122");
    test_ast("(94-83)", "11");
    test_ast("22--75", "97");
    test_ast("-86-48", "-134");
    test_ast("-50+-43", "-93");
    test_ast("-50--2", "-48");
    test_ast("(81+1)", "82");
    test_ast("-11-83", "-94");
    test_ast("25+-43", "-18");
    test_ast("-22--18", "-4");
    test_ast("(-45*-23)", "1035");
    test_ast("(-81+0)", "-81");
    test_ast("(-52-80)", "-132");
    test_ast("-8^8", "-16777216");
    test_ast("(-29*-17)", "493");
    test_ast("(-24--40)", "16");

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
        Worker _w = create_worker();
        Worker *w = &_w;

        char *source = "(-x)^2";
        AST *output = interp_from_string(w, source);

        printf("%s\n", ast_to_string(output));
        printf("%s\n", ast_to_debug_string(output));

        arena_free(&w->arena);
    } else if (do_gui) {
        init_gui();
    } else {
        assert(false);
        // unreachable
    }

    return 0;
}
