#include "library_store.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STORE_HEADER "VNXLIB|1\n"
#define STORE_LINE_CAP 96

static uint32_t checksum_update(uint32_t hash, const char *data, size_t length)
{
    for (size_t i = 0; i < length; ++i) {
        hash ^= (uint8_t)data[i];
        hash *= 16777619u;
    }
    return hash;
}

static int valid_id(const char *game_id)
{
    size_t length;
    if (!game_id) return 0;
    length = strlen(game_id);
    if (length == 0 || length >= VITA_NEXUS_LIBRARY_ID_CAP) return 0;
    for (size_t i = 0; i < length; ++i) {
        const unsigned char c = (unsigned char)game_id[i];
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.')) return 0;
    }
    return 1;
}

void vita_nexus_library_init(VitaNexusLibrary *library)
{
    if (library) memset(library, 0, sizeof(*library));
}

const VitaNexusLibraryRecord *vita_nexus_library_find(const VitaNexusLibrary *library, const char *game_id)
{
    if (!library || !valid_id(game_id)) return NULL;
    for (size_t i = 0; i < library->count; ++i)
        if (strcmp(library->records[i].game_id, game_id) == 0) return &library->records[i];
    return NULL;
}

VitaNexusStoreResult vita_nexus_library_set(VitaNexusLibrary *library, const char *game_id,
    uint8_t flags, uint8_t rating)
{
    if (!library || !valid_id(game_id) || (flags & (uint8_t)~7u) || rating > 10)
        return VITA_NEXUS_STORE_INVALID;
    for (size_t i = 0; i < library->count; ++i) {
        if (strcmp(library->records[i].game_id, game_id) == 0) {
            if (flags == 0 && rating == 0) {
                library->records[i] = library->records[--library->count];
            } else {
                library->records[i].flags = flags;
                library->records[i].rating = rating;
            }
            return VITA_NEXUS_STORE_OK;
        }
    }
    if (flags == 0 && rating == 0) return VITA_NEXUS_STORE_OK;
    if (library->count >= VITA_NEXUS_LIBRARY_MAX_RECORDS) return VITA_NEXUS_STORE_FULL;
    VitaNexusLibraryRecord *record = &library->records[library->count++];
    memset(record, 0, sizeof(*record));
    memcpy(record->game_id, game_id, strlen(game_id) + 1);
    record->flags = flags;
    record->rating = rating;
    return VITA_NEXUS_STORE_OK;
}

VitaNexusStoreResult vita_nexus_library_load(VitaNexusLibrary *library, const char *path)
{
    FILE *file;
    VitaNexusLibrary loaded;
    char line[STORE_LINE_CAP];
    uint32_t hash = 2166136261u;
    int found_end = 0;

    if (!library || !path) return VITA_NEXUS_STORE_INVALID;
    file = fopen(path, "rb");
    if (!file) return VITA_NEXUS_STORE_NOT_FOUND;
    vita_nexus_library_init(&loaded);

    if (!fgets(line, sizeof(line), file) || strcmp(line, STORE_HEADER) != 0) {
        fclose(file);
        return VITA_NEXUS_STORE_INVALID;
    }

    while (fgets(line, sizeof(line), file)) {
        size_t length = strlen(line);
        if (length == 0 || line[length - 1] != '\n') {
            fclose(file);
            return VITA_NEXUS_STORE_INVALID;
        }
        if (strncmp(line, "END|", 4) == 0) {
            unsigned int expected = 0;
            char extra = '\0';
            if (sscanf(line, "END|%8x\n%c", &expected, &extra) != 1 || expected != hash) {
                fclose(file);
                return VITA_NEXUS_STORE_INVALID;
            }
            found_end = 1;
            break;
        }
        if (loaded.count >= VITA_NEXUS_LIBRARY_MAX_RECORDS) {
            fclose(file);
            return VITA_NEXUS_STORE_INVALID;
        }
        hash = checksum_update(hash, line, length);
        line[length - 1] = '\0';
        char *first = strchr(line, '|');
        if (!first) { fclose(file); return VITA_NEXUS_STORE_INVALID; }
        *first++ = '\0';
        char *second = strchr(first, '|');
        if (!second) { fclose(file); return VITA_NEXUS_STORE_INVALID; }
        *second++ = '\0';
        if (!valid_id(line) || strchr(second, '|')) { fclose(file); return VITA_NEXUS_STORE_INVALID; }
        char *end = NULL;
        unsigned long flags = strtoul(first, &end, 10);
        if (!end || *end != '\0') { fclose(file); return VITA_NEXUS_STORE_INVALID; }
        unsigned long rating = strtoul(second, &end, 10);
        if (!end || *end != '\0' || flags > 7 || rating > 10) {
            fclose(file);
            return VITA_NEXUS_STORE_INVALID;
        }
        if (vita_nexus_library_find(&loaded, line)) {
            fclose(file);
            return VITA_NEXUS_STORE_INVALID;
        }
        if (flags == 0 && rating == 0) { fclose(file); return VITA_NEXUS_STORE_INVALID; }
        vita_nexus_library_set(&loaded, line, (uint8_t)flags, (uint8_t)rating);
    }
    if (!found_end || fgetc(file) != EOF || ferror(file)) {
        fclose(file);
        return VITA_NEXUS_STORE_INVALID;
    }
    fclose(file);
    *library = loaded;
    return VITA_NEXUS_STORE_OK;
}

