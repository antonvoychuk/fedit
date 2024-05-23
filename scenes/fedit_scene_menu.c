#include "../fedit.h"

#include <gui/modules/submenu.h>

enum SubmenuIndex {
    SubmenuIndexEditor,
    SubmenuIndexOpenFile,
};

void fedit_scene_menu_submenu_callback(void* ctx, uint32_t index) {
    FeditApp* app = ctx;
    view_dispatcher_send_custom_event(app->view_dispatcher, index);
}

void fedit_scene_menu_on_enter(void* ctx) {
    FeditApp* app = ctx;

    submenu_add_item(
        app->submenu,
        "Editor",
        SubmenuIndexEditor,
        fedit_scene_menu_submenu_callback,
        app
    );
    submenu_add_item(
        app->submenu,
        "Open file",
        SubmenuIndexOpenFile,
        fedit_scene_menu_submenu_callback,
        app
    );

    submenu_set_selected_item(app->submenu, scene_manager_get_scene_state(app->scene_manager, FeditSceneMenu));
    view_dispatcher_switch_to_view(app->view_dispatcher, FeditViewIdMenu);
}

bool fedit_scene_menu_on_event(void* ctx, SceneManagerEvent event) {
    FeditApp* app = ctx;

    if(event.type == SceneManagerEventTypeBack) {
        // exit app
        scene_manager_stop(app->scene_manager);
        view_dispatcher_stop(app->view_dispatcher);
        return true;
    } else if (event.type == SceneManagerEventTypeCustom) {
        if (event.event == SubmenuIndexEditor) {
            scene_manager_set_scene_state(app->scene_manager, FeditSceneMenu, SubmenuIndexEditor);
            if (fedit_file_validate(app->storage, app->file))
                scene_manager_next_scene(app->scene_manager, FeditSceneEditor);
            return true;
        } else if (event.event == SubmenuIndexOpenFile) {
            scene_manager_set_scene_state(app->scene_manager, FeditSceneMenu, SubmenuIndexOpenFile);
            fedit_file_browse(app->file, FEDIT_EXTENSION);
            return true;
        }
    }

    return false;
}

void fedit_scene_menu_on_exit(void* ctx) {
    FeditApp* app = ctx;
    submenu_reset(app->submenu);
}