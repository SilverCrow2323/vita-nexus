#include <psp2/ctrl.h>
#include <psp2/kernel/processmgr.h>
#include <vita2d.h>

#include "catalog_index.h"
#include "menu_state.h"
#include "library_store.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define LIBRARY_PATH "ux0:data/VitaNexus-library-v1.dat"

#define COLOR_BG       RGBA8(11, 9, 16, 255)
#define COLOR_PANEL    RGBA8(24, 17, 29, 255)
#define COLOR_PINK     RGBA8(255, 61, 168, 255)
#define COLOR_ACID     RGBA8(234, 255, 59, 255)
#define COLOR_CYAN     RGBA8(44, 232, 242, 255)
#define COLOR_PAPER    RGBA8(247, 241, 232, 255)
#define COLOR_MUTED    RGBA8(174, 160, 180, 255)
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
    {"LIBRARY", "LOCAL COLLECTION READY", "Favorites, backlog, completion and personal scores."},
    {"DISCOVER", "SHARED CATALOG READY", "Browse Vita, PSP, PS1 and homebrew metadata."},
    {"RANKINGS", "PLANNED", "Personal scores, lists and public chart snapshots."},
    {"SETTINGS", "PLANNED", "Profiles, storage and offline preferences."}
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
    float scale, const char *value, size_t max_bytes, int max_lines)
{
    if (!value || !value[0]) {
        vita2d_pgf_draw_text(font, x, y, COLOR_MUTED, scale, "No editorial summary available.");
        return;
    }

    const char *cursor = value;
    for (int line = 0; line < max_lines && *cursor; ++line) {
        size_t length = strlen(cursor);
        if (length > max_bytes) {
            size_t split = max_bytes;
            while (split > 0 && cursor[split] != ' ') --split;
            if (split == 0) split = max_bytes;
            length = split;
        }
        char buffer[128];
        if (length >= sizeof(buffer)) length = sizeof(buffer) - 1;
        memcpy(buffer, cursor, length);
        buffer[length] = '\0';
        vita2d_pgf_draw_text(font, x, y + (line * 24), color, scale, buffer);
        cursor += length;
        while (*cursor == ' ') ++cursor;
    }
}

static void draw_home(vita2d_pgf *font, int selected)
{
    const int menu_x = 54, menu_y = 190, row_h = 58;
    vita2d_draw_rectangle(0, 0, 960, 544, COLOR_BG);
    vita2d_draw_rectangle(0, 0, 960, 8, COLOR_PINK);
    vita2d_draw_rectangle(40, 38, 880, 2, COLOR_PAPER);
    vita2d_pgf_draw_text(font, 52, 76, COLOR_ACID, 1.25f, "VITA NEXUS");
    vita2d_pgf_draw_text(font, 53, 101, COLOR_CYAN, 0.72f, "THE BACKLOG NEVER SLEEPS");
    vita2d_pgf_draw_text(font, 760, 72, COLOR_PINK, 0.65f, "NATIVE / 00.03");
    vita2d_draw_rectangle(48, 132, 864, 1, COLOR_PINK);
    vita2d_pgf_draw_text(font, 54, 163, COLOR_MUTED, 0.68f, "01  /  SELECT A MODULE");

    for (int i = 0; i < 4; ++i) {
        const int y = menu_y + (i * row_h);
        const bool active = i == selected;
        if (active) {
            vita2d_draw_rectangle(menu_x - 8, y - 22, 388, 44, COLOR_PANEL);
            vita2d_draw_rectangle(menu_x - 8, y - 22, 5, 44, COLOR_PINK);
        }
        vita2d_pgf_draw_textf(font, menu_x + 10, y + 5, active ? COLOR_ACID : COLOR_MUTED,
            0.9f, "%02d  %s", i + 1, menu_items[i].label);
    }

    vita2d_draw_rectangle(490, 184, 420, 228, COLOR_PANEL);
    vita2d_draw_rectangle(490, 184, 420, 4, COLOR_CYAN);
    vita2d_pgf_draw_text(font, 516, 220, COLOR_PINK, 0.72f, "MODULE STATUS");
    vita2d_pgf_draw_text(font, 516, 264, COLOR_PAPER, 0.9f, menu_items[selected].label);
    vita2d_pgf_draw_text(font, 516, 294, COLOR_ACID, 0.62f, menu_items[selected].status);
    vita2d_pgf_draw_text(font, 516, 344, COLOR_MUTED, 0.58f, menu_items[selected].description);
    vita2d_draw_rectangle(48, 448, 864, 1, COLOR_PAPER);
    vita2d_pgf_draw_text(font, 54, 484, COLOR_CYAN, 0.62f, "D-PAD SELECT    X OPEN    START EXIT");
    vita2d_pgf_draw_text(font, 54, 501, COLOR_MUTED, 0.48f, "DISCOVER READS THE SHARED, OFFLINE CATALOG");
    vita2d_pgf_draw_text(font, 54, 516, COLOR_MUTED, 0.52f, "NO INSTALLATION SCAN  /  NO SYSTEM FILES MODIFIED");
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
    vita2d_pgf_draw_text(font, 48, 62, COLOR_ACID, 1.0f, "DISCOVER / CATALOG");
    vita2d_pgf_draw_textf(font, 650, 60, COLOR_PINK, 0.54f, "%lu / %lu",
        (unsigned long)visible_count, (unsigned long)vita_nexus_catalog_count);
    vita2d_pgf_draw_textf(font, 48, 105, COLOR_CYAN, 0.48f, "FILTER  %s     INITIAL  %s",
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
        vita2d_pgf_draw_textf(font, 58, y, index == selected ? COLOR_ACID : COLOR_PAPER,
            0.66f, "%s", entry->title);
        vita2d_pgf_draw_textf(font, 790, y, COLOR_CYAN, 0.5f, "%s", platform_label(entry->platform_mask));
    }

    if (selected < visible_count) {
        const VitaNexusCatalogEntry *entry = &vita_nexus_catalog[visible_games[selected]];
        vita2d_draw_rectangle(48, 448, 864, 1, COLOR_PAPER);
        vita2d_pgf_draw_textf(font, 54, 477, COLOR_PINK, 0.58f, "GAME ID  %s", entry->game_id);
        vita2d_pgf_draw_textf(font, 54, 499, COLOR_CYAN, 0.58f, "%u RELEASE RECORDS  /  %s",
            (unsigned int)entry->release_count, platform_label(entry->platform_mask));
    }
    vita2d_pgf_draw_text(font, 54, 525, COLOR_MUTED, 0.46f, "UP/DOWN BROWSE  L/R INITIAL  TRIANGLE PLATFORM  SQUARE RESET  X DETAILS");
}

