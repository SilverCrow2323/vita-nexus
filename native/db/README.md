# Native SQLite foundation

The native app currently uses its bounded, checksummed local library/profile files. This directory defines the next storage contract without silently switching the running Vita build to a new database dependency.

## What is included

- `migrations/0001_catalog_and_personal_state.sql` defines the base catalog/profile/session schema.
- `migrations/0002_catalog_snapshot_membership.sql` adds source provenance and snapshot-membership flags without deleting historical rows.
- `tools/catalog-builder/import_catalog_sqlite.py` imports the same validated metadata-only JSON snapshot used by the web companion and native C-index builder, recording the exact SHA-256 and source datasets.
- Rows removed from a newer public snapshot are marked `is_in_snapshot = 0`, not deleted; profile entries and play-session history retain their foreign-key targets.
- \`PRAGMA user_version\` is the authoritative migration cursor.
- \`schema_migrations\` records the exact numbered migration name and application time.
- Each migration is applied inside its own \`BEGIN IMMEDIATE\` transaction. A failed statement rolls back both schema changes and the version bump.
- Foreign keys are enabled by the host migration runner and must also be enabled by any future runtime connection.
- Public catalog rows are metadata-only. The schema has no package URL, zRIF, license, ROM, ISO or binary-payload columns.
- Profiles, profile-game state, play sessions and session events are local user-owned records.
- The active checksummed library store validates fixed-size record IDs with an in-bounds NUL check before lookup, mutation or save; malformed in-memory records fail closed rather than reaching unbounded string comparisons.

## Host verification

From the repository root:

\`\`\`sh
python tools/catalog-builder/migrate_native_db.py --database /tmp/vita-nexus.db
python tools/catalog-builder/migrate_native_db.py --database /tmp/vita-nexus.db
python tools/catalog-builder/import_catalog_sqlite.py --catalog web/data/catalog.json --database /tmp/vita-nexus.db
python tools/catalog-builder/import_catalog_sqlite.py --catalog web/data/catalog.json --database /tmp/vita-nexus.db
\`\`\`

The second migration invocation is a no-op. Re-importing the same snapshot keeps catalog rows and the snapshot hash stable while refreshing `imported_at` to record the successful import. The standard-library test suite exercises constraints, foreign keys, source provenance, snapshot hashes, idempotency, safe stale-row retention, sequence gaps and transactional rollback.

## Runtime boundary

This is a **schema and host migration foundation**, not an on-device SQLite implementation. The current VitaSDK CMake target does not link SQLite and the VPK does not open this database. Before runtime integration, verify a maintained SQLite library/package for the pinned VitaSDK 2026.08 toolchain, define database path/backup/restore behavior, and test interrupted migration and low-storage recovery on a physical Vita. Until those gates pass, the existing profile/library file stores remain the active runtime implementation.

Migrations are append-only once shipped. Add \`0002_...sql\` for the next change; never edit a migration that may already have been applied to a user's database.
