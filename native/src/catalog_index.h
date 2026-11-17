#ifndef VITA_NEXUS_CATALOG_INDEX_H
#define VITA_NEXUS_CATALOG_INDEX_H

#include <stddef.h>
#include <stdint.h>

enum {
    VITA_NEXUS_PLATFORM_PSVITA = 1u << 0,
    VITA_NEXUS_PLATFORM_PSP = 1u << 1,
    VITA_NEXUS_PLATFORM_PS1 = 1u << 2,
    VITA_NEXUS_PLATFORM_HOMEBREW = 1u << 3
};

typedef struct {
    const char *game_id;
    const char *title;
    const char *summary;
    const char *developer;
    const char *publisher;
    const char *genres;
    const char *content_kind;
    const char *features;
    const char *themes;
    const char *age_rating;
    const char *age_descriptors;
    const char *age_rating_source;
    const char *score_provider;
    const char *score_source;
    uint8_t platform_mask;
    uint16_t release_count;
    uint16_t release_year;
    float score_value;
    float score_scale;
    uint32_t score_votes;
    uint8_t has_external_score;
} VitaNexusCatalogEntry;

extern const VitaNexusCatalogEntry vita_nexus_catalog[];
extern const size_t vita_nexus_catalog_count;

#endif