VitaNexusStoreResult vita_nexus_library_save(const VitaNexusLibrary *library, const char *path)
{
    char temp_path[256];
    FILE *file;
    uint32_t hash = 2166136261u;
    if (!library || !path || library->count > VITA_NEXUS_LIBRARY_MAX_RECORDS ||
        strlen(path) + 5 >= sizeof(temp_path)) return VITA_NEXUS_STORE_INVALID;
    snprintf(temp_path, sizeof(temp_path), "%s.tmp", path);
    file = fopen(temp_path, "wb");
    if (!file) return VITA_NEXUS_STORE_IO_ERROR;
    if (fputs(STORE_HEADER, file) < 0) { fclose(file); remove(temp_path); return VITA_NEXUS_STORE_IO_ERROR; }
    for (size_t i = 0; i < library->count; ++i) {
        char line[STORE_LINE_CAP];
        const VitaNexusLibraryRecord *record = &library->records[i];
        if (!valid_id(record->game_id) || (record->flags & (uint8_t)~7u) || record->rating > 10 ||
            (record->flags == 0 && record->rating == 0)) {
            fclose(file); remove(temp_path); return VITA_NEXUS_STORE_INVALID;
        }
        for (size_t j = 0; j < i; ++j) {
            if (strcmp(library->records[j].game_id, record->game_id) == 0) {
                fclose(file); remove(temp_path); return VITA_NEXUS_STORE_INVALID;
            }
        }
        int length = snprintf(line, sizeof(line), "%s|%u|%u\n", record->game_id,
            (unsigned int)record->flags, (unsigned int)record->rating);
        if (length <= 0 || (size_t)length >= sizeof(line) ||
            fwrite(line, 1, (size_t)length, file) != (size_t)length) {
            fclose(file); remove(temp_path); return VITA_NEXUS_STORE_IO_ERROR;
        }
        hash = checksum_update(hash, line, (size_t)length);
    }
    if (fprintf(file, "END|%08X\n", (unsigned int)hash) < 0 || fflush(file) != 0) {
        fclose(file); remove(temp_path); return VITA_NEXUS_STORE_IO_ERROR;
    }
    if (fclose(file) != 0) { remove(temp_path); return VITA_NEXUS_STORE_IO_ERROR; }
    if (rename(temp_path, path) != 0) { remove(temp_path); return VITA_NEXUS_STORE_IO_ERROR; }
    return VITA_NEXUS_STORE_OK;
}
