// TODO: repeating keys (ENTER, LEFT ARROW, RIGHT ARROW) not working

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

typedef enum {
    CELL_TEXT_INPUT,
    CELL_TEXT_OUTPUT
} CellType;

typedef struct {
    CellType type;
    String content;
} Cell;

typedef struct {
    usize size;
    Cell *data;
} Cells;

typedef struct {
    usize pos;
} Cursor;

typedef struct {
    Allocator allocator;
    Cursor cursor;
    Cells cells;
} GUI;

usize cursor_get_col(Cursor cursor, String base) {
    usize col = 0;
    for (usize i = 0; i < cursor.pos; i++) {
        if (base.str[i] == '\n') {
            col = 0;
        } else {
            col += 1;
        }
    }

    return col;
}

usize cursor_get_row(Cursor cursor, String base) {
    usize row = 0;
    for (usize i = 0; i < cursor.pos; i++) {
        if (base.str[i] == '\n') {
            row += 1;
        }
    }

    return row;
}

Cursor cursor_from_row_and_col(String base, usize row, usize col) {
    usize current_pos = 0;
    usize current_row = 0;
    usize current_col = 0;

    // move to the right row
    while (current_row < row && current_pos < base.size) {
        if (base.str[current_pos] == '\n') {
            current_row += 1;
            current_pos += 1;
        } else {
            current_pos += 1;
        }
    }

    // move to the right col
    while (current_col < col && current_pos < base.size) {
        if (base.str[current_pos] == '\n') {
            break;
        } else {
            current_col += 1;
            current_pos += 1;
        }
    }

    Cursor cursor = {0};
    cursor.pos = current_pos;

    return cursor;
}

Cursor cursor_increment(Cursor cursor, String base) {
    if (cursor.pos + 1 <= base.size) {
        cursor.pos += 1;
    }
    return cursor;
}

Cursor cursor_decrement(Cursor cursor, String base) {
    // just for symmetry in api calls (see cursor_increment)
    (void) base;

    if (0 < cursor.pos) {
        cursor.pos -= 1;
    }
    return cursor;
}

Cursor cursor_increment_by_row(Cursor cursor, String base) {
    usize current_row = cursor_get_row(cursor, base);
    usize current_col = cursor_get_col(cursor, base);
    current_row += 1;
    Cursor new_cursor = cursor_from_row_and_col(base, current_row, current_col);
    return new_cursor;
}

Cursor cursor_decrement_by_row(Cursor cursor, String base) {
    usize current_row = cursor_get_row(cursor, base);
    usize current_col = cursor_get_col(cursor, base);
    
    if (0 < current_row) {
        current_row -= 1;
    }

    Cursor new_cursor = cursor_from_row_and_col(base, current_row, current_col);
    return new_cursor;
}

Cursor cursor_increment_by_col(Cursor cursor, String base) {
    usize current_row = cursor_get_row(cursor, base);
    usize current_col = cursor_get_col(cursor, base);
    current_col += 1;
    Cursor new_cursor = cursor_from_row_and_col(base, current_row, current_col);
    return new_cursor;
}

Cursor cursor_decrement_by_col(Cursor cursor, String base) {
    usize current_row = cursor_get_row(cursor, base);
    usize current_col = cursor_get_col(cursor, base);

    if (0 < current_col) {
        current_col -= 1;
    }

    Cursor new_cursor = cursor_from_row_and_col(base, current_row, current_col);
    return new_cursor;
}

void cells_add(Cells *cells, CellType type) {

    if (cells->data == NULL) {
        cells->data = malloc(sizeof(Cell));
    } else {
        cells->data = realloc(cells->data, (cells->size+1)*sizeof(Cell));
    }

    cells->data[cells->size].type = type;
    cells->size += 1;
}

