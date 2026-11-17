#include <psp2/ctrl.h>
#include <psp2/kernel/processmgr.h>
#include <vita2d.h>

#include "catalog_index.h"
#include "menu_state.h"
#include "library_store.h"
#include "profile_store.h"
#include "ui_motion.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#define ACTIVE_PROFILE_PATH "ux0:data/VitaNexus-active-profile-v1.dat"

#define COLOR_BG       RGBA8(8, 7, 12, 255)
#define COLOR_SHEET    RGBA8(16, 13, 21, 255)
#define COLOR_PANEL    RGBA8(24, 17, 29, 255)
#define COLOR_CARD     RGBA8(34, 26, 39, 255)
#define COLOR_HAIRLINE RGBA8(76, 61, 78, 255)
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
static VitaNexusUiMotion g_ui_motion;

static unsigned int ui_font_size(float scale)
{
    int size = (int)(scale * 26.0f + 0.5f);
    if (size < 11) size = 11;
    if (size > 34) size = 34;
    return (unsigned int)size;
}

static vita2d_font *ui_font_for_scale(float scale)
{
    if (scale >= 0.82f && ui_font_bold) return ui_font_bold;
    if (ui_font_medium) return ui_font_medium;
    return ui_font_bold;
}

static bool ui_text_has_non_ascii(const char *text)
{
    if (!text) return false;
    for (const unsigned char *cursor = (const unsigned char *)text; *cursor; ++cursor) {
        if (*cursor >= 0x80u) return true;
    }
    return false;
}

static void ui_draw_text(int x, int y, unsigned int color, float scale, const char *text)
{
    vita2d_font *selected = ui_text_has_non_ascii(text) ? NULL : ui_font_for_scale(scale);
    if (selected) {
        (void)vita2d_font_draw_text(selected, x, y, color, ui_font_size(scale), text ? text : "");
    } else if (ui_fallback_font) {
        (void)vita2d_pgf_draw_text(ui_fallback_font, x, y, color, scale, text ? text : "");
    }
}

static int ui_text_width_at_size(float scale, unsigned int size, const char *text)
{
    vita2d_font *selected = ui_text_has_non_ascii(text) ? NULL : ui_font_for_scale(scale);
    if (selected) return vita2d_font_text_width(selected, size, text ? text : "");
    if (ui_fallback_font) return vita2d_pgf_text_width(ui_fallback_font, scale, text ? text : "");
    return 0;
}

static void ui_draw_fit_text(int x, int y, unsigned int color, float scale,
    int max_width, const char *text)
{
    vita2d_font *selected = ui_text_has_non_ascii(text) ? NULL : ui_font_for_scale(scale);
    if (!selected) {
        float fitted_scale = scale;
        while (ui_fallback_font && fitted_scale > 0.42f &&
            vita2d_pgf_text_width(ui_fallback_font, fitted_scale, text ? text : "") > max_width) {
            fitted_scale -= 0.04f;
        }
        ui_draw_text(x, y, color, fitted_scale, text);
        return;
    }

    unsigned int size = ui_font_size(scale);
    char fitted[256];
    (void)snprintf(fitted, sizeof(fitted), "%s", text ? text : "");
    while (size > 12 && vita2d_font_text_width(selected, size, fitted) > max_width) {
        --size;
    }

    if (vita2d_font_text_width(selected, size, fitted) > max_width) {
        const int ellipsis_width = vita2d_font_text_width(selected, size, "...");
        size_t length = strlen(fitted);
        while (length > 0 &&
            vita2d_font_text_width(selected, size, fitted) + ellipsis_width > max_width) {
            fitted[--length] = '\0';
        }
        if (length > 0 && ellipsis_width <= max_width) {
            (void)strncat(fitted, "...", sizeof(fitted) - strlen(fitted) - 1);
        }
    }
    (void)vita2d_font_draw_text(selected, x, y, color, size, fitted);
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

#define CATALOG_ROWS 6
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
        while (*cursor == ' ' || *cursor == '\t' || *cursor == '\n') ++cursor;
        if (!*cursor) break;

        char word[128];
        size_t word_length = 0;
        while (cursor[word_length] && cursor[word_length] != ' ' &&
               cursor[word_length] != '\t' && cursor[word_length] != '\n' &&
               word_length < sizeof(word) - 1) {
            ++word_length;
        }
        if (word_length == 0) break;
        memcpy(word, cursor, word_length);
        word[word_length] = '\0';
        cursor += word_length;

        const size_t line_length = strlen(line);
        const size_t candidate_word_length = strlen(word);
        const size_t separator = line_length ? 1u : 0u;
        char candidate[256];
        candidate[0] = '\0';
        if (line_length + separator + candidate_word_length < sizeof(candidate)) {
            (void)memcpy(candidate, line, line_length);
            if (separator) candidate[line_length] = ' ';
            (void)memcpy(candidate + line_length + separator, word, candidate_word_length + 1u);
        } else {
            candidate[0] = '\0';
        }
        if (line[0] && (!candidate[0] ||
            ui_text_width_at_size(scale, ui_font_size(scale), candidate) > max_width)) {
            ui_draw_text_with_font(font, x, y + (line_number * 18), color, scale, line);
            ++line_number;
            (void)snprintf(line, sizeof(line), "%s", word);
        } else if (candidate[0]) {
            (void)snprintf(line, sizeof(line), "%s", candidate);
        } else {
            (void)snprintf(line, sizeof(line), "%s", word);
        }
    }
    if (line[0] && line_number < max_lines) {
        ui_draw_text_with_font(font, x, y + (line_number * 18), color, scale, line);
    }
}


