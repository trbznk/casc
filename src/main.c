#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#include <math.h>

#include "casc.h"

#define ARENA_SIZE 1024

Arena init_arena() {
    Arena arena = {0};
    arena.memory = malloc(ARENA_SIZE);
    if (arena.memory == NULL) {
        todo()
    }
    arena.size = ARENA_SIZE;
    return arena;
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

// maybe @unused
void arena_reset(Arena *arena) {
    arena->offset = 0;
}

void *arena_alloc(Arena *arena, usize size) {
# if 0
    static bool has_seen_arena_alloc_malloc_warning = false;
    if (!has_seen_arena_alloc_malloc_warning) {
        printf("WARNING: currently arena is using malloc always!\n");
        has_seen_arena_alloc_malloc_warning = true;
    }
    return malloc(size);
#endif

    usize free_capacity = arena->size - arena->offset;

#if 0
    printf("size=%zu, free_capacity=%zu, reallocs_count=%u\n", size, free_capacity, arena->reallocs_count);
#endif

    if (free_capacity <= size) {
        usize new_size;

        if (size < ARENA_SIZE) {
            new_size = arena->size + ARENA_SIZE;
        } else {
            // Do we need padding here? @note
            new_size = arena->size + size;
        }

        arena->size = new_size ;
        arena->memory = realloc(arena->memory, arena->size);
        if (arena->memory == NULL) {
            todo()
        }
        arena->reallocs_count += 1;
    }

    // current position in the memory
    void *memory = arena->memory + arena->offset;

    // compute the new position in the memory
    arena->offset += size;

    // double arena_memory_used = (double)arena->offset / (double)arena->size;
    // assert(arena_memory_used < 0.5);
    assert(arena->offset < arena->size);

#if 0
    printf("arena_memory_used = %.2f\n", arena_memory_used);
#endif

    return memory;
}

void _test_ast(u32 line_number, char *source, char *test_source) {
    Arena arena = init_arena();

    Lexer lexer = {0};
    lexer.source = source;
    lexer.arena = &arena;

    Interp ip = {0};
    ip.arena = &arena;

    AST* output = parse(&lexer);
    output = interp(&ip, output);

    printf("test:%d ... ", line_number);

    char *output_string = ast_to_string(ip.arena, output);
    if (strcmp(output_string, test_source) != 0) {
        printf("FAILED\n");        
        printf("'%s'\n", output_string);
        printf("!=\n");
        printf("'%s'\n", test_source);
        // exit(1);
    } else { 
        printf("OK\n");
    }

    arena_free(&arena);
}

void test() {

    #define test_ast(source, test_source) _test_ast(__LINE__, source, test_source)

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
    test_ast("1/3*2", "2/3");
    test_ast("1/8 + 1", "9/8");
    test_ast("sin(4/5)*2-5.2/4", "0.134712");

    printf("\n\n");
}

typedef struct String String;
struct String {
    char *str;
    usize size;
};

String init_string(Arena *arena, char *str) {
    String s = {0};
    s.size = strlen(str);
    printf("s.size=%zu\n", s.size);
    s.str = arena_alloc(arena, s.size);
    strncpy(s.str, str, s.size);
    return s;
}

String string_slice(Arena *arena, String s, usize start, usize stop) {
    String new_s = {0};
    new_s.size = stop-start;
    new_s.str = arena_alloc(arena, new_s.size);
    strncpy(new_s.str, &s.str[start], new_s.size);
    return new_s;
}

String string_concat(Arena *arena, String s1, String s2) {
    String s = {0};
    s.size = s1.size+s2.size;
    s.str = arena_alloc(arena, s.size);
    strncpy(s.str, s1.str, s1.size);
    strncpy(&s.str[s1.size], s2.str, s2.size);
    return s;
}

String string_insert(Arena *arena, String s1, String s2, usize idx) {
    String prefix = string_slice(arena, s1, 0, idx);
    String postfix = string_slice(arena, s1, idx, s1.size);
    String new_s = string_concat(arena, prefix, s2);
    new_s = string_concat(arena, new_s, postfix);
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

void main_cli() {
    Arena arena = init_arena();

    String first = init_string(&arena, "Alexnder");

    first = string_concat(&arena, first, init_string(&arena, " "));
    first = string_insert(&arena, first, init_string(&arena, "a"), 4);

    String last = init_string(&arena, "Tebbe");
    String name = string_concat(&arena, first, last);

    print(name);
    assert(string_eq(name, init_string(&arena, "Alexander Tebbe")));

    String s = init_string(&arena, "");
    print(s);

    arena_free(&arena);
}

void main_cli1() {
    Arena arena = init_arena();

    Lexer lexer = {0};
    lexer.source = "1/8 * (-4-2*4/3-1)";
    lexer.arena = &arena;

    Interp ip = {0};
    ip.arena = &arena;

    AST* output = parse(&lexer);
    printf("parsed:\n");
    printf("%s\n", ast_to_debug_string(&arena, output));

    output = interp(&ip, output);

    printf("output:\n");
    printf("%s\n", ast_to_debug_string(&arena, output));
    printf("%s\n", ast_to_string(&arena, output));

    arena_free(&arena);
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
