#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

#include "raylib.h"

#include "casc.h"

#define CELL_INPUT_BUFFER_SIZE 1024
#define CELL_OUTPUT_BUFFER_SIZE 1024

const Color COLOR_DARK_GREEN = {18, 40, 36, 255};
const Color COLOR_LIGHT_GREEN = {171, 225, 136, 255};
const Color COLOR_MOONSTONE = {57, 162, 174, 255};
const Color COLOR_POMP_AND_POWER = {129, 110, 148, 255};
const Color COLOR_CHESTNUT = {162, 73, 54, 255};

void init_gui() {
    char* window_title = "casc";
    i32 screen_width = 800;
    i32 screen_height = 600;
    i32 cursor_position = 0;

    Arena gui_arena = create_arena(1024);

    char cell_input_buffer[CELL_INPUT_BUFFER_SIZE];

    // dont like this @todo
    char *cell_output_buffer = NULL;
    cell_input_buffer[0] = '\0';

    InitWindow(screen_width, screen_height, window_title);

    // Font font = LoadFontEx("./fonts/monaspace-v1.101/fonts/otf/MonaspaceNeon-Regular.otf", 96, 0, 0);
    Font font = LoadFontEx("./fonts/PkgTTF-Iosevka-30/Iosevka-Regular.ttf", 96, 0, 0);
    Vector2 font_size = MeasureTextEx(font, "i", 48, 0);
    Vector2 padding = { font_size.x/2, font_size.y/4 };
    float line_height = font_size.y + 2*padding.y;

    SetTextureFilter(font.texture, TEXTURE_FILTER_BILINEAR);
    SetTargetFPS(60);

    bool last_char_is_caret = false;
    while (!WindowShouldClose()) {

        //
        // DEBUG
        //

#if 0
        printf("cursor_position=%d\n", cursor_position);
#endif

        //
        // Control
        //
        char c = GetCharPressed();

        // handle the caret key ^ seperatly because it has dead key mechanism.
        // So we need to clear the buffer here and call GetCharPressed again,
        // because the first call has not the right return value for us.
        if (last_char_is_caret && c != 0) {
            if (c < 0) {
                c = 'a';
            } else {
                c = GetCharPressed();
            }
            last_char_is_caret = false;
        }
        if (GetKeyPressed() == 161) {
            c = '^';
            last_char_is_caret = true;
        }
        
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
                i32 idx_to_delete = cursor_position-1;
                memmove(
                    &cell_input_buffer[idx_to_delete],
                    &cell_input_buffer[idx_to_delete+1],
                    strlen(cell_input_buffer)-cursor_position+1 // we need to add 1 here, because it's a null terminated string for now
                );
                cursor_position--;
            }
        } else if (IsKeyPressed(KEY_LEFT)) {
            if (cursor_position > 0) {
                cursor_position -= 1;
            }
        } else if (IsKeyPressed(KEY_RIGHT)) {
            if (cursor_position < (int)strlen(cell_input_buffer)) {
                cursor_position += 1;
            }
        } else if (IsKeyDown(KEY_LEFT_SHIFT) && IsKeyPressed(KEY_ENTER)) {
            Worker worker = create_worker();

            AST* output = interp_from_string(&worker, cell_input_buffer);

            cell_output_buffer = ast_to_string(&gui_arena, output);

            arena_free(&worker.arena);
        }

        //
        // Draw
        //
        BeginDrawing();

        ClearBackground(WHITE);

#if 0
        {
            Color c = { 0xFF, 0x00, 0x00, 0xFF};
            DrawRectangleLines(0, 0, screen_width, screen_height, c); 
            DrawRectangleLines(0, 0, screen_width, line_height, c); 
            DrawRectangleLines(0, line_height, screen_width, line_height, c); 
        }
#endif

        Vector2 input_text_pos = { padding.x, padding.y, };
        DrawTextEx(font, cell_input_buffer, input_text_pos, font_size.y, 0, COLOR_DARK_GREEN);

        {
            i32 x = padding.x + font_size.x * cursor_position;
            i32 y = padding.y;
            DrawRectangle(x, y, font_size.x/8, font_size.y, COLOR_POMP_AND_POWER);
        }

        DrawLine(0, line_height, screen_width, line_height, COLOR_MOONSTONE);

        if (cell_output_buffer != NULL) {
            Vector2 output_text_pos = { padding.x, line_height + padding.y, };
            DrawTextEx(font, cell_output_buffer, output_text_pos, font_size.y, 2, COLOR_DARK_GREEN);
        }

        EndDrawing();
    }

    UnloadFont(font);

    CloseWindow();
}