/* -------------------------------------------------------------------------
 * VITA NEXUS / PLAYER'S FIELD MAGAZINE
 *
 * Original editorial UI language: loud cover lines, printed folios, compact
 * columns, hard rules, issue codes and source-led information panels.
 * Designed for the Vita's fixed 960x544 framebuffer and controller focus.
 * ---------------------------------------------------------------------- */

static void draw_magazine_barcode(int x, int y, unsigned int color)
{
    static const uint8_t bars[] = {2, 1, 3, 1, 2, 4, 1, 2, 1, 3, 2, 1, 4, 1, 2};
    int cursor = x;
    for (size_t i = 0; i < sizeof(bars) / sizeof(bars[0]); ++i) {
        vita2d_draw_rectangle(cursor, y, bars[i], 15, color);
        cursor += (int)bars[i] + 2;
    }
}

static void draw_magazine_canvas(unsigned int accent)
{
    vita2d_draw_rectangle(0, 0, 960, 544, COLOR_BG);
    vita2d_draw_rectangle(18, 14, 924, 516, COLOR_SHEET);
    vita2d_draw_rectangle(18, 14, 924, 5, accent);
    vita2d_draw_rectangle(18, 14, 4, 516, accent);
    vita2d_draw_rectangle(36, 32, 888, 1, COLOR_HAIRLINE);
    vita2d_draw_rectangle(36, 481, 888, 1, COLOR_HAIRLINE);

    /* Print registration ticks and a restrained halftone block in the gutter. */
    for (int i = 0; i < 5; ++i) {
        vita2d_draw_rectangle(923, 177 + i * 7, 3, 3, COLOR_HAIRLINE);
        vita2d_draw_rectangle(923, 216 + i * 7, 3, 3, COLOR_HAIRLINE);
    }
}

static void draw_magazine_header(vita2d_pgf *font, const char *section,
    const char *headline, const char *strapline, const char *issue,
    unsigned int accent)
{
    draw_magazine_canvas(accent);
    ui_draw_text_with_font(font, 48, 55, COLOR_PAPER, 0.62f, "VITA NEXUS");
    ui_draw_text_with_font(font, 168, 55, COLOR_MUTED, 0.48f, "PLAYER'S FIELD MAGAZINE");
    ui_draw_fit_text(675, 55, accent, 0.48f, 185, issue);
    draw_magazine_barcode(868, 36, accent);

    ui_draw_text_with_font(font, 48, 91, accent, 0.54f, section);
    ui_draw_fit_text(625, 91, COLOR_MUTED, 0.48f, 285, strapline);
    ui_draw_fit_text(48, 133, COLOR_ACID, 1.02f, 862, headline);
    vita2d_draw_rectangle(48, 148, 864, 2, accent);
}

static void draw_magazine_footer(vita2d_pgf *font, const char *controls,
    const char *folio, unsigned int accent)
{
    vita2d_draw_rectangle(48, 495, 864, 2, accent);
    ui_draw_fit_text(54, 519, COLOR_MUTED, 0.43f, 790, controls);
    vita2d_draw_rectangle(864, 503, 48, 25, accent);
    ui_draw_fit_text(869, 521, COLOR_BG, 0.52f, 38, folio);
}

static void draw_magazine_panel(int x, int y, int width, int height,
    unsigned int accent)
{
    vita2d_draw_rectangle(x, y, width, height, COLOR_PANEL);
    vita2d_draw_rectangle(x, y, width, 4, accent);
    vita2d_draw_rectangle(x, y + height - 1, width, 1, COLOR_HAIRLINE);
}

