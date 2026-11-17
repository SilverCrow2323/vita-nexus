#include <psp2/ctrl.h>
#include <psp2/kernel/processmgr.h>
#include <vita2d.h>

#include "catalog_index.h"
#include "menu_state.h"
#include "library_store.h"
#include "profile_store.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#define ACTIVE_PROFILE_PATH "ux0:data/VitaNexus-active-profile-v1.dat"

#define COLOR_BG       RGBA8(11, 9, 16, 255)
#define COLOR_PANEL    RGBA8(24, 17, 29, 255)
#define COLOR_PINK     RGBA8(255, 61, 168, 255)
#define COLOR_ACID     RGBA8(234, 255, 59, 255)
#define COLOR_CYAN     RGBA8(44, 232, 242, 255)
#define COLOR_PAPER    RGBA8(247, 241, 232, 255)
#define COLOR_MUTED    RGBA8(174, 160, 180, 255)

#define UI_FONT_MEDIUM_PATH "app0:assets/fonts/BarlowCondensed-Medium.ttf"
#define UI_FONT_BOLD_PATH   "app0:assets/fonts/BarlowCondensed-Bold.ttf"

static vita2d_font *ui_font_medium;
static vita2d_font *ui_font_bold;
static vita2d_pgf *ui_fallback_font;

static unsigned int ui_font_size(float scale)
{
    int size = (int)(scale * 24.0f + 0.5f);
    if (size < 10) size = 10;
    if (size > 32) size = 32;
    return (unsigned int)size;
}

static vita2d_font *ui_font_for_scale(float scale)
{
    if (scale >= 0.82f && ui_font_bold) return ui_font_bold;
    if (ui_font_medium) return ui_font_medium;
    return ui_font_bold;
}

static void ui_draw_text(int x, int y, unsigned int color, float scale, const char *text)
{
    vita2d_font *selected = ui_font_for_scale(scale);
    if (selected) {
        (void)vita2d_font_draw_text(selected, x, y, color, ui_font_size(scale), text ? text : "");
    } else if (ui_fallback_font) {
        (void)vita2d_pgf_draw_text(ui_fallback_font, x, y, color, scale, text ? text : "");
    }
}

static void ui_draw_textf(int x, int y, unsigned int color, float scale, const char *format, ...)
{
    char buffer[512];
    va_list args;
    va_start(args, format);
    (void)vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    ui_draw_text(x, y, color, scale, buffer);
}

static int ui_text_width_at_size(float scale, unsigned int size, const char *text)
{
    vita2d_font *selected = ui_font_for_scale(scale);
    if (selected) return vita2d_font_text_width(selected, size, text ? text : "");
    if (ui_fallback_font) return vita2d_pgf_text_width(ui_fallback_font, scale, text ? text : "");
    return 0;
}

static void ui_draw_fit_text(int x, int y, unsigned int color, float scale,
    int max_width, const char *text)
{
    unsigned int size = ui_font_size(scale);
    vita2d_font *selected = ui_font_for_scale(scale);
    if (!selected) {
        ui_draw_text(x, y, color, scale, text);
        return;
    }
    while (size > 14 && ui_text_width_at_size(scale, size, text) > max_width) {
        --size;
        /* Re-measure using the smaller size below rather than changing global style. */
        int measured = vita2d_font_text_width(selected, size, text ? text : "");
        if (measured <= max_width) break;
    }
    (void)vita2d_font_draw_text(selected, x, y, color, size, text ? text : "");
}

static void ui_draw_text_with_font(vita2d_pgf *fallback_font, int x, int y,
    unsigned int color, float scale, const char *text)
{
    (void)fallback_font;
    ui_draw_text(x, y, color, scale, text);
}

static void ui_draw_textf_with_font(vita2d_pgf *fallback_font, int x, int y,
    unsigned int color, float scale, const char *format, ...)
{
    char buffer[512];
    va_list args;
    (void)fallback_font;
    va_start(args, format);
    (void)vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    ui_draw_text(x, y, color, scale, buffer);
}

#define CATALOG_ROWS 8
#define MAX_VISIBLE_GAMES 20000

static size_t visible_games[MAX_VISIBLE_GAMES];
static size_t visible_count = 0;
static const char *platform_filters[] = {"ALL PLATFORMS", "PS VITA", "PSP", "PS1", "HOMEBREW"};
static const uint8_t platform_filter_bits[] = {0, VITA_NEXUS_PLATFORM_PSVITA, VITA_NEXUS_PLATFORM_PSP, VITA_NEXUS_PLATFORM_PS1, VITA_NEXUS_PLATFORM_HOMEBREW};

typedef struct {
    const char *label;
    const char *status;
    const char *description;
} MenuItem;

static const MenuItem menu_items[] = {
    {"LIBRARY", "LOCAL COLLECTION READY", "Filter, review and clean up your personal game tracker."},
    {"DISCOVER", "SHARED CATALOG READY", "Browse Vita, PSP, PS1 and homebrew metadata."},
    {"RANKINGS", "PLANNED", "Personal scores, lists and public chart snapshots."},
    {"PROFILES", "3 LOCAL SLOTS", "Switch collections and create portable backups."},
    {"SETTINGS", "PLANNED", "Offline preferences and interface options."}
};

