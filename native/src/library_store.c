#include "library_store.h"

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
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

static int parse_unsigned_decimal(const char *text, unsigned int maximum, unsigned int *value)
{
    unsigned int parsed = 0;
    size_t length;

    if (!text || !value) return 0;
    length = strlen(text);
    if (length == 0 || (length > 1 && text[0] == '0')) return 0;

    for (size_t i = 0; i < length; ++i) {
        const unsigned char c = (unsigned char)text[i];
        unsigned int digit;
        if (c < '0' || c > '9') return 0;
        digit = (unsigned int)(c - '0');
        if (digit > maximum || parsed > (maximum - digit) / 10u) return 0;
        parsed = parsed * 10u + digit;
    }

    *value = parsed;
    return 1;
}

static int parse_checksum_line(const char *line, uint32_t *checksum)
{
    uint32_t parsed = 0;
    if (!line || !checksum || strlen(line) != 13u ||
        memcmp(line, "END|", 4u) != 0 || line[12] != '\n') return 0;

    for (size_t i = 4; i < 12; ++i) {
        const unsigned char c = (unsigned char)line[i];
        unsigned int digit;
        if (c >= '0' && c <= '9') digit = (unsigned int)(c - '0');
        else if (c >= 'a' && c <= 'f') digit = (unsigned int)(c - 'a') + 10u;
        else if (c >= 'A' && c <= 'F') digit = (unsigned int)(c - 'A') + 10u;
        else return 0;
        parsed = (parsed << 4) | digit;
    }

    *checksum = parsed;
    return 1;
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
    if (!library || library->count > VITA_NEXUS_LIBRARY_MAX_RECORDS || !valid_id(game_id)) return NULL;
    for (size_t i = 0; i < library->count; ++i)
        if (strcmp(library->records[i].game_id, game_id) == 0) return &library->records[i];
    return NULL;
}

VitaNexusStoreResult vita_nexus_library_set(VitaNexusLibrary *library, const char *game_id,
    uint8_t flags, uint8_t rating)
{
    if (!library || library->count > VITA_NEXUS_LIBRARY_MAX_RECORDS || !valid_id(game_id) ||
        (flags & (uint8_t)~7u) || rating > 10)
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
            uint32_t expected = 0;
            if (!parse_checksum_line(line, &expected) || expected != hash) {
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
        unsigned int flags = 0;
        unsigned int rating = 0;
        if (!parse_unsigned_decimal(first, 7u, &flags) ||
            !parse_unsigned_decimal(second, 10u, &rating)) {
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
    FILE *file = NULL;
    int fd = -1;
    uint32_t hash = 2166136261u;
    if (!library || !path || library->count > VITA_NEXUS_LIBRARY_MAX_RECORDS ||
        strlen(path) + 32 >= sizeof(temp_path)) return VITA_NEXUS_STORE_INVALID;
    /* Exclusive creation prevents following a pre-existing temporary symlink. */
    for (unsigned int attempt = 0; attempt < 16u; ++attempt) {
        int length = snprintf(temp_path, sizeof(temp_path), "%s.tmp.%ld.%u", path, (long)getpid(), attempt);
        if (length <= 0 || (size_t)length >= sizeof(temp_path)) return VITA_NEXUS_STORE_INVALID;
        fd = open(temp_path, O_WRONLY | O_CREAT | O_EXCL, 0600);
        if (fd >= 0) break;
    }
    if (fd < 0) return VITA_NEXUS_STORE_IO_ERROR;
    file = fdopen(fd, "wb");
    if (!file) { close(fd); remove(temp_path); return VITA_NEXUS_STORE_IO_ERROR; }
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
    if (fprintf(file, "END|%08X\n", (unsigned int)hash) < 0 || fflush(file) != 0 || fsync(fileno(file)) != 0) {
        fclose(file); remove(temp_path); return VITA_NEXUS_STORE_IO_ERROR;
    }
    if (fclose(file) != 0) { remove(temp_path); return VITA_NEXUS_STORE_IO_ERROR; }
    if (rename(temp_path, path) != 0) { remove(temp_path); return VITA_NEXUS_STORE_IO_ERROR; }
    return VITA_NEXUS_STORE_OK;
}
