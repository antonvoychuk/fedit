#include "fedit.h"

#include <furi_hal.h>

bool fedit_custom_event_callback(void* ctx, uint32_t event) {
    furi_assert(ctx);
    FeditApp* app = ctx;
    return scene_manager_handle_custom_event(app->scene_manager, event);
}

void fedit_tick_event_callback(void* ctx) {
    furi_assert(ctx);
    FeditApp* app = ctx;
    scene_manager_handle_tick_event(app->scene_manager);
}

bool fedit_navigation_event_callback(void* ctx) {
    furi_assert(ctx);
    FeditApp* app = ctx;
    return scene_manager_handle_back_event(app->scene_manager);
}

FeditApp* app_alloc(char* arg) {
    FeditApp* app = malloc(sizeof(FeditApp));

    // Open records
    app->gui = furi_record_open(RECORD_GUI);
    app->storage = furi_record_open(RECORD_STORAGE);
    app->notification = furi_record_open(RECORD_NOTIFICATION);

    // Enable display backlight
    notification_message(app->notification, &sequence_display_backlight_on);

    // File
    app->file = fedit_file_alloc();
    furi_string_set(app->file->path, arg != NULL ? arg : FEDIT_PATH);

    // Scene manager
    app->scene_manager = scene_manager_alloc(&fedit_scene_handlers, app);

    // View dispatcher
    app->view_dispatcher = view_dispatcher_alloc();
    view_dispatcher_enable_queue(app->view_dispatcher);
    view_dispatcher_set_event_callback_context(app->view_dispatcher, app);

    // Set custom callbacks
    view_dispatcher_set_navigation_event_callback(app->view_dispatcher, fedit_navigation_event_callback);
    view_dispatcher_set_tick_event_callback(app->view_dispatcher, fedit_tick_event_callback, 100);
    view_dispatcher_set_custom_event_callback(app->view_dispatcher, fedit_custom_event_callback);

    // Alloc views...
    app->submenu = submenu_alloc();
    app->editor = fedit_editor_alloc(app);

    // ...and add them to view dispatcher
    view_dispatcher_add_view(app->view_dispatcher, FeditViewIdMenu, submenu_get_view(app->submenu));
    view_dispatcher_add_view(app->view_dispatcher, FeditViewIdEditor, fedit_editor_get_view(app->editor));

    // Attach view dispatcher to GUI
    view_dispatcher_attach_to_gui(app->view_dispatcher, app->gui, ViewDispatcherTypeFullscreen);

    return app;
}

void app_free(FeditApp* app) {
    furi_assert(app);

    // Remove views
    view_dispatcher_remove_view(app->view_dispatcher, FeditViewIdMenu);
    view_dispatcher_remove_view(app->view_dispatcher, FeditViewIdEditor);

    // Free views
    submenu_free(app->submenu);
    fedit_editor_free(app->editor);

    // Scene manager
    scene_manager_free(app->scene_manager);

    // View dispatcher
    view_dispatcher_free(app->view_dispatcher);
    
    // Close records
    furi_record_close(RECORD_GUI);
    furi_record_close(RECORD_NOTIFICATION);
    furi_record_close(RECORD_STORAGE);

    // Storage
    fedit_file_free(app->file);

    // Empty pointers
    app->gui = NULL;
    app->notification = NULL;

    // Freee app pointer
    free(app);
}

int32_t fedit(void* p) {
    FeditApp* app = app_alloc((char*)p);

    scene_manager_next_scene(app->scene_manager, FeditSceneMenu);

    furi_hal_power_suppress_charge_enter();

    view_dispatcher_run(app->view_dispatcher);

    furi_hal_power_suppress_charge_exit();
    app_free(app);
    return 0;
}