void init_gui() {
    SetTraceLogLevel(LOG_ERROR);

    GUI gui = {0};
    gui.allocator = init_allocator();

    String window_title = init_string("casc");
    i32 screen_width = 800;
    i32 screen_height = 600;

    InitWindow(screen_width, screen_height, window_title.str);

    Font font = LoadFontEx("./fonts/liberation_mono/LiberationMono-Regular.ttf", 96, 0, 0);
    Vector2 font_size = MeasureTextEx(font, "i", 32, 0);
    Vector2 padding = { font_size.x/2, font_size.y/4 };
    float line_height = font_size.y + 2*padding.y;
    (void) line_height;

    SetTextureFilter(font.texture, TEXTURE_FILTER_BILINEAR);
    SetTargetFPS(60);

    // init cells
    cells_add(&gui.cells, CELL_TEXT_INPUT);
    cells_add(&gui.cells, CELL_TEXT_OUTPUT);
    gui.cells.data[0].content = init_string(""); 
    gui.cells.data[1].content = init_string(""); 

    bool last_char_is_caret = false;
    while (!WindowShouldClose()) {
        //
        // Control
        //
        char c = GetCharPressed();

        // handle the caret key ^ seperatly because it has dead key mechanism.
        // So we need to clear the buffer here and call GetCharPressed again,
        // because the first call has not the right return value for us.
        if (last_char_is_caret && c != 0) {
            if (c < 0) {
                // TODO: what is this?
                c = 'a';
            } else {
                c = GetCharPressed();
            }
            last_char_is_caret = false;
        } else if (IsKeyPressed(KEY_ENTER) && !IsKeyDown(KEY_LEFT_SHIFT)) {
            // TODO: new line repetition not working yet
            c = '\n';
        }

        if (GetKeyPressed() == 161) {
            c = '^';
            last_char_is_caret = true;
        }
        
        if (c != 0) {
            gui.cells.data[0].content = string_insert(&gui.allocator, gui.cells.data[0].content, char_to_string(&gui.allocator, c), gui.cursor.pos);
            gui.cursor = cursor_increment(gui.cursor, gui.cells.data[0].content);
        } else if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE)) {
            if (gui.cursor.pos > 0) {
                i32 idx_to_delete = gui.cursor.pos-1;
                gui.cells.data[0].content = string_concat(
                    &gui.allocator,
                    string_slice(&gui.allocator, gui.cells.data[0].content, 0, idx_to_delete),
                    string_slice(&gui.allocator, gui.cells.data[0].content, idx_to_delete+1, gui.cells.data[0].content.size)
                );
                gui.cursor = cursor_decrement(gui.cursor, gui.cells.data[0].content);
            }
        } else if (IsKeyPressed(KEY_LEFT)) {
            if (gui.cursor.pos > 0) {
                gui.cursor = cursor_decrement(gui.cursor, gui.cells.data[0].content);
            }
        } else if (IsKeyPressed(KEY_RIGHT)) {
            if (gui.cursor.pos < gui.cells.data[0].content.size) {
                gui.cursor = cursor_increment(gui.cursor, gui.cells.data[0].content);
            }
        } else if (IsKeyPressed(KEY_DOWN)) {
            gui.cursor = cursor_increment_by_row(gui.cursor, gui.cells.data[0].content);
        } else if (IsKeyPressed(KEY_UP)) {
            gui.cursor = cursor_decrement_by_row(gui.cursor, gui.cells.data[0].content);
        } else if (IsKeyDown(KEY_LEFT_SHIFT) && IsKeyPressed(KEY_ENTER)) {
            Allocator allocator = init_allocator();

            // printf("cell_input_buffer.size=%zu\n", cell_input_buffer.size);
            
            Lexer lexer = {0};
            lexer.source = gui.cells.data[0].content;
            lexer.allocator = &allocator;

            Interp ip = {0};
            ip.allocator = &allocator;

            AST *output = parse(&lexer);
            // print(ast_to_debug_string(&allocator, output));
            output = interp(&ip, output);

            gui.cells.data[1].content = ast_to_string(&gui.allocator, output);

            free_allocator(&allocator);
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

        // cells
        {
            i32 current_y = 0;

            for (usize i = 0; i < gui.cells.size; i++) {

                DrawLine(0, current_y, screen_width, current_y, PINK);

                Cell cell = gui.cells.data[i];

                switch (cell.type) {

                    case CELL_TEXT_INPUT: {
                        usize lines_count = 1;
                        for (usize j = 0; j < cell.content.size; j++) {
                            if (cell.content.str[j] == '\n') {
                                lines_count += 1;
                            }
                        }

                        Vector2 pos = { padding.x, padding.y };
                        i32 cell_height = padding.y + (font_size.y*lines_count) + padding.y;
                        i32 cell_width_without_padding = screen_width - 2*padding.x;
                        i32 cell_height_without_padding = cell_height - 2*padding.y;
                        DrawRectangleLines(padding.x, current_y+padding.y, cell_width_without_padding, cell_height_without_padding, PINK);
                        DrawTextEx(font, cell.content.str, pos, font_size.y, 0, COLOR_DARK_GREEN);

                        current_y += cell_height;

                        // cursor

                        // cursor pos -> row,col
                        usize row = cursor_get_row(gui.cursor, cell.content);
                        usize col = cursor_get_col(gui.cursor, cell.content);

                        i32 x = padding.x + font_size.x * col;
                        i32 y = padding.y + font_size.y * row;
                        DrawRectangle(x, y, font_size.x/8, font_size.y, COLOR_POMP_AND_POWER);

                        break;
                    }

                    case CELL_TEXT_OUTPUT: {
                        if (cell.content.size > 0) {
                            Vector2 output_text_pos = { padding.x, current_y + padding.y };
                            DrawTextEx(font, cell.content.str, output_text_pos, font_size.y, 2, COLOR_DARK_GREEN);
                        }
                        break;
                    }

                    default: assert(false);

                }

            }
        }

        EndDrawing();
    }

    UnloadFont(font);

    CloseWindow();
}