#ifndef VITA_NEXUS_MENU_STATE_H
#define VITA_NEXUS_MENU_STATE_H

#include <stddef.h>

enum { VITA_NEXUS_MENU_COUNT = 4 };

typedef struct {
    size_t selected;
} VitaNexusMenuState;

/* Pure input state: host-testable without VitaSDK or a renderer. */
static inline void vita_nexus_menu_move(VitaNexusMenuState *state, int direction)
{
    if (state == NULL || direction == 0) {
        return;
    }

    if (state->selected >= VITA_NEXUS_MENU_COUNT) {
        state->selected = 0;
    }

    if (direction < 0) {
        state->selected = (state->selected + VITA_NEXUS_MENU_COUNT - 1)
            % VITA_NEXUS_MENU_COUNT;
    } else {
        state->selected = (state->selected + 1) % VITA_NEXUS_MENU_COUNT;
    }
}

#endif
