#ifndef VITA_NEXUS_PROFILE_STORE_H
#define VITA_NEXUS_PROFILE_STORE_H

#include "library_store.h"

#include <stddef.h>
#include <stdint.h>

#define VITA_NEXUS_PROFILE_COUNT 3

typedef struct {
    size_t total;
    size_t favorites;
    size_t backlog;
    size_t completed;
    size_t rated;
    unsigned int rating_total;
} VitaNexusProfileStats;

const char *vita_nexus_profile_data_path(unsigned int profile);
const char *vita_nexus_profile_backup_path(unsigned int profile);
VitaNexusStoreResult vita_nexus_profile_load_active(const char *path, unsigned int *profile);
VitaNexusStoreResult vita_nexus_profile_save_active(const char *path, unsigned int profile);
VitaNexusStoreResult vita_nexus_profile_export_backup(const char *profile_path, const char *backup_path);
VitaNexusStoreResult vita_nexus_profile_import_backup(const char *backup_path, const char *profile_path,
    VitaNexusLibrary *imported);
void vita_nexus_profile_get_stats(const VitaNexusLibrary *library, VitaNexusProfileStats *stats);

#endif
