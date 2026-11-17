#include "ui_motion.h"

#include <assert.h>
#include <stddef.h>

int main(void)
{
    VitaNexusUiMotion motion;
    vita_nexus_ui_motion_init(&motion);
    assert(motion.frame == 0);
    assert(motion.fade_alpha == 0);
    assert(vita_nexus_ui_motion_tick(NULL) == 0);
    vita_nexus_ui_motion_begin_transition(NULL);

    vita_nexus_ui_motion_begin_transition(&motion);
    assert(motion.fade_alpha == 160);
    assert(vita_nexus_ui_motion_tick(&motion) == 140);
    assert(vita_nexus_ui_motion_tick(&motion) == 120);
    assert(vita_nexus_ui_motion_tick(&motion) == 100);
    assert(vita_nexus_ui_motion_tick(&motion) == 80);
    assert(vita_nexus_ui_motion_tick(&motion) == 60);
    assert(vita_nexus_ui_motion_tick(&motion) == 40);
    assert(vita_nexus_ui_motion_tick(&motion) == 20);
    assert(vita_nexus_ui_motion_tick(&motion) == 0);
    assert(vita_nexus_ui_motion_tick(&motion) == 0);
    assert(motion.frame == 9);

    motion.frame = 0;
    assert(vita_nexus_ui_motion_pulse(&motion, 16, 48) == 0);
    motion.frame = 12;
    assert(vita_nexus_ui_motion_pulse(&motion, 16, 48) == 8);
    motion.frame = 24;
    assert(vita_nexus_ui_motion_pulse(&motion, 16, 48) == 16);
    motion.frame = 36;
    assert(vita_nexus_ui_motion_pulse(&motion, 16, 48) == 8);
    motion.frame = 48;
    assert(vita_nexus_ui_motion_pulse(&motion, 16, 48) == 0);
    assert(vita_nexus_ui_motion_pulse(&motion, 0, 48) == 0);
    assert(vita_nexus_ui_motion_pulse(&motion, 16, 1) == 0);
    assert(vita_nexus_ui_motion_pulse(NULL, 16, 48) == 0);

    assert(vita_nexus_ui_screen_id(0, 0, 0, 0) == VITA_NEXUS_SCREEN_HOME);
    assert(vita_nexus_ui_screen_id(0, 0, 0, 1) == VITA_NEXUS_SCREEN_DISCOVER);
    assert(vita_nexus_ui_screen_id(0, 0, 1, 0) == VITA_NEXUS_SCREEN_LIBRARY);
    assert(vita_nexus_ui_screen_id(0, 1, 0, 0) == VITA_NEXUS_SCREEN_PROFILES);
    assert(vita_nexus_ui_screen_id(1, 1, 1, 1) == VITA_NEXUS_SCREEN_DETAIL);
    return 0;
}