static VitaNexusLibrary g_library;

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

static void save_library(const VitaNexusLibrary *library)
{
    (void)vita_nexus_library_save(library, LIBRARY_PATH);
}

static void draw_library(vita2d_pgf *font, const VitaNexusLibrary *library, size_t selected)
{
    const size_t start = selected >= CATALOG_ROWS ? selected - CATALOG_ROWS + 1 : 0;
    vita2d_draw_rectangle(0, 0, 960, 544, COLOR_BG);
    vita2d_draw_rectangle(0, 0, 960, 8, COLOR_PINK);
    vita2d_pgf_draw_text(font, 48, 62, COLOR_ACID, 1.0f, "MY LIBRARY / OFFLINE");
    vita2d_pgf_draw_textf(font, 760, 60, COLOR_CYAN, 0.6f, "%lu ITEMS", (unsigned long)library->count);
    vita2d_draw_rectangle(48, 86, 864, 2, COLOR_PINK);
    if (library->count == 0) {
        vita2d_pgf_draw_text(font, 72, 180, COLOR_PAPER, 0.85f, "YOUR COLLECTION IS A BLANK PAGE.");
        vita2d_pgf_draw_text(font, 72, 220, COLOR_MUTED, 0.62f, "Open a game in DISCOVER and use L / R / TRIANGLE to track it.");
    }
    for (size_t row = 0; row < CATALOG_ROWS && start + row < library->count; ++row) {
        const VitaNexusLibraryRecord *record = &library->records[start + row];
        const int y = 135 + (int)row * 38;
        if (start + row == selected) {
            vita2d_draw_rectangle(42, y - 22, 876, 34, COLOR_PANEL);
            vita2d_draw_rectangle(42, y - 22, 4, 34, COLOR_CYAN);
        }
        vita2d_pgf_draw_text(font, 58, y, start + row == selected ? COLOR_ACID : COLOR_PAPER, 0.64f, record->game_id);
        vita2d_pgf_draw_textf(font, 650, y, COLOR_PINK, 0.5f, "%s%s%s  /  %u/10",
            (record->flags & VITA_NEXUS_LIBRARY_FAVORITE) ? "FAV " : "",
            (record->flags & VITA_NEXUS_LIBRARY_BACKLOG) ? "BACKLOG " : "",
            (record->flags & VITA_NEXUS_LIBRARY_COMPLETED) ? "DONE" : "",
            (unsigned int)record->rating);
    }
    vita2d_draw_rectangle(48, 448, 864, 1, COLOR_PAPER);
    vita2d_pgf_draw_text(font, 54, 484, COLOR_CYAN, 0.55f, "UP/DOWN SELECT  X OPEN GAME  CIRCLE HOME");
    vita2d_pgf_draw_text(font, 54, 508, COLOR_MUTED, 0.48f, "LOCAL FILE ONLY  /  NO GAME SAVES OR SYSTEM DATA TOUCHED");
}

