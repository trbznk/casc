#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#include <math.h>

#include "parser.h"
#include "interp.h"

#include "raylib.h"

#define CELL_INPUT_BUFFER_SIZE 1024
#define CELL_OUTPUT_BUFFER_SIZE 1024

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
        printf("'%s'\n", argv[i]);
    }
    assert(do_cli != do_gui);

    if (do_test) {
        test();
    }

    if (do_cli) {
        // ast_match_type()
        AST* output = interp_from_string("3/2 + 5/2");
        bool r = ast_match_type(output, parse_from_string("1/1 + 1/1"));
        printf("r=%d\n", r);
        
    } else if (do_gui) {
        // Editor
        const int SCREEN_WIDTH = 800;
        const int SCREEN_HEIGHT = 450;
        const int FONT_SIZE = 32;
        char cell_input_buffer[CELL_INPUT_BUFFER_SIZE];
        char *cell_output_buffer = NULL;
        (void)cell_output_buffer;
        cell_input_buffer[0] = '\0';
        int cursor_position = 0;
        (void)cursor_position;

        InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "casc");

        SetTargetFPS(60);

        while (!WindowShouldClose()) {

            // Control
            char c = GetCharPressed();
            if (c != 0) {
                assert(strlen(cell_input_buffer) + 1 < CELL_INPUT_BUFFER_SIZE);
                memmove(
                    &cell_input_buffer[cursor_position+1],
                    &cell_input_buffer[cursor_position],
                    strlen(cell_input_buffer)-cursor_position+1 // we need to add 1 here, because it's a null terminated string for now
                );
                cell_input_buffer[cursor_position] = c;
                cursor_position++;
            } else if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE)) {
                if (cursor_position > 0) {
                    int idx_to_delete = cursor_position-1;
                    memmove(
                        &cell_input_buffer[idx_to_delete],
                        &cell_input_buffer[idx_to_delete+1],
                        strlen(cell_input_buffer)-cursor_position+1 // we need to add 1 here, because it's a null terminated string for now
                    );
                    cursor_position--;
                }
            } else if (IsKeyPressed(KEY_LEFT)) {
                if (cursor_position > 0) {
                    cursor_position--;
                }
            } else if (IsKeyPressed(KEY_RIGHT)) {
                if (cursor_position < (int)strlen(cell_input_buffer)) {
                    cursor_position++;
                }
            } else if (IsKeyDown(KEY_LEFT_SHIFT) && IsKeyPressed(KEY_ENTER)) {
                Tokens tokens = tokenize(cell_input_buffer);
                Parser parser = { .tokens = tokens, .pos=0 };
                AST* ast = parse_expr(&parser);
                AST* output = interp(ast);
                cell_output_buffer = ast_to_string(output);
            }

            // Draw
            BeginDrawing();
                ClearBackground(WHITE);
                DrawText(cell_input_buffer, 10, 10, FONT_SIZE, BLACK);
                {
                    char slice[CELL_INPUT_BUFFER_SIZE];
                    for (int i = 0; i < cursor_position; i++) {
                        slice[i] = cell_input_buffer[i];
                    }
                    slice[cursor_position] = '\0';
                    int cell_input_buffer_text_width = MeasureText(slice, FONT_SIZE);
                    int start_pos_x = 10 + cell_input_buffer_text_width;
                    int start_pos_y = 10;
                    int end_pos_x   = start_pos_x;
                    int end_pos_y   = 10+FONT_SIZE;
                    DrawLine(start_pos_x, start_pos_y, end_pos_x, end_pos_y, BLACK);
                }
                DrawLine(0, 2*10+FONT_SIZE, SCREEN_WIDTH, 2*10+FONT_SIZE, GRAY);
                if (cell_output_buffer != NULL) {
                    DrawText(cell_output_buffer, 10, 2*10+FONT_SIZE+10, FONT_SIZE, BLACK);
                }
            EndDrawing();
        }

        CloseWindow();
    } else {
        assert(false);
        // unreachable
    }

    return 0;
}
