#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "interp.h"
#include "gui.h"

#include "raylib.h"

#define CELL_INPUT_BUFFER_SIZE 1024
#define CELL_OUTPUT_BUFFER_SIZE 1024

// Postfix _ to ensure, that we don't use them directly. They are only
// for initializing the GUI structure with default values.
#define WINDOW_TITLE_ "casc"
#define WINDOW_WIDTH_ 800
#define WINDOW_HEIGHT_ 450
#define FONT_SIZE_ 32

typedef struct {
    char* window_title;
    int screen_width;
    int screen_height;
    int font_size;
    int cursor_position;
} GUI;

void init_gui() {
    GUI gui;
    gui.window_title = WINDOW_TITLE_;
    gui.screen_width = WINDOW_WIDTH_;
    gui.screen_height = WINDOW_HEIGHT_;
    gui.font_size = FONT_SIZE_;
    gui.cursor_position = 0;

    char cell_input_buffer[CELL_INPUT_BUFFER_SIZE];
    char *cell_output_buffer = NULL;
    (void)cell_output_buffer;
    cell_input_buffer[0] = '\0';

    InitWindow(gui.screen_width, gui.screen_height, gui.window_title);

    SetTargetFPS(60);

    while (!WindowShouldClose()) {

        // Control
        char c = GetCharPressed();
        if (c != 0) {
            assert(strlen(cell_input_buffer) + 1 < CELL_INPUT_BUFFER_SIZE);
            memmove(
                &cell_input_buffer[gui.cursor_position+1],
                &cell_input_buffer[gui.cursor_position],
                strlen(cell_input_buffer)-gui.cursor_position+1 // we need to add 1 here, because it's a null terminated string for now
            );
            cell_input_buffer[gui.cursor_position] = c;
            gui.cursor_position++;
        } else if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE)) {
            if (gui.cursor_position > 0) {
                int idx_to_delete = gui.cursor_position-1;
                memmove(
                    &cell_input_buffer[idx_to_delete],
                    &cell_input_buffer[idx_to_delete+1],
                    strlen(cell_input_buffer)-gui.cursor_position+1 // we need to add 1 here, because it's a null terminated string for now
                );
                gui.cursor_position--;
            }
        } else if (IsKeyPressed(KEY_LEFT)) {
            if (gui.cursor_position > 0) {
                gui.cursor_position--;
            }
        } else if (IsKeyPressed(KEY_RIGHT)) {
            if (gui.cursor_position < (int)strlen(cell_input_buffer)) {
                gui.cursor_position++;
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
            DrawText(cell_input_buffer, 10, 10, gui.font_size, BLACK);
            {
                char slice[CELL_INPUT_BUFFER_SIZE];
                for (int i = 0; i < gui.cursor_position; i++) {
                    slice[i] = cell_input_buffer[i];
                }
                slice[gui.cursor_position] = '\0';
                int cell_input_buffer_text_width = MeasureText(slice, gui.font_size);
                int start_pos_x = 10 + cell_input_buffer_text_width;
                int start_pos_y = 10;
                int end_pos_x   = start_pos_x;
                int end_pos_y   = 10+gui.font_size;
                DrawLine(start_pos_x, start_pos_y, end_pos_x, end_pos_y, BLACK);
            }
            DrawLine(0, 2*10+gui.font_size, gui.screen_width, 2*10+gui.font_size, GRAY);
            if (cell_output_buffer != NULL) {
                DrawText(cell_output_buffer, 10, 2*10+gui.font_size+10, gui.font_size, BLACK);
            }
        EndDrawing();
    }

    CloseWindow();
}