static void draw_game_detail(vita2d_pgf *font, size_t selected)
{
    if (selected >= vita_nexus_catalog_count) return;
    const VitaNexusCatalogEntry *entry = &vita_nexus_catalog[selected];

    vita2d_draw_rectangle(0, 0, 960, 544, COLOR_BG);
    vita2d_draw_rectangle(0, 0, 960, 8, COLOR_PINK);
    vita2d_pgf_draw_text(font, 48, 54, COLOR_CYAN, 0.65f, "DISCOVER / GAME FILE");
    vita2d_pgf_draw_text(font, 48, 102, COLOR_ACID, 1.05f, entry->title);
    vita2d_draw_rectangle(48, 124, 864, 2, COLOR_PINK);

    vita2d_draw_rectangle(48, 150, 864, 112, COLOR_PANEL);
    vita2d_pgf_draw_text(font, 70, 180, COLOR_PINK, 0.62f, "PLATFORMS");
    vita2d_pgf_draw_text(font, 70, 212, COLOR_PAPER, 0.9f, platform_label(entry->platform_mask));
    vita2d_pgf_draw_textf(font, 600, 180, COLOR_PINK, 0.62f, "RELEASES  %u",
        (unsigned int)entry->release_count);
    if (entry->release_year)
        vita2d_pgf_draw_textf(font, 600, 212, COLOR_CYAN, 0.72f, "YEAR  %u",
            (unsigned int)entry->release_year);
    else
        vita2d_pgf_draw_text(font, 600, 212, COLOR_MUTED, 0.62f, "YEAR  UNKNOWN");

    vita2d_pgf_draw_text(font, 54, 300, COLOR_ACID, 0.65f, "DEVELOPER");
    vita2d_pgf_draw_text(font, 54, 326, COLOR_PAPER, 0.62f,
        entry->developer[0] ? entry->developer : "Not available");
    vita2d_pgf_draw_text(font, 500, 300, COLOR_ACID, 0.65f, "PUBLISHER");
    vita2d_pgf_draw_text(font, 500, 326, COLOR_PAPER, 0.62f,
        entry->publisher[0] ? entry->publisher : "Not available");

    vita2d_pgf_draw_text(font, 54, 365, COLOR_PINK, 0.62f, "GENRES");
    vita2d_pgf_draw_text(font, 54, 389, COLOR_PAPER, 0.58f,
        entry->genres[0] ? entry->genres : "Not classified");

    const uint8_t flags = library_flags(&g_library, entry->game_id);
    const uint8_t rating = library_rating(&g_library, entry->game_id);
    vita2d_pgf_draw_text(font, 54, 416, COLOR_CYAN, 0.58f, "YOUR TRACKER");
    vita2d_pgf_draw_textf(font, 54, 438, COLOR_PAPER, 0.52f, "%s%s%s   SCORE %u/10",
        (flags & VITA_NEXUS_LIBRARY_FAVORITE) ? "FAVORITE  " : "",
        (flags & VITA_NEXUS_LIBRARY_BACKLOG) ? "BACKLOG  " : "",
        (flags & VITA_NEXUS_LIBRARY_COMPLETED) ? "COMPLETED" : "NOT TRACKED",
        (unsigned int)rating);
    vita2d_pgf_draw_text(font, 54, 462, COLOR_PINK, 0.55f, "EDITORIAL SUMMARY");
    draw_wrapped(font, 54, 484, COLOR_PAPER, 0.48f, entry->summary, 145, 1);

    vita2d_draw_rectangle(48, 518, 864, 1, COLOR_PAPER);
    vita2d_pgf_draw_text(font, 54, 539, COLOR_MUTED, 0.42f, "L FAV  R BACKLOG  TRIANGLE DONE  LEFT/RIGHT SCORE  CIRCLE BACK");
}

