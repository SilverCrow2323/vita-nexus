#ifndef VITA_NEXUS_LIBRARY_STORE_H
#define VITA_NEXUS_LIBRARY_STORE_H

#include <stddef.h>
#include <stdint.h>

#define VITA_NEXUS_LIBRARY_MAX_RECORDS 512
#define VITA_NEXUS_LIBRARY_ID_CAP 64
#define VITA_NEXUS_LIBRARY_FAVORITE 1u
#define VITA_NEXUS_LIBRARY_BACKLOG 2u
#define VITA_NEXUS_LIBRARY_COMPLETED 4u

typedef struct {
    char game_id[VITA_NEXUS_LIBRARY_ID_CAP];
    uint8_t flags;
    uint8_t rating;
} VitaNexusLibraryRecord;

typedef struct {
    VitaNexusLibraryRecord records[VITA_NEXUS_LIBRARY_MAX_RECORDS];
    size_t count;
} VitaNexusLibrary;

typedef enum {
    VITA_NEXUS_STORE_OK = 0,
    VITA_NEXUS_STORE_NOT_FOUND = 1,
    VITA_NEXUS_STORE_INVALID = -1,
    VITA_NEXUS_STORE_IO_ERROR = -2,
    VITA_NEXUS_STORE_FULL = -3
} VitaNexusStoreResult;

void vita_nexus_library_init(VitaNexusLibrary *library);
const VitaNexusLibraryRecord *vita_nexus_library_find(const VitaNexusLibrary *library, const char *game_id);
VitaNexusStoreResult vita_nexus_library_set(VitaNexusLibrary *library, const char *game_id,
    uint8_t flags, uint8_t rating);
VitaNexusStoreResult vita_nexus_library_load(VitaNexusLibrary *library, const char *path);
VitaNexusStoreResult vita_nexus_library_save(const VitaNexusLibrary *library, const char *path);

#endif