static const char *platform_label(uint8_t mask)
{
    if (mask & VITA_NEXUS_PLATFORM_HOMEBREW) return "HOMEBREW";
    if ((mask & (VITA_NEXUS_PLATFORM_PSVITA | VITA_NEXUS_PLATFORM_PSP | VITA_NEXUS_PLATFORM_PS1)) ==
        (VITA_NEXUS_PLATFORM_PSVITA | VITA_NEXUS_PLATFORM_PSP | VITA_NEXUS_PLATFORM_PS1)) return "VITA / PSP / PS1";
    if (mask & VITA_NEXUS_PLATFORM_PSVITA) return "PS VITA";
    if (mask & VITA_NEXUS_PLATFORM_PSP) return "PSP";
    if (mask & VITA_NEXUS_PLATFORM_PS1) return "PS1";
    return "MULTI";
}

static void draw_wrapped(vita2d_pgf *font, int x, int y, unsigned int color,
    float scale, const char *value, int max_width, int max_lines)
{
    if (!value || !value[0]) {
        ui_draw_text_with_font(font, x, y, COLOR_MUTED, scale, "No editorial summary available.");
        return;
    }

    const char *cursor = value;
    int line_number = 0;
    char line[256] = "";
    while (*cursor && line_number < max_lines) {
        while (*cursor == ' ' || *cursor == '\\t' || *cursor == '\\n') ++cursor;
        if (!*cursor) break;

        char word[128];
        size_t word_length = 0;
        while (cursor[word_length] && cursor[word_length] != ' ' &&
               cursor[word_length] != '\\t' && cursor[word_length] != '\\n' &&
               word_length < sizeof(word) - 1) {
            ++word_length;
        }
        if (word_length == 0) break;
        memcpy(word, cursor, word_length);
        word[word_length] = '\0';
        cursor += word_length;

        char candidate[256];
        (void)snprintf(candidate, sizeof(candidate), "%s%s%s",
            line, line[0] ? " " : "", word);
        if (line[0] && ui_text_width_at_size(scale, ui_font_size(scale), candidate) > max_width) {
            ui_draw_text_with_font(font, x, y + (line_number * 18), color, scale, line);
            ++line_number;
            line[0] = '\0';
            (void)snprintf(line, sizeof(line), "%s", word);
        } else {
            (void)snprintf(line, sizeof(line), "%s", candidate);
        }
    }
    if (line[0] && line_number < max_lines) {
        ui_draw_text_with_font(font, x, y + (line_number * 18), color, scale, line);
    }
}

static void draw_home(vita2d_pgf *font, int selected)
{
    const int menu_x = 54, menu_y = 184, row_h = 48;
    vita2d_draw_rectangle(0, 0, 960, 544, COLOR_BG);
    vita2d_draw_rectangle(0, 0, 960, 8, COLOR_PINK);
    vita2d_draw_rectangle(40, 38, 880, 2, COLOR_PAPER);
    ui_draw_text_with_font(font, 52, 76, COLOR_ACID, 1.25f, "VITA NEXUS");
    ui_draw_text_with_font(font, 53, 101, COLOR_CYAN, 0.72f, "THE BACKLOG NEVER SLEEPS");
    ui_draw_text_with_font(font, 760, 72, COLOR_PINK, 0.65f, "NATIVE / 00.07");
    vita2d_draw_rectangle(48, 132, 864, 1, COLOR_PINK);
    ui_draw_text_with_font(font, 54, 163, COLOR_MUTED, 0.68f, "01  /  SELECT A MODULE");

    for (int i = 0; i < 5; ++i) {
        const int y = menu_y + (i * row_h);
        const bool active = i == selected;
        if (active) {
            vita2d_draw_rectangle(menu_x - 8, y - 22, 388, 44, COLOR_PANEL);
            vita2d_draw_rectangle(menu_x - 8, y - 22, 5, 44, COLOR_PINK);
        }
        ui_draw_textf_with_font(font, menu_x + 10, y + 5, active ? COLOR_ACID : COLOR_MUTED,
            0.9f, "%02d  %s", i + 1, menu_items[i].label);
    }

    vita2d_draw_rectangle(490, 184, 420, 228, COLOR_PANEL);
    vita2d_draw_rectangle(490, 184, 420, 4, COLOR_CYAN);
    ui_draw_text_with_font(font, 516, 220, COLOR_PINK, 0.72f, "MODULE STATUS");
    ui_draw_text_with_font(font, 516, 264, COLOR_PAPER, 0.9f, menu_items[selected].label);
    ui_draw_text_with_font(font, 516, 294, COLOR_ACID, 0.62f, menu_items[selected].status);
    ui_draw_text_with_font(font, 516, 344, COLOR_MUTED, 0.58f, menu_items[selected].description);
    vita2d_draw_rectangle(48, 448, 864, 1, COLOR_PAPER);
    ui_draw_text_with_font(font, 54, 484, COLOR_CYAN, 0.62f, "D-PAD SELECT    X OPEN    START EXIT");
    ui_draw_text_with_font(font, 54, 501, COLOR_MUTED, 0.48f, "DISCOVER READS THE SHARED, OFFLINE CATALOG");
    ui_draw_text_with_font(font, 54, 516, COLOR_MUTED, 0.52f, "NO INSTALLATION SCAN  /  NO SYSTEM FILES MODIFIED");
}


