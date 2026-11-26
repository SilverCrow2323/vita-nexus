#define _POSIX_C_SOURCE 200809L
#include "profile_store.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


static FILE *open_unique_temp(const char *path, char *temp_path, size_t capacity)
{
    int descriptor;
    FILE *file;
    if (!path || !temp_path || strlen(path) + sizeof(".tmp.XXXXXX") > capacity)
        return NULL;
    if (snprintf(temp_path, capacity, "%s.tmp.XXXXXX", path) < 0)
        return NULL;
    descriptor = mkstemp(temp_path);
    if (descriptor < 0) return NULL;
    file = fdopen(descriptor, "wb");
    if (!file) {
        close(descriptor);
        remove(temp_path);
        return NULL;
    }
    return file;
}

#define ACTIVE_HEADER "VNXP|1\n"

static uint32_t profile_checksum(const char *value, size_t length)
{
    uint32_t hash = 2166136261u;
    for (size_t i = 0; i < length; ++i) {
        hash ^= (uint8_t)value[i];
        hash *= 16777619u;
    }
    return hash;
}

/*
 * Active-profile files are deliberately tiny and canonical:
 *   VNXP|1\n
 *   <one digit from 0 to 2>\n
 *   <exactly eight hexadecimal checksum digits>\n
 *
 * Do not let scanf's permissive numeric grammar accept alternate encodings
 * such as "+1", "01" or leading whitespace. They are not written by this
 * application and should be treated as malformed state.
 */
static int parse_profile_slot(const char *line, unsigned int *profile)
{
    if (!line || !profile || strlen(line) != 2u ||
        line[0] < '0' || line[0] > '2' || line[1] != '\n') {
        return 0;
    }
    *profile = (unsigned int)(line[0] - '0');
    return 1;
}

static int parse_checksum_line(const char *line, uint32_t *checksum)
{
    uint32_t value = 0;
    if (!line || !checksum || strlen(line) != 9u || line[8] != '\n') return 0;
    for (size_t i = 0; i < 8; ++i) {
        const unsigned char c = (unsigned char)line[i];
        unsigned int digit;
        if (c >= '0' && c <= '9') digit = (unsigned int)(c - '0');
        else if (c >= 'a' && c <= 'f') digit = (unsigned int)(c - 'a') + 10u;
        else if (c >= 'A' && c <= 'F') digit = (unsigned int)(c - 'A') + 10u;
        else return 0;
        value = (value << 4) | digit;
    }
    *checksum = value;
    return 1;
}

const char *vita_nexus_profile_data_path(unsigned int profile)
{
    static const char *paths[VITA_NEXUS_PROFILE_COUNT] = {
        "ux0:data/VitaNexus-library-v1.dat",
        "ux0:data/VitaNexus-profile-2-v1.dat",
        "ux0:data/VitaNexus-profile-3-v1.dat"
    };
    return profile < VITA_NEXUS_PROFILE_COUNT ? paths[profile] : NULL;
}

const char *vita_nexus_profile_backup_path(unsigned int profile)
{
    static const char *paths[VITA_NEXUS_PROFILE_COUNT] = {
        "ux0:data/VitaNexus-backup-1-v1.dat",
        "ux0:data/VitaNexus-backup-2-v1.dat",
        "ux0:data/VitaNexus-backup-3-v1.dat"
    };
    return profile < VITA_NEXUS_PROFILE_COUNT ? paths[profile] : NULL;
}

VitaNexusStoreResult vita_nexus_profile_load_active(const char *path, unsigned int *profile)
{
    FILE *file;
    char line[32];
    char checksum_line[32];
    unsigned int slot;
    uint32_t expected;

    if (!path || !profile) return VITA_NEXUS_STORE_INVALID;
    file = fopen(path, "rb");
    if (!file) return VITA_NEXUS_STORE_NOT_FOUND;
    if (!fgets(line, sizeof(line), file) || strcmp(line, ACTIVE_HEADER) != 0 ||
        !fgets(line, sizeof(line), file) ||
        !parse_profile_slot(line, &slot) ||
        !fgets(checksum_line, sizeof(checksum_line), file) ||
        !parse_checksum_line(checksum_line, &expected) ||
        expected != profile_checksum(line, strlen(line)) ||
        fgetc(file) != EOF || ferror(file)) {
        fclose(file);
        return VITA_NEXUS_STORE_INVALID;
    }
    fclose(file);
    *profile = slot;
    return VITA_NEXUS_STORE_OK;
}

