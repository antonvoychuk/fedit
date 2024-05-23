#pragma once

#include <gui/scene_manager.h>

// Generate scene id and total number
#define ADD_SCENE(prefix, name, id) FeditScene##id,
typedef enum {
#include "fedit_scene_config.h"
    FeditSceneNum,
} FeditScene;
#undef ADD_SCENE

extern const SceneManagerHandlers fedit_scene_handlers;

// Generate scene on_enter handlers declaration
#define ADD_SCENE(prefix, name, id) void prefix##_scene_##name##_on_enter(void* ctx);
#include "fedit_scene_config.h"
#undef ADD_SCENE

// Generate scene on_event handlers declaration
#define ADD_SCENE(prefix, name, id) bool prefix##_scene_##name##_on_event(void* ctx, SceneManagerEvent event);
#include "fedit_scene_config.h"
#undef ADD_SCENE

// Generate scene on_xit handlers declaration
#define ADD_SCENE(prefix, name, id) void prefix##_scene_##name##_on_exit(void* ctx);
#include "fedit_scene_config.h"
#undef ADD_SCENE
