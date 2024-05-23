#pragma once

#include <gui/view.h>

#include "../helpers/fedit_custom_event.h"

#define FEDIT_EDITOR_BYTES_PER_LINE 8u
#define FEDIT_EDITOR_LINES_ON_SCREEN 7u
#define FEDIT_EDITOR_BUF_SIZE (FEDIT_EDITOR_LINES_ON_SCREEN * FEDIT_EDITOR_BYTES_PER_LINE)

typedef struct FeditEditor FeditEditor;
typedef struct FeditEditorModel FeditEditorModel;

typedef void (*FeditEditorCallback)(FeditCustomEvent event, void* ctx);

void fedit_editor_set_callback(FeditEditor*, FeditEditorCallback, void* ctx);

FeditEditor* fedit_editor_alloc(void* ctx);
void fedit_editor_free(FeditEditor*);

View* fedit_editor_get_view(FeditEditor*);