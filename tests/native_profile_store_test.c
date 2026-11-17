#include "profile_store.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void write_corrupt_backup(const char *path)
{
    FILE *file = fopen(path, "wb");
    assert(file != NULL);
    fputs("VNXLIB|1\nnot-valid|1|2\nEND|00000000\n", file);
    fclose(file);
}

int main(void)
{
    const char *active_path = "/tmp/vita-nexus-active-profile.dat";
    const char *profile_path = "/tmp/vita-nexus-profile.dat";
    const char *backup_path = "/tmp/vita-nexus-profile-backup.dat";
    const char *corrupt_path = "/tmp/vita-nexus-profile-corrupt.dat";
    unsigned int active = 99;
    VitaNexusLibrary profile, imported, untouched;
    VitaNexusProfileStats stats;

    assert(strcmp(vita_nexus_profile_data_path(0), "ux0:data/VitaNexus-library-v1.dat") == 0);
    assert(vita_nexus_profile_data_path(VITA_NEXUS_PROFILE_COUNT) == NULL);
    assert(vita_nexus_profile_backup_path(2) != NULL);
    assert(vita_nexus_profile_backup_path(VITA_NEXUS_PROFILE_COUNT) == NULL);

    assert(vita_nexus_profile_load_active(active_path, &active) == VITA_NEXUS_STORE_NOT_FOUND);
    assert(vita_nexus_profile_save_active(active_path, 2) == VITA_NEXUS_STORE_OK);
    assert(vita_nexus_profile_load_active(active_path, &active) == VITA_NEXUS_STORE_OK);
    assert(active == 2);
    assert(vita_nexus_profile_save_active(active_path, VITA_NEXUS_PROFILE_COUNT) == VITA_NEXUS_STORE_INVALID);
    FILE *broken_active = fopen(active_path, "wb");
    assert(broken_active != NULL);
    fputs("VNXP|1\\n1\\n00000000\\n", broken_active);
    fclose(broken_active);
    assert(vita_nexus_profile_load_active(active_path, &active) == VITA_NEXUS_STORE_INVALID);
    assert(active == 2);
    assert(vita_nexus_profile_save_active(active_path, 2) == VITA_NEXUS_STORE_OK);

    vita_nexus_library_init(&profile);
    assert(vita_nexus_library_set(&profile, "game-favorite", VITA_NEXUS_LIBRARY_FAVORITE, 9) == VITA_NEXUS_STORE_OK);
    assert(vita_nexus_library_set(&profile, "game-backlog", VITA_NEXUS_LIBRARY_BACKLOG, 0) == VITA_NEXUS_STORE_OK);
    assert(vita_nexus_library_set(&profile, "game-done", VITA_NEXUS_LIBRARY_COMPLETED, 7) == VITA_NEXUS_STORE_OK);
    assert(vita_nexus_library_save(&profile, profile_path) == VITA_NEXUS_STORE_OK);
    assert(vita_nexus_profile_export_backup(profile_path, backup_path) == VITA_NEXUS_STORE_OK);

    vita_nexus_profile_get_stats(&profile, &stats);
    assert(stats.total == 3 && stats.favorites == 1 && stats.backlog == 1);
    assert(stats.completed == 1 && stats.rated == 2 && stats.rating_total == 16);

    vita_nexus_library_init(&imported);
    assert(vita_nexus_library_set(&imported, "old-entry", VITA_NEXUS_LIBRARY_FAVORITE, 1) == VITA_NEXUS_STORE_OK);
    assert(vita_nexus_profile_import_backup(backup_path, profile_path, &imported) == VITA_NEXUS_STORE_OK);
    assert(imported.count == 3);
    assert(vita_nexus_library_find(&imported, "old-entry") == NULL);
    assert(vita_nexus_library_find(&imported, "game-done") != NULL);

    write_corrupt_backup(corrupt_path);
    untouched = imported;
    assert(vita_nexus_profile_import_backup(corrupt_path, profile_path, &imported) == VITA_NEXUS_STORE_INVALID);
    assert(imported.count == untouched.count);
    assert(vita_nexus_library_find(&imported, "game-favorite") != NULL);

    remove(active_path);
    remove("/tmp/vita-nexus-active-profile.dat.tmp");
    remove(profile_path);
    remove(profile_path[0] ? backup_path : backup_path);
    remove(corrupt_path);
    remove("/tmp/vita-nexus-profile.dat.tmp");
    remove("/tmp/vita-nexus-profile-backup.dat.tmp");
    return 0;
}
