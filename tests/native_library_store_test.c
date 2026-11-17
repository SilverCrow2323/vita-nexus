#include "library_store.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    VitaNexusLibrary library, loaded;
    const char *path = "/tmp/vita-nexus-library-test.dat";
    const char *bad_path = "/tmp/vita-nexus-library-test-bad.dat";
    vita_nexus_library_init(&library);
    assert(vita_nexus_library_set(&library, "vita-game-001", VITA_NEXUS_LIBRARY_FAVORITE, 8) == VITA_NEXUS_STORE_OK);
    assert(vita_nexus_library_set(&library, "psp-game-002", VITA_NEXUS_LIBRARY_BACKLOG | VITA_NEXUS_LIBRARY_COMPLETED, 10) == VITA_NEXUS_STORE_OK);
    assert(vita_nexus_library_save(&library, path) == VITA_NEXUS_STORE_OK);
    vita_nexus_library_init(&loaded);
    assert(vita_nexus_library_load(&loaded, path) == VITA_NEXUS_STORE_OK);
    assert(loaded.count == 2);
    assert(vita_nexus_library_find(&loaded, "vita-game-001")->rating == 8);
    assert(vita_nexus_library_find(&loaded, "psp-game-002")->flags == 6);

    assert(vita_nexus_library_set(&loaded, "vita-game-001", 0, 0) == VITA_NEXUS_STORE_OK);
    assert(vita_nexus_library_find(&loaded, "vita-game-001") == NULL);
    assert(vita_nexus_library_set(&loaded, "future-catalog-id", VITA_NEXUS_LIBRARY_FAVORITE, 0) == VITA_NEXUS_STORE_OK);
    assert(vita_nexus_library_find(&loaded, "future-catalog-id") != NULL);
    assert(vita_nexus_library_set(&loaded, "bad/id", 1, 0) == VITA_NEXUS_STORE_INVALID);
    assert(vita_nexus_library_set(&loaded, "rating-overflow", 1, 11) == VITA_NEXUS_STORE_INVALID);

    FILE *bad = fopen(bad_path, "wb");
    assert(bad != NULL);
    fputs("VNXLIB|1\nvalid-id|1|4\nEND|00000000\n", bad);
    fclose(bad);
    VitaNexusLibrary before = loaded;
    assert(vita_nexus_library_load(&loaded, bad_path) == VITA_NEXUS_STORE_INVALID);
    assert(loaded.count == before.count);
    assert(vita_nexus_library_find(&loaded, "future-catalog-id") != NULL);

    FILE *empty_number = fopen(bad_path, "wb");
    assert(empty_number != NULL);
    fputs("VNXLIB|1\\nempty-number||3\\nEND|00000000\\n", empty_number);
    fclose(empty_number);
    assert(vita_nexus_library_load(&loaded, bad_path) == VITA_NEXUS_STORE_INVALID);

    remove(path);
    remove(bad_path);
    return 0;
}
