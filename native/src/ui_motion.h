#ifndef VITA_NEXUS_UI_MOTION_H
#define VITA_NEXUS_UI_MOTION_H

#include <stdint.h>

typedef enum {
    VITA_NEXUS_SCREEN_HOME = 0,
    VITA_NEXUS_SCREEN_LIBRARY = 1,
    VITA_NEXUS_SCREEN_DISCOVER = 2,
    VITA_NEXUS_SCREEN_PROFILES = 3,
    VITA_NEXUS_SCREEN_DETAIL = 4
} VitaNexusScreenId;

typedef struct {
    uint32_t frame;
    uint8_t fade_alpha;
} VitaNexusUiMotion;

static inline void vita_nexus_ui_motion_init(VitaNexusUiMotion *motion)
{
    if (!motion) return;
    motion->frame = 0;
    motion->fade_alpha = 0;
}

static inline void vita_nexus_ui_motion_begin_transition(VitaNexusUiMotion *motion)
{
    if (!motion) return;
    motion->fade_alpha = 160;
}

static inline uint8_t vita_nexus_ui_motion_tick(VitaNexusUiMotion *motion)
{
    if (!motion) return 0;
    ++motion->frame;
    if (motion->fade_alpha > 20) motion->fade_alpha = (uint8_t)(motion->fade_alpha - 20);
    else motion->fade_alpha = 0;
    return motion->fade_alpha;
}

static inline unsigned int vita_nexus_ui_motion_pulse(
    const VitaNexusUiMotion *motion, unsigned int amplitude, unsigned int period)
{
    if (!motion || amplitude == 0 || period < 2) return 0;
    const unsigned int phase = motion->frame % period;
    const unsigned int half_period = period / 2;
    const unsigned int ramp = phase <= half_period ? phase : period - phase;
    return (ramp * amplitude) / half_period;
}

static inline VitaNexusScreenId vita_nexus_ui_screen_id(
    int in_detail, int in_profiles, int in_library, int in_discover)
{
    if (in_detail) return VITA_NEXUS_SCREEN_DETAIL;
    if (in_profiles) return VITA_NEXUS_SCREEN_PROFILES;
    if (in_library) return VITA_NEXUS_SCREEN_LIBRARY;
    if (in_discover) return VITA_NEXUS_SCREEN_DISCOVER;
    return VITA_NEXUS_SCREEN_HOME;
}

#endif
