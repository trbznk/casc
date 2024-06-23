// TODO: Do we really want our own String struct to be extended to a null terminated
//       string? In the past we have some memory errors because it's very easy to forget
//       this if we implement new functions for strings.

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#include <math.h>

#include "casc.h"

#define ARENA_SIZE 1024
#define TEST_EARLY_STOP true

Allocator init_allocator() {
    Allocator a = {0};
    return a;
}

void *alloc(Allocator *allocator, usize size) {
    // bring size up to a power of 2 to have proper alignment
    usize padding = 64;
    if (size % padding != 0) {
        size = size+(padding-size%padding);
    }
    assert(size > 0);
    assert(size % 32 == 0);
    assert(size <= ARENA_SIZE);

    // check if we have a arena
    if (allocator->arena == NULL) {
        // TODO: duplicate code here @1
        allocator->arena = calloc(1, sizeof(Arena)); // to ensure zero initialization
        allocator->arena->memory = malloc(ARENA_SIZE);
        allocator->arena->size = ARENA_SIZE;
        assert(allocator->arena->memory != NULL);
    }

    // compute free capacity of current arena
    usize free_capacity = allocator->arena->size - allocator->arena->offset;

    // choose arena
    if (free_capacity < size) {
        Arena *last_arena = allocator->arena;
        // TODO: duplicate code here @1
        allocator->arena = calloc(1, sizeof(Arena));
        allocator->arena->memory = malloc(ARENA_SIZE);
        allocator->arena->size = ARENA_SIZE;
        allocator->arena->prev = last_arena; // difference @1
        assert(allocator->arena->memory != NULL);
    }
    Arena *arena = allocator->arena;

    // give memory
    void *memory = arena->memory + arena->offset;
    arena->offset += size;
    return memory;
}

void free_allocator(Allocator *allocator) {
    Arena *arena = allocator->arena;
    while (arena != NULL) {
        Arena *prev_arena = arena->prev;
        free(arena->memory);
        free(arena);
        if (prev_arena != NULL) {
            arena = prev_arena;
        } else {
            // TODO: maybe we dont need this because free ensures that arena is NULL now
            arena = NULL; 
        }
    }
}

String init_string(const char *str) {
    String s = {0};
    s.size = strlen(str);
    s.str = (char*)str;
    return s;
}

String char_to_string(Allocator *allocator, char c) {
    String s = {0};
    s.size = 1;
    s.str = alloc(allocator, 2);
    s.str[0] = c;
    s.str[1] = '\0';
    return s;
}

String string_slice(Allocator *allocator, String s, usize start, usize stop) {
    String new_s = {0};
    new_s.size = stop-start;
    new_s.str = alloc(allocator, new_s.size+1);
    strncpy(new_s.str, &s.str[start], new_s.size);
    new_s.str[new_s.size] = '\0';
    return new_s;
}

String string_concat(Allocator *allocator, String s1, String s2) {
    String s = {0};
    s.size = s1.size+s2.size;
    s.str = alloc(allocator, s.size+1);
    strncpy(s.str, s1.str, s1.size);
    strncpy(&s.str[s1.size], s2.str, s2.size);
    s.str[s.size] = '\0';
    return s;
}

String string_insert(Allocator *allocator, String s1, String s2, usize idx) {
    String prefix = string_slice(allocator, s1, 0, idx);
    String postfix = string_slice(allocator, s1, idx, s1.size);
    String new_s = string_concat(allocator, prefix, s2);
    // null termination of new_s.str should be already done in string_concat
    new_s = string_concat(allocator, new_s, postfix);
    return new_s;
}

bool string_eq(String s1, String s2) {
    if (s1.size != s2.size) {
        return false;
    }

    for (usize i = 0; i < s1.size; i++) {
        if (s1.str[i] != s2.str[i]) {
            return false;
        }
    }
    return true;
}

void print(String s) {
    for (usize i = 0; i < s.size; i++) {
        printf("%c", s.str[i]);
    }
    printf("\n");
}

void _test_ast(u32 line_number, String source, String test_source) {
    Allocator allocator = init_allocator();

    Lexer lexer = {0};
    lexer.source = source;
    lexer.allocator = &allocator;

    Interp ip = {0};
    ip.allocator = &allocator;

    AST* output = parse(&lexer);
    output = interp(&ip, output);

    printf("test:%d ... ", line_number);

    String output_string = ast_to_string(ip.allocator, output);
    if (!string_eq(output_string, test_source)) {
        printf("FAILED\n");
        printf("\nPARAMETERS:\n");
        printf("source:\n");
        print(source);
        printf("test_source:\n");
        print(test_source);
        printf("\nASSERTION:\n");
        print(output_string);
        printf("!=\n");
        print(test_source);
        printf("\n");
        
#if TEST_EARLY_STOP
        exit(1);
#endif

    } else { 
        printf("OK\n");
    }

    free_allocator(&allocator);
}

void test() {

    #define test_ast(source, test_source) _test_ast(__LINE__, init_string(source), init_string(test_source))

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
    // TEST_SOURCE_TO_INTERP("sqrt(8)", _create_ast_binop(_create_ast_integer(2), create_ast_call((Token){ .text="sqrt" }, _create_ast_integer(2)), OP_MUL));

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
    test_ast("-8^8", "-16777216");
    test_ast("(-29*-17)", "493");
    test_ast("(-24--40)", "16");
    test_ast("1/3*2", "2/3");
    test_ast("1/8 + 1", "9/8");
    test_ast("sin(4/5)*2-5.2/4", "0.134712");
    test_ast("log(8, 2)", "3");

    printf("\n\n");

    {
        // test strings
        Allocator allocator = init_allocator();

        String first = init_string("Alexnder");

        first = string_concat(&allocator, first, init_string(" "));
        first = string_insert(&allocator, first, init_string("a"), 4);

        String last = init_string("Tebbe");
        String name = string_concat(&allocator, first, last);

        assert(string_eq(name, init_string("Alexander Tebbe")));

        String empty = init_string("");
        String not_empty = init_string("casc");
        String together = string_concat(&allocator, empty, not_empty);

        assert(together.size == 4);

        free_allocator(&allocator);
    }
}

void main_cli() {
    Allocator allocator = init_allocator();

    Lexer lexer = {0};
    lexer.source = init_string("5 - - - + - (3 + 4) - +2");
    printf("source:\n");
    print(lexer.source);
    lexer.allocator = &allocator;

    Interp ip = {0};
    ip.allocator = &allocator;

    AST* output = parse(&lexer);
    printf("parsed:\n");
    print(ast_to_debug_string(&allocator, output));

    output = interp(&ip, output);

    printf("output:\n");
    print(ast_to_debug_string(&allocator, output));
    print(ast_to_string(&allocator, output));

    free_allocator(&allocator);
}

i32 main(i32 argc, char *argv[]) {
    bool do_test = false;
    bool do_cli = true;
    bool do_gui = false;

    for (i32 i = 1; i < argc; i++) {
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
        main_cli();
    } else if (do_gui) {
        init_gui();
    } else {
        assert(false);
        // unreachable
    }

    return 0;
}