static void rebuild_visible_games(unsigned int platform_filter, int letter_filter)
{
    visible_count = 0;
    for (size_t i = 0; i < vita_nexus_catalog_count && visible_count < MAX_VISIBLE_GAMES; ++i) {
        const VitaNexusCatalogEntry *entry = &vita_nexus_catalog[i];
        if (platform_filter && !(entry->platform_mask & platform_filter_bits[platform_filter])) continue;
        if (letter_filter >= 0) {
            const unsigned char first = (unsigned char)entry->title[0];
            const int normalized = (first >= 'a' && first <= 'z') ? first - 'a' + 'A' : first;
            if (normalized != ('A' + letter_filter)) continue;
        }
        visible_games[visible_count++] = i;
    }
}

static void draw_discover(vita2d_pgf *font, size_t selected, unsigned int platform_filter, int letter_filter)
{
    const size_t start = selected >= CATALOG_ROWS ? selected - CATALOG_ROWS + 1 : 0;
    vita2d_draw_rectangle(0, 0, 960, 544, COLOR_BG);
    vita2d_draw_rectangle(0, 0, 960, 8, COLOR_CYAN);
    ui_draw_text_with_font(font, 48, 62, COLOR_ACID, 1.0f, "DISCOVER / CATALOG");
    ui_draw_textf_with_font(font, 650, 60, COLOR_PINK, 0.54f, "%lu / %lu",
        (unsigned long)visible_count, (unsigned long)vita_nexus_catalog_count);
    ui_draw_textf_with_font(font, 48, 105, COLOR_CYAN, 0.48f, "FILTER  %s     INITIAL  %s",
        platform_filters[platform_filter], letter_filter < 0 ? "ALL" : (char[2]){'A' + letter_filter, '\0'});
    vita2d_draw_rectangle(48, 86, 864, 2, COLOR_PINK);

    for (size_t row = 0; row < CATALOG_ROWS && start + row < visible_count; ++row) {
        const size_t index = visible_games[start + row];
        const VitaNexusCatalogEntry *entry = &vita_nexus_catalog[index];
        const int y = 125 + (int)row * 39;
        if (index == selected) {
            vita2d_draw_rectangle(42, y - 22, 876, 34, COLOR_PANEL);
            vita2d_draw_rectangle(42, y - 22, 4, 34, COLOR_PINK);
        }
        ui_draw_textf_with_font(font, 58, y, index == selected ? COLOR_ACID : COLOR_PAPER,
            0.66f, "%s", entry->title);
        ui_draw_textf_with_font(font, 790, y, COLOR_CYAN, 0.5f, "%s", platform_label(entry->platform_mask));
    }

    if (selected < visible_count) {
        const VitaNexusCatalogEntry *entry = &vita_nexus_catalog[visible_games[selected]];
        vita2d_draw_rectangle(48, 448, 864, 1, COLOR_PAPER);
        ui_draw_textf_with_font(font, 54, 477, COLOR_PINK, 0.58f, "GAME ID  %s", entry->game_id);
        ui_draw_textf_with_font(font, 54, 499, COLOR_CYAN, 0.58f, "%u RELEASE RECORDS  /  %s",
            (unsigned int)entry->release_count, platform_label(entry->platform_mask));
    }
    ui_draw_text_with_font(font, 54, 525, COLOR_MUTED, 0.46f, "UP/DOWN BROWSE  L/R INITIAL  TRIANGLE PLATFORM  SQUARE RESET  X DETAILS");
}

static VitaNexusLibrary g_library;
static unsigned int active_profile = 0;
static bool active_profile_read_only = false;
static size_t library_visible[VITA_NEXUS_LIBRARY_MAX_RECORDS];
static size_t library_visible_count = 0;
static unsigned int library_filter = 0;

static const char *library_filter_labels[] = {"ALL ITEMS", "FAVORITES", "BACKLOG", "COMPLETED"};

static uint8_t library_flags(const VitaNexusLibrary *library, const char *game_id)
{
    const VitaNexusLibraryRecord *record = vita_nexus_library_find(library, game_id);
    return record ? record->flags : 0;
}

static uint8_t library_rating(const VitaNexusLibrary *library, const char *game_id)
{
    const VitaNexusLibraryRecord *record = vita_nexus_library_find(library, game_id);
    return record ? record->rating : 0;
}

static VitaNexusStoreResult save_library(const VitaNexusLibrary *library)
{
    if (active_profile_read_only) return VITA_NEXUS_STORE_INVALID;
    return vita_nexus_library_save(library, vita_nexus_profile_data_path(active_profile));
}

