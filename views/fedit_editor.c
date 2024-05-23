#include "../fedit.h"

#include <stream/stream.h>

typedef enum {
    FeditEditorModeMain = 1,
    FeditEditorModeData = 2,
    FeditEditorModeEdit = 4,
} FeditEditorMode;

struct FeditEditorModel {
    FeditEditorMode mode;
    FeditFile* file;
};

struct FeditEditor {
    View* view;
    FeditEditorCallback callback;
    void* ctx;
};

void fedit_editor_set_callback(FeditEditor* instance, FeditEditorCallback callback, void* ctx) {
    furi_assert(instance);
    furi_assert(callback);
    instance->callback = callback;
    instance->ctx = ctx;
}

void fedit_editor_draw(Canvas* canvas, FeditEditorModel* model) {
    furi_assert(model);
    furi_assert(model->file);
    furi_assert(model->file->buffer);
    canvas_clear(canvas);
    canvas_set_color(canvas, ColorBlack);

    uint32_t bytes_per_line = FEDIT_EDITOR_BYTES_PER_LINE;
    uint32_t lines_on_screen = FEDIT_EDITOR_LINES_ON_SCREEN;

    int ROW_HEIGHT = 9;
    int TOP_OFFSET = 8;
    int LEFT_OFFSET = 6;
    int ASCII_OFFSET = 60;

    uint32_t line_count = model->file->size / bytes_per_line;
    if (model->file->size % bytes_per_line != 0) line_count += 1;

    uint16_t first_line_on_screen = model->file->offset / bytes_per_line;
    uint16_t scrollbar_cur = first_line_on_screen / (line_count > 0xFFFF ? 0x100 : 1);
    uint16_t scrollbar_max = line_count - (lines_on_screen - 1) / (line_count > 0xFFFF ? 0x100 : 1);
    if (line_count > lines_on_screen) {
        uint8_t width = canvas_width(canvas);
        elements_scrollbar_pos(
            canvas,
            width,
            0,
            ROW_HEIGHT * lines_on_screen,
            scrollbar_cur,
            scrollbar_max);
    }

    char temp_buf[32];
    uint32_t row_iters = model->file->buffer->used / bytes_per_line;
    if (model->file->buffer->used % bytes_per_line != 0) row_iters += 1;

    for (uint32_t i = 0; i < row_iters; ++i) {
        uint32_t bytes_left_per_row = model->file->buffer->used - i * bytes_per_line;
        bytes_left_per_row = MIN(bytes_left_per_row, bytes_per_line);

        if (model->mode & FeditEditorModeData) {
            // Show hex direction
            uint32_t addr = model->file->offset + i * bytes_per_line;
            snprintf(temp_buf, 32, "%08lX", addr);

            canvas_set_font(canvas, FontKeyboard);
            canvas_draw_str(canvas, LEFT_OFFSET, TOP_OFFSET + i * ROW_HEIGHT, temp_buf);
            // Show as ASCII
            memcpy(temp_buf, model->file->buffer->data + bytes_per_line * i, bytes_left_per_row);
            temp_buf[bytes_left_per_row] = '\0';
            for (uint32_t j = 0; j < bytes_left_per_row; ++j)
                if (!isprint((int)temp_buf[j])) temp_buf[j] = ' ';

            canvas_set_font(canvas, FontKeyboard);
            canvas_draw_str(canvas, LEFT_OFFSET + ASCII_OFFSET, TOP_OFFSET + i * ROW_HEIGHT, temp_buf);
        }

        if (model->mode & FeditEditorModeMain) { // Show as hex, complete screen
            char* p = temp_buf;
            for (uint32_t j = 0; j < bytes_left_per_row; ++j)
                p += snprintf(p, 32, (j % 2 == 0) ? "%02X" : "%02X ", *(model->file->buffer->data + i * bytes_per_line + j));
            *p = '\0';
            canvas_set_font(canvas, FontKeyboard);
            canvas_draw_str(canvas, LEFT_OFFSET, TOP_OFFSET + i * ROW_HEIGHT, temp_buf);
        }
    }
    if (model->mode & FeditEditorModeEdit) {
        uint8_t t_offset = TOP_OFFSET + (model->file->buffer->cursor / bytes_per_line) * ROW_HEIGHT;
        uint8_t line_offset = model->file->buffer->cursor % bytes_per_line;
        uint8_t d_line_offset = LEFT_OFFSET;
        if (model->mode & FeditEditorModeMain)
            d_line_offset += line_offset * 12 + (line_offset / 2) * 6;
        else if (model->mode & FeditEditorModeData)
            d_line_offset += line_offset * 6 + ASCII_OFFSET;
        canvas_draw_line(canvas, d_line_offset, t_offset, d_line_offset + (model->mode & FeditEditorModeMain ? 10 : 6), t_offset);
    }
}

static void fedit_editor_model_init(FeditEditorModel* const model) {
    model->mode = FeditEditorModeMain;
}