int main(void)
{
    SceCtrlData pad;
    uint32_t previous_buttons = 0;
    VitaNexusMenuState menu_state = {0};
    size_t catalog_selected = 0;
    size_t detail_catalog_index = 0;
    size_t library_selected = 0;
    unsigned int platform_filter = 0;
    int letter_filter = -1;
    bool in_discover = false;
    bool in_library = false;
    bool detail_from_library = false;
    bool in_detail = false;
    bool running = true;

    sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);
    if (vita2d_init() < 0) {
        sceKernelExitProcess(1);
        return 1;
    }

    vita2d_set_clear_color(COLOR_BG);
    vita_nexus_library_init(&g_library);
    (void)vita_nexus_library_load(&g_library, LIBRARY_PATH);
    vita2d_pgf *font = vita2d_load_default_pgf();
    if (!font) {
        vita2d_fini();
        sceKernelExitProcess(2);
        return 2;
    }

    while (running) {
        sceCtrlPeekBufferPositive(0, &pad, 1);
        const uint32_t pressed = pad.buttons & ~previous_buttons;
        previous_buttons = pad.buttons;

        if (in_detail) {
            if (detail_catalog_index < vita_nexus_catalog_count) {
                const char *game_id = vita_nexus_catalog[detail_catalog_index].game_id;
                const VitaNexusLibraryRecord *existing = vita_nexus_library_find(&g_library, game_id);
                uint8_t flags = existing ? existing->flags : 0;
                uint8_t rating = existing ? existing->rating : 0;
                if (pressed & SCE_CTRL_LTRIGGER) { flags ^= VITA_NEXUS_LIBRARY_FAVORITE; vita_nexus_library_set(&g_library, game_id, flags, rating); save_library(&g_library); }
                if (pressed & SCE_CTRL_RTRIGGER) { flags ^= VITA_NEXUS_LIBRARY_BACKLOG; vita_nexus_library_set(&g_library, game_id, flags, rating); save_library(&g_library); }
                if (pressed & SCE_CTRL_TRIANGLE) { flags ^= VITA_NEXUS_LIBRARY_COMPLETED; vita_nexus_library_set(&g_library, game_id, flags, rating); save_library(&g_library); }
                if (pressed & SCE_CTRL_LEFT) { if (rating > 0) --rating; vita_nexus_library_set(&g_library, game_id, flags, rating); save_library(&g_library); }
                if (pressed & SCE_CTRL_RIGHT) { if (rating < 10) ++rating; vita_nexus_library_set(&g_library, game_id, flags, rating); save_library(&g_library); }
            }
            if (pressed & SCE_CTRL_CIRCLE) { in_detail = false; if (detail_from_library) in_library = true; }
        } else if (in_library) {
            if (pressed & SCE_CTRL_UP) { if (library_selected > 0) --library_selected; }
            else if (pressed & SCE_CTRL_DOWN) { if (library_selected + 1 < g_library.count) ++library_selected; }
            if ((pressed & SCE_CTRL_CROSS) && library_selected < g_library.count) {
                const char *game_id = g_library.records[library_selected].game_id;
                for (size_t i = 0; i < vita_nexus_catalog_count; ++i) {
                    if (strcmp(vita_nexus_catalog[i].game_id, game_id) == 0) {
                        detail_catalog_index = i; in_detail = true; detail_from_library = true; in_library = false; break;
                    }
                }
            }
            if (pressed & SCE_CTRL_CIRCLE) in_library = false;
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
            if ((pressed & SCE_CTRL_CROSS) && catalog_selected < visible_count) { detail_catalog_index = visible_games[catalog_selected]; in_detail = true; detail_from_library = false; }
            if (pressed & SCE_CTRL_CIRCLE) in_discover = false;
        } else {
            if (pressed & SCE_CTRL_UP) vita_nexus_menu_move(&menu_state, -1);
            else if (pressed & SCE_CTRL_DOWN) vita_nexus_menu_move(&menu_state, 1);
            if ((pressed & SCE_CTRL_CROSS) && menu_state.selected == 0) { in_library = true; library_selected = 0; }
            if ((pressed & SCE_CTRL_CROSS) && menu_state.selected == 1) {
                in_discover = true;
                detail_from_library = false;
                rebuild_visible_games(platform_filter, letter_filter);
                catalog_selected = 0;
            }
            if (pressed & SCE_CTRL_START) running = false;
        }

        vita2d_start_drawing();
        vita2d_clear_screen();
        if (in_detail) draw_game_detail(font, detail_catalog_index);
        else if (in_library) draw_library(font, &g_library, library_selected);
        else if (in_discover) draw_discover(font, catalog_selected, platform_filter, letter_filter);
        else draw_home(font, (int)menu_state.selected);
        vita2d_end_drawing();
        vita2d_swap_buffers();
    }

    vita2d_free_pgf(font);
    vita2d_fini();
    sceKernelExitProcess(0);
    return 0;
}