static void load_startup_profile(void)
{
    unsigned int selected = 0;
    VitaNexusStoreResult result = vita_nexus_profile_load_active(ACTIVE_PROFILE_PATH, &selected);
    if (result == VITA_NEXUS_STORE_OK) active_profile = selected;
    else active_profile = 0;

    vita_nexus_library_init(&g_library);
    result = vita_nexus_library_load(&g_library, vita_nexus_profile_data_path(active_profile));
    if (result == VITA_NEXUS_STORE_INVALID || result == VITA_NEXUS_STORE_IO_ERROR) {
        vita_nexus_library_init(&g_library);
        active_profile_read_only = true;
    } else {
        active_profile_read_only = false;
    }
}

static void rebuild_visible_library(const VitaNexusLibrary *library, unsigned int filter)
{
    library_visible_count = 0;
    if (!library || library->count > VITA_NEXUS_LIBRARY_MAX_RECORDS) return;
    for (size_t i = 0; i < library->count; ++i) {
        const VitaNexusLibraryRecord *record = &library->records[i];
        uint8_t required = 0;
        if (filter == 1) required = VITA_NEXUS_LIBRARY_FAVORITE;
        else if (filter == 2) required = VITA_NEXUS_LIBRARY_BACKLOG;
        else if (filter == 3) required = VITA_NEXUS_LIBRARY_COMPLETED;
        if (required && !(record->flags & required)) continue;
        library_visible[library_visible_count++] = i;
    }
}

static VitaNexusStoreResult read_profile(unsigned int profile, VitaNexusLibrary *library)
{
    if (profile >= VITA_NEXUS_PROFILE_COUNT || !library) return VITA_NEXUS_STORE_INVALID;
    vita_nexus_library_init(library);
    return vita_nexus_library_load(library, vita_nexus_profile_data_path(profile));
}

static void activate_profile(unsigned int selected, const char **notice)
{
    VitaNexusLibrary next;
    VitaNexusStoreResult result;
    if (selected >= VITA_NEXUS_PROFILE_COUNT || !notice) return;
    if (selected == active_profile) {
        *notice = "THIS PROFILE IS ALREADY ACTIVE";
        return;
    }
    if (!active_profile_read_only && save_library(&g_library) != VITA_NEXUS_STORE_OK) {
        *notice = "CURRENT PROFILE COULD NOT BE SAVED";
        return;
    }
    result = read_profile(selected, &next);
    if (result == VITA_NEXUS_STORE_INVALID || result == VITA_NEXUS_STORE_IO_ERROR) {
        *notice = "TARGET PROFILE IS DAMAGED - RESTORE ITS BACKUP";
        return;
    }
    if (vita_nexus_profile_save_active(ACTIVE_PROFILE_PATH, selected) != VITA_NEXUS_STORE_OK) {
        *notice = "ACTIVE PROFILE SETTING COULD NOT BE SAVED";
        return;
    }
    active_profile = selected;
    g_library = next;
    active_profile_read_only = false;
    library_filter = 0;
    rebuild_visible_library(&g_library, library_filter);
    *notice = "PROFILE SWITCHED - COLLECTION LOADED";
}

