#pragma once

typedef enum {
    FeditEventEditorBack,
} FeditCustomEvent;

enum FeditCustomEventType {
    //Reserve first 100 events for button types and indexes, starting from 0
    FeditEventMenuVoid,
    FeditEventMenuSelected,
};