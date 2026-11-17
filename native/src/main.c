#include <psp2/ctrl.h>
#include <psp2/kernel/processmgr.h>
#include <vita2d.h>

#include <stdbool.h>
#include <stdint.h>

#define COLOR_BG       RGBA8(11, 9, 16, 255)
#define COLOR_PANEL    RGBA8(24, 17, 29, 255)
#define COLOR_PINK     RGBA8(255, 61, 168, 255)
#define COLOR_ACID     RGBA8(234, 255, 59, 255)
#define COLOR_CYAN     RGBA8(44, 232, 242, 255)
#define COLOR_PAPER    RGBA8(247, 241, 232, 255)
#define COLOR_MUTED    RGBA8(174, 160, 180, 255)

typedef struct {
    const char *label;
    const char *status;
    const char *description;
} MenuItem;

static const MenuItem menu_items[] = {
    {"LIBRARY", "NOT WIRED YET", "Installed titles and local collection."},
    {"DISCOVER", "CATALOG CONTRACT IN PROGRESS", "Browse Vita, PSP, PS1 and homebrew."},
    {"RANKINGS", "PLANNED", "Personal scores, lists and public chart snapshots."},
    {"SETTINGS", "PLANNED", "Profiles, storage and offline preferences."}
};

static void draw_menu(vita2d_pgf *font, int selected)
{
    const int menu_x = 54;
    const int menu_y = 190;
    const int row_h = 58;

    vita2d_draw_rectangle(0, 0, 960, 544, COLOR_BG);
    vita2d_draw_rectangle(0, 0, 960, 8, COLOR_PINK);
    vita2d_draw_rectangle(40, 38, 880, 2, COLOR_PAPER);
    vita2d_pgf_draw_text(font, 52, 76, COLOR_ACID, 1.25f, "VITA NEXUS");
    vita2d_pgf_draw_text(font, 53, 101, COLOR_CYAN, 0.72f, "THE BACKLOG NEVER SLEEPS");
    vita2d_pgf_draw_text(font, 760, 72, COLOR_PINK, 0.65f, "NATIVE / 00.01");

    vita2d_draw_rectangle(48, 132, 864, 1, COLOR_PINK);
    vita2d_pgf_draw_text(font, 54, 163, COLOR_MUTED, 0.68f, "01  /  SELECT A MODULE");

    for (int i = 0; i < 4; ++i) {
        const int y = menu_y + (i * row_h);
        const bool active = i == selected;
        const unsigned int accent = active ? COLOR_ACID : COLOR_MUTED;

        if (active) {
            vita2d_draw_rectangle(menu_x - 8, y - 22, 388, 44, COLOR_PANEL);
            vita2d_draw_rectangle(menu_x - 8, y - 22, 5, 44, COLOR_PINK);
        }
        vita2d_pgf_draw_textf(font, menu_x + 10, y + 5, accent, 0.9f, "%02d  %s", i + 1, menu_items[i].label);
    }

    vita2d_draw_rectangle(490, 184, 420, 228, COLOR_PANEL);
    vita2d_draw_rectangle(490, 184, 420, 4, COLOR_CYAN);
    vita2d_pgf_draw_text(font, 516, 220, COLOR_PINK, 0.72f, "MODULE STATUS");
    vita2d_pgf_draw_text(font, 516, 264, COLOR_PAPER, 0.9f, menu_items[selected].label);
    vita2d_pgf_draw_text(font, 516, 294, COLOR_ACID, 0.62f, menu_items[selected].status);
    vita2d_pgf_draw_text(font, 516, 344, COLOR_MUTED, 0.58f, menu_items[selected].description);

    vita2d_draw_rectangle(48, 448, 864, 1, COLOR_PAPER);
    vita2d_pgf_draw_text(font, 54, 484, COLOR_CYAN, 0.62f, "D-PAD  SELECT     X  OPEN     START  EXIT");
    vita2d_pgf_draw_text(font, 54, 516, COLOR_MUTED, 0.52f, "FOUNDATION BUILD  /  OFFLINE-FIRST  /  NO SYSTEM FILES MODIFIED");
}

int main(void)
{
    SceCtrlData pad;
    uint32_t previous_buttons = 0;
    int selected = 0;
    bool running = true;

    sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);

    if (vita2d_init() < 0) {
        sceKernelExitProcess(1);
        return 1;
    }

    vita2d_set_clear_color(COLOR_BG);
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

        if (pressed & SCE_CTRL_UP) {
            selected = (selected + 3) % 4;
        } else if (pressed & SCE_CTRL_DOWN) {
            selected = (selected + 1) % 4;
        }

        if (pressed & SCE_CTRL_START) {
            running = false;
        }

        vita2d_start_drawing();
        vita2d_clear_screen();
        draw_menu(font, selected);
        vita2d_end_drawing();
        vita2d_swap_buffers();
    }

    vita2d_free_pgf(font);
    vita2d_fini();
    sceKernelExitProcess(0);
    return 0;
}