static void draw_library(vita2d_pgf *font, const VitaNexusLibrary *library,
    size_t selected, unsigned int filter, bool remove_confirm)
{
    const size_t start = selected >= CATALOG_ROWS ? selected - CATALOG_ROWS + 1 : 0;
    VitaNexusProfileStats stats;
    vita_nexus_profile_get_stats(library, &stats);
    vita2d_draw_rectangle(0, 0, 960, 544, COLOR_BG);
    vita2d_draw_rectangle(0, 0, 960, 8, COLOR_PINK);
    ui_draw_textf_with_font(font, 48, 62, COLOR_ACID, 1.0f, "MY LIBRARY / PLAYER %u", active_profile + 1);
    ui_draw_textf_with_font(font, 760, 60, COLOR_CYAN, 0.6f, "%lu ITEMS",
        (unsigned long)stats.total);
    vita2d_draw_rectangle(48, 86, 864, 2, COLOR_PINK);
    ui_draw_textf_with_font(font, 54, 112, COLOR_PINK, 0.52f, "%s   FAV %lu   BACKLOG %lu   DONE %lu   RATED %lu",
        library_filter_labels[filter], (unsigned long)stats.favorites, (unsigned long)stats.backlog,
        (unsigned long)stats.completed, (unsigned long)stats.rated);

    if (library_visible_count == 0) {
        ui_draw_text_with_font(font, 72, 210, COLOR_PAPER, 0.82f, "NO GAMES IN THIS VIEW.");
        ui_draw_text_with_font(font, 72, 246, COLOR_MUTED, 0.58f, "Track games from DISCOVER or switch the L/R filter.");
    }
    for (size_t row = 0; row < CATALOG_ROWS && start + row < library_visible_count; ++row) {
        const VitaNexusLibraryRecord *record = &library->records[library_visible[start + row]];
        const int y = 158 + (int)row * 34;
        if (start + row == selected) {
            vita2d_draw_rectangle(42, y - 20, 876, 30, COLOR_PANEL);
            vita2d_draw_rectangle(42, y - 20, 4, 30, COLOR_CYAN);
        }
        ui_draw_text_with_font(font, 58, y, start + row == selected ? COLOR_ACID : COLOR_PAPER, 0.6f, record->game_id);
        ui_draw_textf_with_font(font, 650, y, COLOR_PINK, 0.46f, "%s%s%s  /  %u/10",
            (record->flags & VITA_NEXUS_LIBRARY_FAVORITE) ? "FAV " : "",
            (record->flags & VITA_NEXUS_LIBRARY_BACKLOG) ? "BACKLOG " : "",
            (record->flags & VITA_NEXUS_LIBRARY_COMPLETED) ? "DONE" : "",
            (unsigned int)record->rating);
    }
    vita2d_draw_rectangle(48, 448, 864, 1, COLOR_PAPER);
    if (active_profile_read_only) {
        ui_draw_text_with_font(font, 54, 478, COLOR_PINK, 0.52f, "PROFILE DATA IS DAMAGED - RESTORE A VALID BACKUP IN PROFILES");
    } else if (remove_confirm) {
        ui_draw_text_with_font(font, 54, 478, COLOR_PINK, 0.55f, "REMOVE THIS TRACKER ENTRY?  X CONFIRM  CIRCLE CANCEL");
    } else {
        ui_draw_text_with_font(font, 54, 478, COLOR_CYAN, 0.52f, "UP/DOWN SELECT  X OPEN  L/R FILTER  SQUARE REMOVE");
    }
    ui_draw_text_with_font(font, 54, 508, COLOR_MUTED, 0.46f, "CIRCLE HOME  /  LOCAL COLLECTION ONLY");
}

static void draw_profiles(vita2d_pgf *font, size_t selected, bool import_confirm, const char *notice)
{
    vita2d_draw_rectangle(0, 0, 960, 544, COLOR_BG);
    vita2d_draw_rectangle(0, 0, 960, 8, COLOR_CYAN);
    ui_draw_text_with_font(font, 48, 62, COLOR_ACID, 1.0f, "LOCAL PROFILES / BACKUP BAY");
    vita2d_draw_rectangle(48, 86, 864, 2, COLOR_PINK);

    for (unsigned int i = 0; i < VITA_NEXUS_PROFILE_COUNT; ++i) {
        VitaNexusLibrary snapshot;
        VitaNexusProfileStats stats;
        const VitaNexusStoreResult result = read_profile(i, &snapshot);
        vita_nexus_profile_get_stats(&snapshot, &stats);
        const int y = 148 + (int)i * 91;
        const bool active = i == active_profile;
        const bool focus = i == selected;
        if (focus) {
            vita2d_draw_rectangle(42, y - 30, 876, 76, COLOR_PANEL);
            vita2d_draw_rectangle(42, y - 30, 5, 76, COLOR_PINK);
        }
        ui_draw_textf_with_font(font, 64, y, focus ? COLOR_ACID : COLOR_PAPER, 0.82f,
            "PLAYER %02u%s", i + 1, active ? "  /  ACTIVE" : "");
        if (result == VITA_NEXUS_STORE_INVALID || result == VITA_NEXUS_STORE_IO_ERROR) {
            ui_draw_text_with_font(font, 500, y, COLOR_PINK, 0.58f, "DATA ERROR - RESTORE BACKUP");
        } else {
            ui_draw_textf_with_font(font, 500, y, COLOR_CYAN, 0.56f,
                "%lu ITEMS  /  FAV %lu  BACKLOG %lu  DONE %lu",
                (unsigned long)stats.total, (unsigned long)stats.favorites,
                (unsigned long)stats.backlog, (unsigned long)stats.completed);
        }
        ui_draw_textf_with_font(font, 64, y + 25, COLOR_MUTED, 0.48f,
            "RATING DATA  %lu ENTRIES", (unsigned long)stats.rated);
    }

    vita2d_draw_rectangle(48, 448, 864, 1, COLOR_PAPER);
    if (import_confirm) {
        ui_draw_textf_with_font(font, 54, 476, COLOR_PINK, 0.56f, "RESTORE PLAYER %02lu FROM BACKUP?", (unsigned long)selected + 1);
        ui_draw_text_with_font(font, 54, 501, COLOR_CYAN, 0.52f, "X CONFIRM OVERWRITE  /  CIRCLE CANCEL");
    } else {
        ui_draw_text_with_font(font, 54, 476, COLOR_CYAN, 0.52f, "UP/DOWN SLOT  X ACTIVATE  TRIANGLE EXPORT  SQUARE RESTORE");
        ui_draw_text_with_font(font, 54, 501, COLOR_MUTED, 0.48f, "CIRCLE HOME  /  RESTORE REQUIRES A SECOND CONFIRMATION");
    }
    if (notice && notice[0]) ui_draw_text_with_font(font, 54, 528, COLOR_ACID, 0.44f, notice);
}

