#pragma once

#include <furi.h>
#include <stream/stream.h>
#include <storage/storage.h>

typedef struct {
    uint8_t* data;
    bool data_allocated;
    uint32_t size;
    uint32_t used;
    uint32_t cursor;
} FeditFileBuffer;

typedef struct {
    FuriString* name;
    FuriString* path;
    bool path_set;
    FuriString* cache;
    bool cache_set;

    uint32_t size;
    uint32_t offset;

    FeditFileBuffer* buffer;
    Stream* stream;
} FeditFile;

void fedit_file_get_name(FeditFile* file);

bool fedit_file_browse(FeditFile* file, char* extension);
bool fedit_file_validate(Storage* storage, FeditFile* file);

bool fedit_file_update_buffer(FeditFile* file, uint32_t bytes_per_line);
bool fedit_file_update_cache(FeditFile* file);
bool fedit_file_save(Storage* storage, FeditFile* file);
bool fedit_file_scroll(FeditFile* file, uint32_t bytes_per_line, bool dir);
bool fedit_file_move_cursor(FeditFile* file, uint32_t bytes_per_line, bool dir, uint32_t qty);

FeditFile* fedit_file_alloc();
bool fedit_file_realloc_buffer(FeditFile* file, uint32_t new_size);
void fedit_file_free(FeditFile* file);

void fedit_file_stream_start(Storage* storage, FeditFile* file);
bool fedit_file_stream_end(FeditFile* file);
bool fedit_file_cache_open(Storage* storage, FeditFile* file);