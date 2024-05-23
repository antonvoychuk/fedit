#pragma once

#include <furi.h>

#include <gui/gui.h>
#include <gui/view_dispatcher.h>
#include <gui/scene_manager.h>
#include <storage/storage.h>
#include <dialogs/dialogs.h>

#include <gui/modules/submenu.h>

#include <gui/elements.h>
#include <notification/notification_messages.h>

#include "helpers/fedit_files.h"
#include "scenes/fedit_scene.h"
#include "views/fedit_editor.h"

#define FEDIT_PATH STORAGE_APP_DATA_PATH_PREFIX
#define FEDIT_CACHE_FOLDER STORAGE_APP_DATA_PATH_PREFIX "/.cache"
#define FEDIT_EXTENSION "*"
#define FEDIT_MAX_FILENAME_LENGTH 64

typedef struct {
    Gui* gui;
    ViewDispatcher* view_dispatcher;
    SceneManager* scene_manager;
    Submenu* submenu;
    NotificationApp* notification;
    Storage* storage;

    FeditEditor* editor;
    FeditFile* file;
} FeditApp;

typedef enum {
    FeditViewIdMenu,
    FeditViewIdEditor,
} FeditViewId;