static void draw_magazine_kicker(vita2d_pgf *font, int x, int y,
    const char *label, unsigned int accent)
{
    vita2d_draw_rectangle(x, y + 2, 4, 13, accent);
    ui_draw_text_with_font(font, x + 11, y + 1, accent, 0.50f, label);
}

static void draw_magazine_chip(vita2d_pgf *font, int x, int y, int width,
    const char *label, unsigned int accent)
{
    vita2d_draw_rectangle(x, y, width, 23, COLOR_CARD);
    vita2d_draw_rectangle(x, y, 3, 23, accent);
    ui_draw_fit_text(x + 10, y + 16, COLOR_PAPER, 0.48f, width - 17, label);
}

static void draw_magazine_progress(vita2d_pgf *font, int x, int y, int width,
    const char *label, size_t value, size_t total, unsigned int accent)
{
    char count[48];
    int fill = 0;
    (void)snprintf(count, sizeof(count), "%lu / %lu",
        (unsigned long)value, (unsigned long)total);
    ui_draw_text_with_font(font, x, y, COLOR_PAPER, 0.48f, label);
    ui_draw_fit_text(x + width - 76, y, COLOR_MUTED, 0.44f, 76, count);
    vita2d_draw_rectangle(x, y + 9, width, 5, COLOR_CARD);
    if (total > 0) {
        fill = (int)((value * (size_t)width) / total);
        if (fill > width) fill = width;
    }
    if (fill > 0) vita2d_draw_rectangle(x, y + 9, fill, 5, accent);
}

static const VitaNexusCatalogEntry *find_catalog_entry(const char *game_id)
{
    if (!game_id || !game_id[0]) return NULL;
    for (size_t i = 0; i < vita_nexus_catalog_count; ++i) {
        if (strcmp(vita_nexus_catalog[i].game_id, game_id) == 0)
            return &vita_nexus_catalog[i];
    }
    return NULL;
}