static void draw_game_detail(vita2d_pgf *font, size_t selected)
{
    if (selected >= vita_nexus_catalog_count) return;
    const VitaNexusCatalogEntry *entry = &vita_nexus_catalog[selected];

    vita2d_draw_rectangle(0, 0, 960, 544, COLOR_BG);
    vita2d_draw_rectangle(0, 0, 960, 8, COLOR_PINK);
    ui_draw_text_with_font(font, 48, 54, COLOR_CYAN, 0.65f, "DISCOVER / GAME FILE");
    ui_draw_fit_text(48, 102, COLOR_ACID, 1.05f, 864, entry->title);
    vita2d_draw_rectangle(48, 124, 864, 2, COLOR_PINK);

    vita2d_draw_rectangle(48, 150, 864, 112, COLOR_PANEL);
    ui_draw_text_with_font(font, 70, 180, COLOR_PINK, 0.62f, "PLATFORMS");
    ui_draw_text_with_font(font, 70, 212, COLOR_PAPER, 0.9f, platform_label(entry->platform_mask));
    ui_draw_textf_with_font(font, 600, 180, COLOR_PINK, 0.62f, "RELEASES  %u",
        (unsigned int)entry->release_count);
    if (entry->release_year)
        ui_draw_textf_with_font(font, 600, 212, COLOR_CYAN, 0.72f, "YEAR  %u",
            (unsigned int)entry->release_year);
    else
        ui_draw_text_with_font(font, 600, 212, COLOR_MUTED, 0.62f, "YEAR  UNKNOWN");

    ui_draw_text_with_font(font, 54, 300, COLOR_ACID, 0.65f, "DEVELOPER");
    ui_draw_text_with_font(font, 54, 326, COLOR_PAPER, 0.62f,
        entry->developer[0] ? entry->developer : "Not available");
    ui_draw_text_with_font(font, 500, 300, COLOR_ACID, 0.65f, "PUBLISHER");
    ui_draw_text_with_font(font, 500, 326, COLOR_PAPER, 0.62f,
        entry->publisher[0] ? entry->publisher : "Not available");

    ui_draw_text_with_font(font, 54, 365, COLOR_PINK, 0.62f, "GENRES");
    ui_draw_text_with_font(font, 54, 389, COLOR_PAPER, 0.58f,
        entry->genres[0] ? entry->genres : "Not classified");

    const uint8_t flags = library_flags(&g_library, entry->game_id);
    const uint8_t rating = library_rating(&g_library, entry->game_id);
    ui_draw_text_with_font(font, 54, 416, COLOR_CYAN, 0.58f, "YOUR TRACKER");
    ui_draw_textf_with_font(font, 54, 438, COLOR_PAPER, 0.52f, "%s%s%s   SCORE %u/10",
        (flags & VITA_NEXUS_LIBRARY_FAVORITE) ? "FAVORITE  " : "",
        (flags & VITA_NEXUS_LIBRARY_BACKLOG) ? "BACKLOG  " : "",
        (flags & VITA_NEXUS_LIBRARY_COMPLETED) ? "COMPLETED" : "NOT TRACKED",
        (unsigned int)rating);
    ui_draw_text_with_font(font, 54, 462, COLOR_PINK, 0.55f, "EDITORIAL SUMMARY");
    draw_wrapped(font, 54, 484, COLOR_PAPER, 0.48f, entry->summary, 850, 2);

    vita2d_draw_rectangle(48, 518, 864, 1, COLOR_PAPER);
    ui_draw_text_with_font(font, 54, 539, COLOR_MUTED, 0.42f, "L FAV  R BACKLOG  TRIANGLE DONE  LEFT/RIGHT SCORE  CIRCLE BACK");
}