VitaNexusStoreResult vita_nexus_profile_save_active(const char *path, unsigned int profile)
{
    char temp_path[256];
    char line[32];
    FILE *file;
    int length;
    uint32_t checksum;

    if (!path || profile >= VITA_NEXUS_PROFILE_COUNT || strlen(path) + sizeof(".tmp.XXXXXX") > sizeof(temp_path))
        return VITA_NEXUS_STORE_INVALID;
    snprintf(temp_path, sizeof(temp_path), "%s.tmp", path);
    length = snprintf(line, sizeof(line), "%u\n", profile);
    if (length <= 0 || (size_t)length >= sizeof(line)) return VITA_NEXUS_STORE_INVALID;
    checksum = profile_checksum(line, (size_t)length);

    file = fopen(temp_path, "wb");
    if (!file) return VITA_NEXUS_STORE_IO_ERROR;
    if (fputs(ACTIVE_HEADER, file) < 0 ||
        fwrite(line, 1, (size_t)length, file) != (size_t)length ||
        fprintf(file, "%08X\n", (unsigned int)checksum) < 0 ||
        fflush(file) != 0) {
        fclose(file);
        remove(temp_path);
        return VITA_NEXUS_STORE_IO_ERROR;
    }
    if (fclose(file) != 0) {
        remove(temp_path);
        return VITA_NEXUS_STORE_IO_ERROR;
    }
    if (rename(temp_path, path) != 0) {
        remove(temp_path);
        return VITA_NEXUS_STORE_IO_ERROR;
    }
    return VITA_NEXUS_STORE_OK;
}

VitaNexusStoreResult vita_nexus_profile_export_backup(const char *profile_path, const char *backup_path)
{
    VitaNexusLibrary snapshot;
    VitaNexusStoreResult result;
    if (!profile_path || !backup_path) return VITA_NEXUS_STORE_INVALID;
    vita_nexus_library_init(&snapshot);
    result = vita_nexus_library_load(&snapshot, profile_path);
    if (result != VITA_NEXUS_STORE_OK) return result;
    return vita_nexus_library_save(&snapshot, backup_path);
}

VitaNexusStoreResult vita_nexus_profile_import_backup(const char *backup_path, const char *profile_path,
    VitaNexusLibrary *imported)
{
    VitaNexusLibrary snapshot;
    VitaNexusStoreResult result;
    if (!backup_path || !profile_path || !imported) return VITA_NEXUS_STORE_INVALID;
    vita_nexus_library_init(&snapshot);
    result = vita_nexus_library_load(&snapshot, backup_path);
    if (result != VITA_NEXUS_STORE_OK) return result;
    result = vita_nexus_library_save(&snapshot, profile_path);
    if (result != VITA_NEXUS_STORE_OK) return result;
    *imported = snapshot;
    return VITA_NEXUS_STORE_OK;
}

void vita_nexus_profile_get_stats(const VitaNexusLibrary *library, VitaNexusProfileStats *stats)
{
    if (!stats) return;
    memset(stats, 0, sizeof(*stats));
    if (!library || library->count > VITA_NEXUS_LIBRARY_MAX_RECORDS) return;
    stats->total = library->count;
    for (size_t i = 0; i < library->count; ++i) {
        const VitaNexusLibraryRecord *record = &library->records[i];
        if (record->flags & VITA_NEXUS_LIBRARY_FAVORITE) ++stats->favorites;
        if (record->flags & VITA_NEXUS_LIBRARY_BACKLOG) ++stats->backlog;
        if (record->flags & VITA_NEXUS_LIBRARY_COMPLETED) ++stats->completed;
        if (record->rating > 0) {
            ++stats->rated;
            stats->rating_total += record->rating;
        }
    }
}
