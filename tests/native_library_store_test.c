#define _POSIX_C_SOURCE 200809L
#include "library_store.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

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


static void write_record_fixture(const char *path, const char *record_line, const char *checksum_suffix)
{
    uint32_t hash = 2166136261u;
    hash = test_checksum_update(hash, record_line, strlen(record_line));
    FILE *file = fopen(path, "wb");
    assert(file != NULL);
    fputs("VNXLIB|1\n", file);
    fputs(record_line, file);
    fprintf(file, "END|%08X%s\n", (unsigned int)hash,
        checksum_suffix ? checksum_suffix : "");
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
    {
        char sentinel_path[256], hostile_temp[256];
        FILE *sentinel;
        snprintf(sentinel_path, sizeof(sentinel_path), "%s-sentinel", path);
        snprintf(hostile_temp, sizeof(hostile_temp), "%s.tmp.%ld.0", path, (long)getpid());
        sentinel = fopen(sentinel_path, "wb");
        assert(sentinel != NULL);
        fputs("DO NOT TRUNCATE", sentinel);
        assert(fclose(sentinel) == 0);
        remove(hostile_temp);
        assert(symlink(sentinel_path, hostile_temp) == 0);
        assert(vita_nexus_library_save(&library, path) == VITA_NEXUS_STORE_OK);
        sentinel = fopen(sentinel_path, "rb");
        assert(sentinel != NULL);
        char sentinel_data[32] = {0};
        assert(fread(sentinel_data, 1, sizeof(sentinel_data) - 1, sentinel) == strlen("DO NOT TRUNCATE"));
        assert(fclose(sentinel) == 0);
        assert(strcmp(sentinel_data, "DO NOT TRUNCATE") == 0);
        remove(hostile_temp);
        remove(sentinel_path);
    }
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

    /* A fixed-size record without an in-bounds NUL must fail closed before
       lookup, mutation or persistence can reach strcmp/strlen. */
    VitaNexusLibrary malformed_record = loaded;
    memset(malformed_record.records[0].game_id, 'x',
        sizeof(malformed_record.records[0].game_id));
    assert(vita_nexus_library_find(&malformed_record, "future-catalog-id") == NULL);
    assert(vita_nexus_library_set(&malformed_record, "safe-id",
        VITA_NEXUS_LIBRARY_FAVORITE, 0) == VITA_NEXUS_STORE_INVALID);
    assert(malformed_record.count == loaded.count);
    assert(vita_nexus_library_save(&malformed_record, bad_path) == VITA_NEXUS_STORE_INVALID);

    FILE *wrong_version = fopen(bad_path, "wb");
    assert(wrong_version != NULL);
    fputs("VNXLIB|2\nEND|811C9DC5\n", wrong_version);
    assert(fclose(wrong_version) == 0);
    assert(vita_nexus_library_load(&loaded, bad_path) == VITA_NEXUS_STORE_INVALID);
    assert(loaded.count == 2 && vita_nexus_library_find(&loaded, "future-catalog-id") != NULL);

    write_duplicate_records(bad_path);
    assert(vita_nexus_library_load(&loaded, bad_path) == VITA_NEXUS_STORE_INVALID);
    assert(loaded.count == 2 && vita_nexus_library_find(&loaded, "future-catalog-id") != NULL);

    static const char *noncanonical_records[] = {
        "plus-flags|+1|4\n",
        "space-flags| 1|4\n",
        "minus-flags|-1|4\n",
        "leading-zero-flags|01|4\n",
        "plus-rating|1|+4\n",
        "space-rating|1| 4\n",
        "leading-zero-rating|1|04\n",
        "overflow-flags|999999999999999999999999|4\n",
        "overflow-rating|1|999999999999999999999999\n"
    };
    for (size_t i = 0; i < sizeof(noncanonical_records) / sizeof(noncanonical_records[0]); ++i) {
        write_record_fixture(bad_path, noncanonical_records[i], "");
        assert(vita_nexus_library_load(&loaded, bad_path) == VITA_NEXUS_STORE_INVALID);
        assert(loaded.count == 2);
        assert(vita_nexus_library_find(&loaded, "future-catalog-id") != NULL);
    }

    write_record_fixture(bad_path, "checksum-width|1|4\n", "0");
    assert(vita_nexus_library_load(&loaded, bad_path) == VITA_NEXUS_STORE_INVALID);
    assert(loaded.count == 2);
    assert(vita_nexus_library_find(&loaded, "future-catalog-id") != NULL);

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
