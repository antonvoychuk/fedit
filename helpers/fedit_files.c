#include "../fedit.h"

#include <stream/buffered_file_stream.h>

#include <assets_icons.h>
#include <fedit_icons.h>

void fedit_file_get_name(FeditFile* file) {
    if (file->path_set) {
        size_t path_length = furi_string_size(file->path);
        size_t name_offset = furi_string_search_rchar(file->path, '/');
        char* name = malloc(sizeof(char) * (path_length - name_offset + 2));
        char* tmp_buf = malloc(sizeof(char) * FEDIT_MAX_FILENAME_LENGTH);
        memcpy(name, furi_string_get_cstr(file->path) + name_offset + 1, path_length - name_offset - 1);
        snprintf(tmp_buf, FEDIT_MAX_FILENAME_LENGTH, FEDIT_CACHE_FOLDER "/.%s", name);
        furi_string_set(file->cache, tmp_buf);
        file->cache_set = true;
    }
}

bool fedit_file_browse(FeditFile* file, char* extension) {
    furi_assert(file);
    furi_assert(extension);

    DialogsFileBrowserOptions browser_options;
    dialog_file_browser_set_basic_options(&browser_options, extension, &I_fedit_10px);
    browser_options.hide_ext = false;

    DialogsApp* dialog = furi_record_open(RECORD_DIALOGS);
    bool selected = dialog_file_browser_show(dialog, file->path, file->path, &browser_options);

    furi_record_close(RECORD_DIALOGS);
    if (selected) {
        file->path_set = true;
    }
    return selected;
}

bool fedit_file_validate(Storage* storage, FeditFile* file) {
    furi_assert(storage);
    furi_assert(file);
    if (!storage_file_exists(storage, furi_string_get_cstr(file->path)) || !file->path_set) {
        if (!fedit_file_browse(file, FEDIT_EXTENSION)) {
            return false;
        } else return true;
    }
    return true;
}

bool fedit_file_update_buffer(FeditFile* file, uint32_t bytes_per_line) {
    furi_assert(file);
    furi_assert(file->buffer);
    furi_assert(file->buffer->data);
    furi_assert(file->stream);
    if (file->offset % bytes_per_line != 0)
        file->offset -= file->offset % bytes_per_line;
    memset(file->buffer->data, 0, sizeof(uint8_t) * file->buffer->size);

    if (!stream_seek(file->stream, file->offset, StreamOffsetFromStart)) {
        return false;
    }

    file->buffer->used = stream_read(file->stream, file->buffer->data, file->buffer->size);
    return true;
}

bool fedit_file_update_cache(FeditFile* file) {
    if (!stream_seek(file->stream, -(file->buffer->used), StreamOffsetFromCurrent))
        return false;
    stream_write(file->stream, file->buffer->data, file->buffer->used);
    return true;
}

bool fedit_file_save(Storage* storage, FeditFile* file) {
    furi_assert(file);
    fedit_file_stream_end(file);
    storage_common_remove(storage, furi_string_get_cstr(file->path));
    storage_common_copy(storage, furi_string_get_cstr(file->cache), furi_string_get_cstr(file->path));
    fedit_file_stream_start(storage, file);
    return true;
}

bool fedit_file_scroll(FeditFile* file, uint32_t bytes_per_line, bool dir) {
    furi_assert(file);
    if (stream_seek(file->stream, -(file->buffer->used), StreamOffsetFromCurrent))
        stream_write(file->stream, file->buffer->data, file->buffer->used);
    uint32_t last_byte_on_screen = file->offset + file->buffer->used;
    if (dir && (file->size > last_byte_on_screen)) {
        file->offset += bytes_per_line;
        uint32_t cap = MIN(file->buffer->used, file->buffer->size - 1);
        if (file->buffer->cursor > cap) {
            file->buffer->cursor = cap;
        }
    } else if (!dir && (file->offset > 0)) {
        file->offset -= bytes_per_line;
    } else return false;
    return fedit_file_update_buffer(file, bytes_per_line);
}

bool fedit_file_move_cursor(FeditFile* file, uint32_t bytes_per_line, bool dir, uint32_t qty) {
    furi_assert(file);
        uint32_t cap = MIN(file->buffer->used, file->buffer->size - 1);
    if (dir && (file->buffer->cursor < cap)) file->buffer->cursor += qty;
    else if(dir && (file->offset + file->buffer->used < file->size)) {
        fedit_file_scroll(file, bytes_per_line, dir);
        file->buffer->cursor -= bytes_per_line - qty;
    } else if (!dir && (file->buffer->cursor > 0)) file->buffer->cursor -= qty;
    else if(!dir && (file->offset > 0)) {
        fedit_file_scroll(file, bytes_per_line, dir);
        file->buffer->cursor += bytes_per_line - qty;
    }
    return true;
}

FeditFile* fedit_file_alloc() {
    FeditFile* file = malloc(sizeof(FeditFile));

    file->path = furi_string_alloc();
    file->path_set = false;
    file->cache = furi_string_alloc();
    file->cache_set = false;

    file->size = 0;
    file->offset = 0;

    file->buffer = malloc(sizeof(FeditFileBuffer));
    file->buffer->data_allocated = false;
    file->buffer->size = 0;
    file->buffer->used = 0;
    file->buffer->cursor = 0;
    return file;
}

bool fedit_file_realloc_buffer(FeditFile* file, uint32_t new_size) {
    furi_assert(file);
    if (file->buffer->data_allocated)
        file->buffer->data = realloc(file->buffer->data, new_size);
    else
        file->buffer->data = malloc(sizeof(uint8_t) * new_size);
    file->buffer->size = new_size;
    return true;
}

void fedit_file_free(FeditFile* file) {
    furi_assert(file);
    // Close stream if it exists
    if (file->stream != NULL) buffered_file_stream_close(file->stream);

    // 
    if (file->buffer->data_allocated || file->buffer->data != NULL) {
        free(file->buffer->data);
        file->buffer->data_allocated = false;
    }
    free(file->buffer);

    furi_string_free(file->cache);
    file->cache_set = false;
    furi_string_free(file->path);
    file->path_set = false;

    free(file);
}

void fedit_file_stream_start(Storage* storage, FeditFile* file) {
    file->stream = buffered_file_stream_alloc(storage);
}

bool fedit_file_stream_end(FeditFile* file) {
    return buffered_file_stream_close(file->stream);
}

bool fedit_file_cache_open(Storage* storage, FeditFile* file) {
    // Create cache folder if it doesn't exist
    if (!storage_common_exists(storage, FEDIT_CACHE_FOLDER))
        storage_common_mkdir(storage, FEDIT_CACHE_FOLDER);
    // If cached file already exists, delete it
    if (storage_common_exists(storage, furi_string_get_cstr(file->cache)))
        storage_common_remove(storage, furi_string_get_cstr(file->cache));
    // Create a cached copy of the file
    if (storage_common_copy(storage, furi_string_get_cstr(file->path),
            furi_string_get_cstr(file->cache)) == FSE_OK)
    {
        // Open cached file
        if (buffered_file_stream_open(file->stream,
                furi_string_get_cstr(file->cache),
                FSAM_READ_WRITE, FSOM_OPEN_EXISTING))
        {
            file->size = stream_size(file->stream);
            return true;
        };
    };
    return false;
}