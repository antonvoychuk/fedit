#include "../fedit.h"

void fedit_editor_callback(FeditCustomEvent event, void* ctx) {
    furi_assert(ctx);
    FeditApp* app = ctx;
    view_dispatcher_send_custom_event(app->view_dispatcher, event);
}

void fedit_scene_editor_on_enter(void* ctx) {
    furi_assert(ctx);
    FeditApp* app = ctx;

    fedit_file_validate(app->storage, app->file);
    fedit_file_stream_start(app->storage, app->file);
    fedit_file_get_name(app->file);
    fedit_file_cache_open(app->storage, app->file);
    fedit_file_realloc_buffer(app->file, FEDIT_EDITOR_BUF_SIZE);
    fedit_file_update_buffer(app->file, FEDIT_EDITOR_BYTES_PER_LINE);

    fedit_editor_set_callback(app->editor, fedit_editor_callback, app);
    view_dispatcher_switch_to_view(app->view_dispatcher, FeditViewIdEditor);
}

bool fedit_scene_editor_on_event(void* ctx, SceneManagerEvent event) {
    FeditApp* app = ctx;
    bool consumed = false;

    if (event.type == SceneManagerEventTypeCustom) {
        switch (event.event)
        {
        break;
        case FeditEventEditorBack:
            if (!scene_manager_search_and_switch_to_previous_scene(app->scene_manager, FeditSceneMenu)) {
                scene_manager_stop(app->scene_manager);
                view_dispatcher_stop(app->view_dispatcher);
            }
            consumed = true;
        break;
        default:
        break;
        }
    }
    return consumed;
}

void fedit_scene_editor_on_exit(void* ctx) {
    FeditApp* app = ctx;
    fedit_file_stream_end(app->file);
}