bool fedit_editor_input(InputEvent* event, void* ctx) {
    furi_assert(ctx);
    FeditEditor* instance = ctx;
    FeditApp* app = instance->ctx;
    with_view_model(
        instance->view,
        FeditEditorModel* model,
        {
            // Mode management
            if (model->mode == FeditEditorModeMain && event->type == InputTypeShort && event->key == InputKeyLeft) {
                model->mode = FeditEditorModeData;
            } else if (model->mode == FeditEditorModeData && event->type == InputTypeShort && event->key == InputKeyLeft) {
                model->mode = FeditEditorModeMain;
            } else if ((model->mode == FeditEditorModeMain || model->mode == FeditEditorModeData) && event->type == InputTypeShort && event->key == InputKeyRight) {
                model->mode |= FeditEditorModeEdit;
            } else if ((model->mode & FeditEditorModeEdit) && event->type == InputTypeShort && event->key == InputKeyBack) {
                model->mode &= ~FeditEditorModeEdit;
            } else if (!(model->mode & FeditEditorModeMain) && event->type == InputTypeShort && event->key == InputKeyBack) {
                model->mode = FeditEditorModeMain;
            }
            // View scroll
            else if (!(model->mode & FeditEditorModeEdit) && (event->type == InputTypeShort || event->type == InputTypeRepeat) && event->key == InputKeyUp) {
                fedit_file_scroll(app->file, FEDIT_EDITOR_BYTES_PER_LINE, false);
            } else if (!(model->mode & FeditEditorModeEdit) && (event->type == InputTypeShort || event->type == InputTypeRepeat) && event->key == InputKeyDown) {
                fedit_file_scroll(app->file, FEDIT_EDITOR_BYTES_PER_LINE, true);
            }
            // Back to menu
            else if (model->mode == FeditEditorModeMain && event->type == InputTypeShort && event->key == InputKeyBack) {
                instance->callback(FeditEventEditorBack, instance->ctx);
            }
            // Offset
            else if (model->mode & FeditEditorModeEdit && (event->type == InputTypeShort || event->type == InputTypeRepeat) && event->key == InputKeyLeft) {
                fedit_file_move_cursor(model->file, FEDIT_EDITOR_BYTES_PER_LINE, false, event->type == InputTypeShort ? 1 : 2);
            } else if (model->mode & FeditEditorModeEdit && (event->type == InputTypeShort || event->type == InputTypeRepeat) && event->key == InputKeyRight) {
                fedit_file_move_cursor(model->file, FEDIT_EDITOR_BYTES_PER_LINE, true, event->type == InputTypeShort ? 1 : 2);
            }
            // Char edit
            else if (model->mode & FeditEditorModeEdit && (event->type == InputTypeShort || event->type == InputTypeRepeat) && event->key == InputKeyUp) {
                app->file->buffer->data[app->file->buffer->cursor] += event->type == InputTypeShort ? 1 : 2;
            } else if (model->mode & FeditEditorModeEdit && (event->type == InputTypeShort || event->type == InputTypeRepeat) && event->key == InputKeyDown) {
                app->file->buffer->data[app->file->buffer->cursor] -= event->type == InputTypeShort ? 1 : 2;
            }
            // Save file
            else if (!(model->mode & FeditEditorModeEdit) && event->type == InputTypeLong && event->key == InputKeyOk) {
                fedit_file_update_cache(app->file);
                fedit_file_save(app->storage, app->file);
                fedit_file_cache_open(app->storage, app->file);
                fedit_file_update_buffer(app->file, FEDIT_EDITOR_BYTES_PER_LINE);
                notification_message(app->notification, &sequence_blink_cyan_100);
            }
            else if (model->mode & FeditEditorModeEdit && event->type == InputTypeLong && event->key == InputKeyBack) {
                fedit_file_update_cache(app->file);
                notification_message(app->notification, &sequence_blink_magenta_100);
            }
            else if (model->mode & FeditEditorModeEdit && event->type == InputTypeLong && event->key == InputKeyOk) {
                fedit_file_update_cache(app->file);
                notification_message(app->notification, &sequence_blink_blue_100);
            }
        },
        true
    );
    return true;
}

void fedit_editor_exit(void* ctx) {
    furi_assert(ctx);
    FeditApp* app = ctx;
    UNUSED(app);
}

FeditEditor* fedit_editor_alloc(void* ctx) {
    FeditApp* app = ctx;
    FeditEditor* instance = malloc(sizeof(FeditEditor));

    instance->view = view_alloc();

    view_allocate_model(instance->view, ViewModelTypeLocking, sizeof(FeditEditorModel));
    view_set_context(instance->view, instance);
    view_set_draw_callback(instance->view, (ViewDrawCallback)fedit_editor_draw);
    view_set_input_callback(instance->view, fedit_editor_input);
    view_set_exit_callback(instance->view, fedit_editor_exit);

    with_view_model(
        instance->view,
        FeditEditorModel* model,
        {
            fedit_editor_model_init(model);
            model->file = app->file;
        },
        true
    );

    return instance;
}

void fedit_editor_free(FeditEditor* instance) {
    furi_assert(instance);

    view_free(instance->view);

    free(instance);
}

View* fedit_editor_get_view(FeditEditor* instance) {
    furi_assert(instance);

    return instance->view;
}
