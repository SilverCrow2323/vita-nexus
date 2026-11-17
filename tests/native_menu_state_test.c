#include "menu_state.h"

#include <assert.h>
#include <stddef.h>
#include <stdio.h>

int main(void)
{
    VitaNexusMenuState state = {0};

    vita_nexus_menu_move(&state, -1);
    assert(state.selected == 3);

    vita_nexus_menu_move(&state, 1);
    assert(state.selected == 0);

    vita_nexus_menu_move(&state, 1);
    assert(state.selected == 1);

    vita_nexus_menu_move(&state, 0);
    assert(state.selected == 1);

    state.selected = 99;
    vita_nexus_menu_move(&state, -1);
    assert(state.selected == 3);

    vita_nexus_menu_move(NULL, 1);
    puts("VITA NEXUS native menu state: PASS");
    return 0;
}