int main(void)
{
    SceCtrlData pad;
    uint32_t previous_buttons = 0;
    VitaNexusMenuState menu_state = {0};
    size_t catalog_selected = 0;
    size_t detail_catalog_index = 0;
    size_t library_selected = 0;
    size_t profile_selected = 0;
    unsigned int platform_filter = 0;
    int letter_filter = -1;
    bool in_discover = false;
    bool in_library = false;
    bool in_profiles = false;
    bool profile_import_confirm = false;
    bool library_remove_confirm = false;
    bool detail_from_library = false;
    bool in_detail = false;
    bool running = true;
    const char *profile_notice = "";

    sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);
    if (vita2d_init() < 0) {
        sceKernelExitProcess(1);
        return 1;
    }

    vita2d_set_clear_color(COLOR_BG);
    vita_nexus_library_init(&g_library);
    load_startup_profile();
    rebuild_visible_library(&g_library, library_filter);
    ui_font_medium = vita2d_load_font_file(UI_FONT_MEDIUM_PATH);
    ui_font_bold = vita2d_load_font_file(UI_FONT_BOLD_PATH);
    ui_fallback_font = vita2d_load_default_pgf();
    vita2d_pgf *font = ui_fallback_font;
    if (!ui_font_medium && !ui_font_bold && !font) {
        vita2d_fini();
        sceKernelExitProcess(2);
        return 2;
    }

    while (running) {
        sceCtrlPeekBufferPositive(0, &pad, 1);
        const uint32_t pressed = pad.buttons & ~previous_buttons;
        previous_buttons = pad.buttons;

        if (in_detail) {
            if (detail_catalog_index < vita_nexus_catalog_count && !active_profile_read_only) {
                const char *game_id = vita_nexus_catalog[detail_catalog_index].game_id;
                const VitaNexusLibraryRecord *existing = vita_nexus_library_find(&g_library, game_id);
                uint8_t flags = existing ? existing->flags : 0;
                uint8_t rating = existing ? existing->rating : 0;
                bool changed = false;
                if (pressed & SCE_CTRL_LTRIGGER) { flags ^= VITA_NEXUS_LIBRARY_FAVORITE; changed = true; }
                if (pressed & SCE_CTRL_RTRIGGER) { flags ^= VITA_NEXUS_LIBRARY_BACKLOG; changed = true; }
                if (pressed & SCE_CTRL_TRIANGLE) { flags ^= VITA_NEXUS_LIBRARY_COMPLETED; changed = true; }
                if (pressed & SCE_CTRL_LEFT) { if (rating > 0) --rating; changed = true; }
                if (pressed & SCE_CTRL_RIGHT) { if (rating < 10) ++rating; changed = true; }
                if (changed && vita_nexus_library_set(&g_library, game_id, flags, rating) == VITA_NEXUS_STORE_OK)
                    (void)save_library(&g_library);
            }
            if (pressed & SCE_CTRL_CIRCLE) {
                in_detail = false;
                if (detail_from_library) {
                    in_library = true;
                    rebuild_visible_library(&g_library, library_filter);
                    if (library_selected >= library_visible_count && library_visible_count > 0)
                        library_selected = library_visible_count - 1;
                }
            }
        } else if (in_profiles) {
            if (profile_import_confirm) {
                if (pressed & SCE_CTRL_CIRCLE) {
                    profile_import_confirm = false;
                    profile_notice = "RESTORE CANCELLED";
                } else if ((pressed & SCE_CTRL_CROSS) && profile_selected < VITA_NEXUS_PROFILE_COUNT) {
                    VitaNexusLibrary imported;
                    const VitaNexusStoreResult result = vita_nexus_profile_import_backup(
                        vita_nexus_profile_backup_path((unsigned int)profile_selected),
                        vita_nexus_profile_data_path((unsigned int)profile_selected), &imported);
                    if (result == VITA_NEXUS_STORE_OK) {
                        if (profile_selected == active_profile) {
                            g_library = imported;
                            active_profile_read_only = false;
                            rebuild_visible_library(&g_library, library_filter);
                        }
                        profile_notice = "BACKUP RESTORED - COLLECTION RELOADED";
                    } else if (result == VITA_NEXUS_STORE_NOT_FOUND) {
                        profile_notice = "NO BACKUP FOUND FOR THIS PROFILE";
                    } else {
                        profile_notice = "BACKUP INVALID - CURRENT PROFILE LEFT UNCHANGED";
                    }
                    profile_import_confirm = false;
                }
            } else {
                if (pressed & SCE_CTRL_UP) { if (profile_selected > 0) --profile_selected; }
                else if (pressed & SCE_CTRL_DOWN) {
                    if (profile_selected + 1 < VITA_NEXUS_PROFILE_COUNT) ++profile_selected;
                }
                if (pressed & SCE_CTRL_CROSS) activate_profile((unsigned int)profile_selected, &profile_notice);
                if (pressed & SCE_CTRL_TRIANGLE) {
                    const VitaNexusStoreResult result = vita_nexus_profile_export_backup(
                        vita_nexus_profile_data_path((unsigned int)profile_selected),
                        vita_nexus_profile_backup_path((unsigned int)profile_selected));
                    profile_notice = result == VITA_NEXUS_STORE_OK ? "BACKUP EXPORTED TO APP DATA" :
                        (result == VITA_NEXUS_STORE_NOT_FOUND ? "PROFILE HAS NO DATA TO EXPORT" : "BACKUP EXPORT FAILED");
                }
                if (pressed & SCE_CTRL_SQUARE) {
                    profile_import_confirm = true;
                    profile_notice = "";
                }
                if (pressed & SCE_CTRL_CIRCLE) in_profiles = false;
            }
        } else if (in_library) {
            if (library_remove_confirm) {
                if (pressed & SCE_CTRL_CIRCLE) library_remove_confirm = false;
                else if ((pressed & SCE_CTRL_CROSS) && library_selected < library_visible_count && !active_profile_read_only) {
                    const size_t record_index = library_visible[library_selected];
                    const char *game_id = g_library.records[record_index].game_id;
                    if (vita_nexus_library_set(&g_library, game_id, 0, 0) == VITA_NEXUS_STORE_OK) {
                        (void)save_library(&g_library);
                        rebuild_visible_library(&g_library, library_filter);
                        if (library_selected >= library_visible_count && library_visible_count > 0)
                            library_selected = library_visible_count - 1;
                    }
                    library_remove_confirm = false;
                }
            } else {
                if (pressed & SCE_CTRL_UP) { if (library_selected > 0) --library_selected; }
                else if (pressed & SCE_CTRL_DOWN) {
                    if (library_selected + 1 < library_visible_count) ++library_selected;
                }
                if (pressed & SCE_CTRL_LTRIGGER) {
                    library_filter = (library_filter + 3) % 4;
                    library_selected = 0;
                    rebuild_visible_library(&g_library, library_filter);
                } else if (pressed & SCE_CTRL_RTRIGGER) {
                    library_filter = (library_filter + 1) % 4;
                    library_selected = 0;
                    rebuild_visible_library(&g_library, library_filter);
                }
                if ((pressed & SCE_CTRL_CROSS) && library_selected < library_visible_count) {
                    const size_t record_index = library_visible[library_selected];
                    const char *game_id = g_library.records[record_index].game_id;
                    for (size_t i = 0; i < vita_nexus_catalog_count; ++i) {
                        if (strcmp(vita_nexus_catalog[i].game_id, game_id) == 0) {
                            detail_catalog_index = i;
                            in_detail = true;
                            detail_from_library = true;
                            in_library = false;
                            break;
                        }
                    }
                }
                if ((pressed & SCE_CTRL_SQUARE) && library_selected < library_visible_count && !active_profile_read_only)
                    library_remove_confirm = true;
                if (pressed & SCE_CTRL_CIRCLE) in_library = false;
            }
        } else if (in_discover) {
            if (pressed & SCE_CTRL_UP) {
                if (catalog_selected > 0) --catalog_selected;
            } else if (pressed & SCE_CTRL_DOWN) {
                if (catalog_selected + 1 < visible_count) ++catalog_selected;
            }
            if (pressed & SCE_CTRL_LTRIGGER) {
                letter_filter = letter_filter < 0 ? 25 : (letter_filter + 25) % 26;
                catalog_selected = 0;
                rebuild_visible_games(platform_filter, letter_filter);
            } else if (pressed & SCE_CTRL_RTRIGGER) {
                letter_filter = letter_filter < 0 ? 0 : (letter_filter + 1) % 26;
                catalog_selected = 0;
                rebuild_visible_games(platform_filter, letter_filter);
            }
            if (pressed & SCE_CTRL_TRIANGLE) {
                platform_filter = (platform_filter + 1) % 5;
                catalog_selected = 0;
                rebuild_visible_games(platform_filter, letter_filter);
            }
            if (pressed & SCE_CTRL_SQUARE) {
                platform_filter = 0;
                letter_filter = -1;
                catalog_selected = 0;
                rebuild_visible_games(platform_filter, letter_filter);
            }
            if ((pressed & SCE_CTRL_CROSS) && catalog_selected < visible_count) {
                detail_catalog_index = visible_games[catalog_selected];
                in_detail = true;
                detail_from_library = false;
            }
            if (pressed & SCE_CTRL_CIRCLE) in_discover = false;
        } else {
            if (pressed & SCE_CTRL_UP) vita_nexus_menu_move(&menu_state, -1);
            else if (pressed & SCE_CTRL_DOWN) vita_nexus_menu_move(&menu_state, 1);
            if ((pressed & SCE_CTRL_CROSS) && menu_state.selected == 0) {
                in_library = true;
                library_selected = 0;
                library_filter = 0;
                rebuild_visible_library(&g_library, library_filter);
            }
            if ((pressed & SCE_CTRL_CROSS) && menu_state.selected == 1) {
                in_discover = true;
                detail_from_library = false;
                rebuild_visible_games(platform_filter, letter_filter);
                catalog_selected = 0;
            }
            if ((pressed & SCE_CTRL_CROSS) && menu_state.selected == 3) {
                in_profiles = true;
                profile_selected = active_profile;
                profile_notice = "";
                profile_import_confirm = false;
            }
            if (pressed & SCE_CTRL_START) running = false;
        }

        vita2d_start_drawing();
        vita2d_clear_screen();
        if (in_detail) draw_game_detail(font, detail_catalog_index);
        else if (in_profiles) draw_profiles(font, profile_selected, profile_import_confirm, profile_notice);
        else if (in_library) draw_library(font, &g_library, library_selected, library_filter, library_remove_confirm);
        else if (in_discover) draw_discover(font, catalog_selected, platform_filter, letter_filter);
        else draw_home(font, (int)menu_state.selected);
        vita2d_end_drawing();
        vita2d_swap_buffers();
    }

    if (ui_font_medium) vita2d_free_font(ui_font_medium);
    if (ui_font_bold) vita2d_free_font(ui_font_bold);
    if (font) vita2d_free_pgf(font);
    vita2d_fini();
    sceKernelExitProcess(0);
    return 0;
}
