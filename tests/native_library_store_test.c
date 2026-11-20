#include "library_store.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static uint32_t test_checksum_update(uint32_t hash, const char *data, size_t length)
{
    for (size_t i = 0; i < length; ++i) {
        hash ^= (uint8_t)data[i];
        hash *= 16777619u;
    }
    return hash;
}

static void write_duplicate_records(const char *path)
{
    const char *line = "duplicate-id|1|0\n";
    uint32_t hash = 2166136261u;
    hash = test_checksum_update(hash, line, strlen(line));
    hash = test_checksum_update(hash, line, strlen(line));
    FILE *file = fopen(path, "wb");
    assert(file != NULL);
    fputs("VNXLIB|1\n", file);
    fputs(line, file);
    fputs(line, file);
    fprintf(file, "END|%08X\n", (unsigned int)hash);
    assert(fclose(file) == 0);
}

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

    FILE *wrong_version = fopen(bad_path, "wb");
    assert(wrong_version != NULL);
    fputs("VNXLIB|2\nEND|811C9DC5\n", wrong_version);
    assert(fclose(wrong_version) == 0);
    assert(vita_nexus_library_load(&loaded, bad_path) == VITA_NEXUS_STORE_INVALID);
    assert(loaded.count == 2 && vita_nexus_library_find(&loaded, "future-catalog-id") != NULL);

    write_duplicate_records(bad_path);
    assert(vita_nexus_library_load(&loaded, bad_path) == VITA_NEXUS_STORE_INVALID);
    assert(loaded.count == 2 && vita_nexus_library_find(&loaded, "future-catalog-id") != NULL);

    FILE *truncated = fopen(bad_path, "wb");
    assert(truncated != NULL);
    fputs("VNXLIB|1\ntruncated-record|1|4", truncated);
    assert(fclose(truncated) == 0);
    assert(vita_nexus_library_load(&loaded, bad_path) == VITA_NEXUS_STORE_INVALID);
    assert(loaded.count == 2 && vita_nexus_library_find(&loaded, "future-catalog-id") != NULL);

    VitaNexusLibrary invalid_count = loaded;
    invalid_count.count = VITA_NEXUS_LIBRARY_MAX_RECORDS + 1;
    assert(vita_nexus_library_find(&invalid_count, "future-catalog-id") == NULL);
    assert(vita_nexus_library_set(&invalid_count, "new-entry", VITA_NEXUS_LIBRARY_FAVORITE, 0) == VITA_NEXUS_STORE_INVALID);
    assert(invalid_count.count == VITA_NEXUS_LIBRARY_MAX_RECORDS + 1);
    assert(vita_nexus_library_save(&invalid_count, path) == VITA_NEXUS_STORE_INVALID);

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
    fputs("VNXLIB|1\nempty-number||3\nEND|00000000\n", empty_number);
    fclose(empty_number);
    assert(vita_nexus_library_load(&loaded, bad_path) == VITA_NEXUS_STORE_INVALID);

    remove(path);
    remove(bad_path);
    return 0;
}