static void draw_home(vita2d_pgf *font, int selected)
{
    static const char *folios[] = {"06", "04", "11", "08", "12"};
    const int menu_x = 48, menu_y = 176, row_h = 55;
    if (selected < 0 || selected >= (int)(sizeof(menu_items) / sizeof(menu_items[0])))
        selected = 0;

    draw_magazine_header(font, "EDITOR'S DESK", "THE BACKLOG NEVER SLEEPS",
        "VOL. 01  /  CONSOLE EDITION", "ISSUE 010", COLOR_PINK);

    draw_magazine_kicker(font, 48, 168, "IN THIS ISSUE", COLOR_CYAN);
    for (int i = 0; i < 5; ++i) {
        const int y = menu_y + (i * row_h);
        const bool active = i == selected;
        draw_magazine_panel(menu_x, y, 280, 46, active ? COLOR_PINK : COLOR_HAIRLINE);
        if (active) vita2d_draw_rectangle(menu_x, y, 280, 46, COLOR_CARD);
        ui_draw_textf_with_font(font, menu_x + 14, y + 30,
            active ? COLOR_ACID : COLOR_MUTED, 0.68f, "%02d", i + 1);
        ui_draw_fit_text(menu_x + 58, y + 29,
            active ? COLOR_PAPER : COLOR_MUTED, 0.76f, 175, menu_items[i].label);
        ui_draw_text_with_font(font, menu_x + 238, y + 29,
            active ? COLOR_CYAN : COLOR_HAIRLINE, 0.45f, folios[i]);
        if (active)
            vita2d_draw_rectangle(menu_x + 58, y + 36,
                50 + (int)vita_nexus_ui_motion_pulse(&g_ui_motion, 22, 48), 2, COLOR_CYAN);
    }

    draw_magazine_panel(348, 168, 564, 311, COLOR_CYAN);
    draw_magazine_kicker(font, 372, 190, "COVER STORY  /  SELECTED SECTION", COLOR_PINK);
    ui_draw_textf_with_font(font, 372, 246, COLOR_PAPER, 1.45f,
        "%02d", selected + 1);
    ui_draw_fit_text(450, 245, COLOR_ACID, 1.12f, 425, menu_items[selected].label);
    ui_draw_fit_text(374, 278, COLOR_CYAN, 0.62f, 510, menu_items[selected].status);
    draw_wrapped(font, 374, 315, COLOR_PAPER, 0.60f,
        menu_items[selected].description, 505, 3);
    ui_draw_text_with_font(font, 374, 379, COLOR_MUTED, 0.46f,
        "A FIELD NOTE FROM THE VITA NEXUS EDITORIAL DESK");
    draw_magazine_barcode(822, 365, COLOR_PINK);

    draw_magazine_panel(370, 400, 164, 62, COLOR_PINK);
    draw_magazine_panel(546, 400, 164, 62, COLOR_ACID);
    draw_magazine_panel(722, 400, 166, 62, COLOR_CYAN);
    ui_draw_text_with_font(font, 383, 425, COLOR_MUTED, 0.43f, "CATALOG");
    ui_draw_textf_with_font(font, 383, 450, COLOR_PAPER, 0.72f, "%lu GAMES",
        (unsigned long)vita_nexus_catalog_count);
    ui_draw_text_with_font(font, 559, 425, COLOR_MUTED, 0.43f, "FORMATS");
    ui_draw_text_with_font(font, 559, 450, COLOR_PAPER, 0.72f, "VITA / PSP / PS1");
    ui_draw_text_with_font(font, 735, 425, COLOR_MUTED, 0.43f, "PLAYER SLOTS");
    ui_draw_text_with_font(font, 735, 450, COLOR_PAPER, 0.72f, "03 / LOCAL");

    draw_magazine_footer(font, "D-PAD  TURN THE PAGE     CROSS  OPEN SECTION     START  EXIT",
        "01", COLOR_PINK);
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

static void draw_discover(vita2d_pgf *font, size_t selected,
    unsigned int platform_filter, int letter_filter)
{
    const size_t start = selected >= CATALOG_ROWS ? selected - CATALOG_ROWS + 1 : 0;
    char filter_note[96];

    draw_magazine_header(font, "THE INDEX  /  DISCOVER", "FIND YOUR NEXT OBSESSION",
        "9,732 FILES  /  FOUR PLAYSTATION WORLDS", "INDEX / 04", COLOR_CYAN);
    draw_magazine_chip(font, 48, 165, 255, platform_filters[platform_filter], COLOR_CYAN);
    (void)snprintf(filter_note, sizeof(filter_note), "INITIAL  %s",
        letter_filter < 0 ? "ALL" : (char[2]){'A' + letter_filter, '\0'});
    draw_magazine_chip(font, 315, 165, 170, filter_note, COLOR_PINK);
    draw_magazine_chip(font, 495, 165, 190, "SQUARE  /  RESET FILTERS", COLOR_ACID);

    draw_magazine_panel(48, 199, 640, 280, COLOR_CYAN);
    draw_magazine_panel(704, 199, 208, 280, COLOR_PINK);
    draw_magazine_kicker(font, 68, 218, "THE GAME REGISTER", COLOR_CYAN);
    ui_draw_text_with_font(font, 535, 220, COLOR_MUTED, 0.43f, "PLATFORM");
    vita2d_draw_rectangle(64, 236, 608, 1, COLOR_HAIRLINE);

    if (visible_count == 0) {
        ui_draw_text_with_font(font, 82, 300, COLOR_PAPER, 0.88f, "NO MATCHES IN THIS EDITION");
        ui_draw_text_with_font(font, 82, 330, COLOR_MUTED, 0.55f, "Clear a filter and try the index again.");
    }

    for (size_t row = 0; row < CATALOG_ROWS && start + row < visible_count; ++row) {
        const size_t index = visible_games[start + row];
        const VitaNexusCatalogEntry *entry = &vita_nexus_catalog[index];
        const int y = 263 + (int)row * 34;
        const bool active = start + row == selected;
        if (active) {
            vita2d_draw_rectangle(64, y - 22, 608, 30, COLOR_CARD);
            vita2d_draw_rectangle(64, y - 22, 4, 30, COLOR_PINK);
            vita2d_draw_rectangle(79, y + 5,
                46 + (int)vita_nexus_ui_motion_pulse(&g_ui_motion, 20, 48), 2, COLOR_CYAN);
        } else {
            vita2d_draw_rectangle(78, y + 8, 594, 1, COLOR_HAIRLINE);
        }
        ui_draw_textf_with_font(font, 82, y, COLOR_MUTED, 0.46f, "%02lu",
            (unsigned long)(start + row + 1));
        ui_draw_fit_text(122, y, active ? COLOR_ACID : COLOR_PAPER,
            0.66f, 405, entry->title);
        ui_draw_fit_text(538, y, COLOR_CYAN, 0.47f, 120,
            platform_label(entry->platform_mask));
    }

    draw_magazine_kicker(font, 724, 218, "INDEX NOTES", COLOR_PINK);
    ui_draw_text_with_font(font, 727, 269, COLOR_MUTED, 0.44f, "MATCHES");
    ui_draw_textf_with_font(font, 724, 311, COLOR_ACID, 1.16f, "%lu",
        (unsigned long)visible_count);
    vita2d_draw_rectangle(724, 324, 166, 1, COLOR_HAIRLINE);
    ui_draw_text_with_font(font, 727, 350, COLOR_MUTED, 0.44f, "ALL CATALOG FILES");
    ui_draw_textf_with_font(font, 727, 378, COLOR_PAPER, 0.70f, "%lu",
        (unsigned long)vita_nexus_catalog_count);
    ui_draw_text_with_font(font, 727, 408, COLOR_MUTED, 0.44f, "CURRENT EDITION");
    ui_draw_fit_text(727, 434, COLOR_CYAN, 0.53f, 164,
        platform_filters[platform_filter]);
    ui_draw_text_with_font(font, 727, 459, COLOR_PINK, 0.44f,
        "CROSS / OPEN GAME FILE");

    if (selected < visible_count) {
        const VitaNexusCatalogEntry *entry = &vita_nexus_catalog[visible_games[selected]];
        ui_draw_fit_text(54, 474, COLOR_MUTED, 0.40f, 620, entry->game_id);
    }
    draw_magazine_footer(font, "UP / DOWN  BROWSE     L / R  INITIAL     TRIANGLE  PLATFORM     CROSS  GAME FILE",
        "04", COLOR_CYAN);
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
    VitaNexusProfileStats stats;
    const size_t start = selected >= CATALOG_ROWS ? selected - CATALOG_ROWS + 1 : 0;
    vita_nexus_profile_get_stats(library, &stats);

    draw_magazine_header(font, "PLAYER'S LOG  /  LIBRARY", "YOUR COLLECTION, IN PRINT",
        "PRIVATE NOTES  /  PLAYER PROFILE", "LOG / 06", COLOR_PINK);

    const size_t metric_values[] = {stats.total, stats.favorites, stats.backlog, stats.completed};
    const char *metric_labels[] = {"TRACKED TITLES", "FAVORITES", "BACKLOG", "COMPLETED"};
    const unsigned int metric_colors[] = {COLOR_CYAN, COLOR_PINK, COLOR_ACID, COLOR_CYAN};
    for (int i = 0; i < 4; ++i) {
        const int x = 48 + i * 219;
        draw_magazine_panel(x, 165, 207, 62, metric_colors[i]);
        ui_draw_text_with_font(font, x + 13, 188, COLOR_MUTED, 0.43f, metric_labels[i]);
        ui_draw_textf_with_font(font, x + 13, 216, COLOR_PAPER, 0.82f, "%lu",
            (unsigned long)metric_values[i]);
    }

    draw_magazine_panel(48, 242, 610, 237, COLOR_CYAN);
    draw_magazine_panel(674, 242, 238, 237, COLOR_PINK);
    draw_magazine_kicker(font, 68, 261, "THE PERSONAL REGISTER", COLOR_CYAN);
    ui_draw_fit_text(480, 263, COLOR_MUTED, 0.44f, 158, library_filter_labels[filter]);
    vita2d_draw_rectangle(64, 280, 578, 1, COLOR_HAIRLINE);

    if (library_visible_count == 0) {
        ui_draw_text_with_font(font, 82, 337, COLOR_PAPER, 0.82f, "NOTHING ON FILE. YET.");
        ui_draw_text_with_font(font, 82, 367, COLOR_MUTED, 0.53f,
            "Open a game in DISCOVER and add it to your personal log.");
    }

    for (size_t row = 0; row < CATALOG_ROWS && start + row < library_visible_count; ++row) {
        const VitaNexusLibraryRecord *record =
            &library->records[library_visible[start + row]];
        const VitaNexusCatalogEntry *entry = find_catalog_entry(record->game_id);
        const int y = 309 + (int)row * 27;
        const bool active = start + row == selected;
        const char *title = entry ? entry->title : record->game_id;
        char status[72] = "";
        if (record->flags & VITA_NEXUS_LIBRARY_FAVORITE)
            (void)strncat(status, "FAV  ", sizeof(status) - strlen(status) - 1);
        if (record->flags & VITA_NEXUS_LIBRARY_BACKLOG)
            (void)strncat(status, "BACKLOG  ", sizeof(status) - strlen(status) - 1);
        if (record->flags & VITA_NEXUS_LIBRARY_COMPLETED)
            (void)strncat(status, "DONE", sizeof(status) - strlen(status) - 1);
        if (active) {
            vita2d_draw_rectangle(64, y - 18, 578, 24, COLOR_CARD);
            vita2d_draw_rectangle(64, y - 18, 4, 24, COLOR_PINK);
        }
        ui_draw_textf_with_font(font, 78, y, COLOR_MUTED, 0.43f, "%02lu",
            (unsigned long)(start + row + 1));
        ui_draw_fit_text(115, y, active ? COLOR_ACID : COLOR_PAPER,
            0.58f, 285, title);
        ui_draw_fit_text(410, y, COLOR_CYAN, 0.41f, 166,
            status[0] ? status : "UNFILED");
        ui_draw_textf_with_font(font, 594, y, COLOR_PINK, 0.48f,
            "%u/10", (unsigned int)record->rating);
        if (!active) vita2d_draw_rectangle(78, y + 7, 560, 1, COLOR_HAIRLINE);
    }

    draw_magazine_kicker(font, 694, 261, "COLLECTION REPORT", COLOR_PINK);
    ui_draw_textf_with_font(font, 694, 302, COLOR_PAPER, 0.78f,
        "PLAYER %02u", active_profile + 1);
    draw_magazine_progress(font, 694, 332, 198, "FAVORITES",
        stats.favorites, stats.total, COLOR_PINK);
    draw_magazine_progress(font, 694, 367, 198, "BACKLOG",
        stats.backlog, stats.total, COLOR_ACID);
    draw_magazine_progress(font, 694, 402, 198, "COMPLETED",
        stats.completed, stats.total, COLOR_CYAN);
    ui_draw_textf_with_font(font, 694, 454, COLOR_MUTED, 0.42f,
        "%lu RATED ENTRIES", (unsigned long)stats.rated);

    if (active_profile_read_only) {
        ui_draw_fit_text(54, 473, COLOR_PINK, 0.43f, 580,
            "DATA ERROR  /  RESTORE A VALID BACKUP IN PROFILES");
    } else if (remove_confirm) {
        ui_draw_fit_text(54, 473, COLOR_PINK, 0.46f, 580,
            "REMOVE THIS ENTRY?  CROSS CONFIRM  /  CIRCLE CANCEL");
    }
    draw_magazine_footer(font,
        "UP / DOWN  SELECT     CROSS  OPEN FILE     L / R  FILTER     SQUARE  REMOVE",
        "06", COLOR_PINK);
}

static void draw_profiles(vita2d_pgf *font, size_t selected,
    bool import_confirm, const char *notice)
{
    draw_magazine_header(font, "PLAYER DOSSIERS  /  BACKUP BAY",
        "THREE PLAYERS. THREE COLLECTIONS.",
        "LOCAL PROFILES  /  PORTABLE BACKUPS", "DOSSIER / 08", COLOR_CYAN);

    for (unsigned int i = 0; i < VITA_NEXUS_PROFILE_COUNT; ++i) {
        VitaNexusLibrary snapshot;
        VitaNexusProfileStats stats;
        const VitaNexusStoreResult result = read_profile(i, &snapshot);
        vita_nexus_profile_get_stats(&snapshot, &stats);
        const int x = 48 + (int)i * 292;
        const int y = 169;
        const bool active = i == active_profile;
        const bool focus = i == selected;
        const unsigned int accent = focus ? COLOR_PINK : (i == 1 ? COLOR_ACID : COLOR_CYAN);

        draw_magazine_panel(x, y, 280, 281, accent);
        draw_magazine_kicker(font, x + 18, y + 19,
            focus ? "CURRENT SELECTION" : "PLAYER DOSSIER", accent);
        ui_draw_textf_with_font(font, x + 18, y + 91, COLOR_PAPER, 1.68f,
            "%02u", i + 1);
        ui_draw_textf_with_font(font, x + 108, y + 77, focus ? COLOR_ACID : COLOR_PAPER,
            0.78f, "PLAYER %02u", i + 1);
        if (active) {
            draw_magazine_chip(font, x + 108, y + 88, 126, "ACTIVE PROFILE", COLOR_CYAN);
        } else {
            draw_magazine_chip(font, x + 108, y + 88, 126, "LOCAL SLOT", COLOR_HAIRLINE);
        }

        vita2d_draw_rectangle(x + 18, y + 124, 244, 1, COLOR_HAIRLINE);
        if (result == VITA_NEXUS_STORE_INVALID || result == VITA_NEXUS_STORE_IO_ERROR) {
            ui_draw_fit_text(x + 18, y + 165, COLOR_PINK, 0.56f, 238,
                "DATA DAMAGED  /  RESTORE BACKUP");
        } else {
            ui_draw_text_with_font(font, x + 18, y + 153, COLOR_MUTED, 0.43f, "COLLECTION");
            ui_draw_textf_with_font(font, x + 18, y + 187, COLOR_PAPER, 0.88f,
                "%lu TITLES", (unsigned long)stats.total);
            ui_draw_textf_with_font(font, x + 18, y + 218, COLOR_CYAN, 0.49f,
                "%lu FAV   /   %lu BACKLOG",
                (unsigned long)stats.favorites, (unsigned long)stats.backlog);
            draw_magazine_progress(font, x + 18, y + 240, 244, "COMPLETED",
                stats.completed, stats.total, COLOR_PINK);
            ui_draw_textf_with_font(font, x + 18, y + 268, COLOR_MUTED, 0.43f,
                "%lu PERSONAL SCORES", (unsigned long)stats.rated);
        }
    }

    if (import_confirm) {
        draw_magazine_panel(48, 461, 864, 27, COLOR_PINK);
        ui_draw_fit_text(64, 481, COLOR_PAPER, 0.49f, 830,
            "RESTORE PLAYER BACKUP?  CROSS CONFIRM OVERWRITE  /  CIRCLE CANCEL");
    } else if (notice && notice[0]) {
        draw_magazine_panel(48, 461, 864, 27, COLOR_ACID);
        ui_draw_fit_text(64, 481, COLOR_PAPER, 0.48f, 830, notice);
    }

    draw_magazine_footer(font,
        "UP / DOWN  SELECT PLAYER     CROSS  ACTIVATE     TRIANGLE  EXPORT     SQUARE  RESTORE",
        "08", COLOR_CYAN);
}

static void draw_game_detail(vita2d_pgf *font, size_t selected)
{
    if (selected >= vita_nexus_catalog_count) return;
    const VitaNexusCatalogEntry *entry = &vita_nexus_catalog[selected];
    const uint8_t flags = library_flags(&g_library, entry->game_id);
    const uint8_t rating = library_rating(&g_library, entry->game_id);
    char personal_score[48];

    (void)snprintf(personal_score, sizeof(personal_score),
        rating ? "%u / 10" : "NOT RATED", (unsigned int)rating);
    draw_magazine_header(font, "GAME FILE  /  THE REVIEW DESK", entry->title,
        "EDITORIAL PROFILE  /  SOURCE-LED DATA", "FILE / 09", COLOR_PINK);

    draw_magazine_panel(48, 165, 230, 314, COLOR_PINK);
    draw_magazine_kicker(font, 66, 184, "COVER FILE", COLOR_PINK);
    ui_draw_text_with_font(font, 66, 229, COLOR_PAPER, 1.62f, "01");
    ui_draw_fit_text(66, 250, COLOR_ACID, 0.72f, 192, entry->title);
    vita2d_draw_rectangle(66, 286, 194, 2, COLOR_CYAN);
    draw_magazine_chip(font, 66, 300, 194, platform_label(entry->platform_mask), COLOR_CYAN);
    ui_draw_text_with_font(font, 66, 346, COLOR_MUTED, 0.43f, "RELEASE YEAR");
    if (entry->release_year)
        ui_draw_textf_with_font(font, 66, 372, COLOR_PAPER, 0.74f, "%u",
            (unsigned int)entry->release_year);
    else
        ui_draw_text_with_font(font, 66, 372, COLOR_MUTED, 0.53f, "UNKNOWN");

    ui_draw_text_with_font(font, 66, 402, COLOR_PINK, 0.45f, "EDITOR'S NOTE");
    draw_wrapped(font, 66, 420, COLOR_PAPER, 0.45f,
        entry->summary, 190, 2);
    vita2d_draw_rectangle(66, 456, 194, 1, COLOR_HAIRLINE);
    ui_draw_text_with_font(font, 66, 474, COLOR_MUTED, 0.42f, "YOUR SCORE");
    ui_draw_fit_text(169, 474, COLOR_ACID, 0.55f, 90, personal_score);

    draw_magazine_panel(294, 165, 300, 139, COLOR_CYAN);
    draw_magazine_panel(608, 165, 304, 139, COLOR_ACID);
    draw_magazine_kicker(font, 313, 185, "RELEASE DESK", COLOR_CYAN);
    ui_draw_text_with_font(font, 313, 226, COLOR_MUTED, 0.45f, "PLATFORMS");
    ui_draw_fit_text(313, 250, COLOR_PAPER, 0.63f, 260,
        platform_label(entry->platform_mask));
    ui_draw_text_with_font(font, 313, 279, COLOR_MUTED, 0.43f, "RELEASE RECORDS");
    ui_draw_textf_with_font(font, 476, 279, COLOR_PAPER, 0.57f, "%u",
        (unsigned int)entry->release_count);

    draw_magazine_kicker(font, 627, 185, "AGE / REVIEW DESK", COLOR_PINK);
    ui_draw_text_with_font(font, 627, 220, COLOR_MUTED, 0.43f, "AGE GUIDANCE");
    ui_draw_fit_text(627, 245, COLOR_PAPER, 0.59f, 263,
        entry->age_guidance[0] ? entry->age_guidance : "NOT LISTED IN SOURCE DATA");
    ui_draw_text_with_font(font, 627, 271, COLOR_MUTED, 0.43f, "EXTERNAL SCORE");
    ui_draw_fit_text(627, 293, entry->external_score[0] ? COLOR_ACID : COLOR_MUTED,
        0.48f, 263, entry->external_score[0] ? entry->external_score : "NO SCORE ON FILE");

    draw_magazine_panel(294, 316, 618, 91, COLOR_PINK);
    draw_magazine_kicker(font, 313, 335, "CREDITS / PRODUCTION NOTES", COLOR_PINK);
    ui_draw_text_with_font(font, 313, 366, COLOR_MUTED, 0.43f, "DEVELOPER");
    ui_draw_fit_text(313, 389, COLOR_PAPER, 0.55f, 264,
        entry->developer[0] ? entry->developer : "NOT AVAILABLE");
    ui_draw_text_with_font(font, 615, 366, COLOR_MUTED, 0.43f, "PUBLISHER");
    ui_draw_fit_text(615, 389, COLOR_PAPER, 0.55f, 280,
        entry->publisher[0] ? entry->publisher : "NOT AVAILABLE");

    draw_magazine_panel(294, 419, 618, 60, COLOR_CYAN);
    ui_draw_text_with_font(font, 313, 443, COLOR_CYAN, 0.43f, "GENRES");
    ui_draw_fit_text(390, 443, COLOR_PAPER, 0.47f, 500,
        entry->genres[0] ? entry->genres : "NOT CLASSIFIED");
    ui_draw_text_with_font(font, 313, 467, COLOR_PINK, 0.43f, "TAGS");
    ui_draw_fit_text(390, 467, COLOR_PAPER, 0.45f, 500,
        entry->editorial_tags[0] ? entry->editorial_tags : "NO EDITORIAL TAGS");

    draw_magazine_footer(font,
        "L  FAVORITE     R  BACKLOG     TRIANGLE  COMPLETED     LEFT / RIGHT  PERSONAL SCORE     CIRCLE  BACK",
        "09", COLOR_PINK);
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
    vita_nexus_ui_motion_init(&g_ui_motion);
    vita_nexus_ui_motion_begin_transition(&g_ui_motion);
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

    VitaNexusScreenId previous_screen = VITA_NEXUS_SCREEN_HOME;
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

        const VitaNexusScreenId current_screen = vita_nexus_ui_screen_id(
            in_detail, in_profiles, in_library, in_discover);
        if (current_screen != previous_screen) {
            vita_nexus_ui_motion_begin_transition(&g_ui_motion);
            previous_screen = current_screen;
        }
        const uint8_t fade_alpha = vita_nexus_ui_motion_tick(&g_ui_motion);

        vita2d_start_drawing();
        vita2d_clear_screen();
        if (in_detail) draw_game_detail(font, detail_catalog_index);
        else if (in_profiles) draw_profiles(font, profile_selected, profile_import_confirm, profile_notice);
        else if (in_library) draw_library(font, &g_library, library_selected, library_filter, library_remove_confirm);
        else if (in_discover) draw_discover(font, catalog_selected, platform_filter, letter_filter);
        else draw_home(font, (int)menu_state.selected);
        if (fade_alpha > 0) {
            vita2d_draw_rectangle(0, 0, 960, 544, RGBA8(8, 6, 13, fade_alpha));
        }
        vita2d_end_drawing();
        vita2d_swap_buffers();
    }

    vita2d_fini();
    if (ui_font_medium) vita2d_free_font(ui_font_medium);
    if (ui_font_bold) vita2d_free_font(ui_font_bold);
    if (font) vita2d_free_pgf(font);
    sceKernelExitProcess(0);
    return 0;
}
