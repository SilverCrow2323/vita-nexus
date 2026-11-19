# VITA NEXUS · Public catalog snapshot manifest

> **DATA DROP / VERIFIED METADATA ONLY**  
> This manifest records the exact public JSON snapshot used by the web companion and native read-only catalog index. It does not describe game binaries or package availability.

## Snapshot identity

- File: `web/data/catalog.json`
- Schema version: `1.0.0`
- Generated at (UTC): `2026-11-18T13:01:01Z`
- SHA-256: `e60cfe04a3d1b89024980a9f1a83682eaa4c1f40742f8060aaf9807c78245bb0`
- Git blob SHA-1: `ba0e263ebdec8d32af1438649323a11f7e265b27`
- Import workflow: [#44309423607](https://github.com/SilverCrow2323/vita-nexus/actions/runs/44309423607)
- Snapshot commit: `73b0ec32e3cf96b2e4d4e5e6e679f59b85e236a3` (included in main HEAD `e24d0d7bb02d880738e6f0f664f9601c22983d65))

## Canonical counts

| Record type | Count |
|---|---:|
| Games | **9,734** |
| Releases | **12,359** |
| Homebrew entries | **4,468** |

## Source rows

| Source | Dataset | Fetched at (UTC) | Accepted rows |
|---|---|---|---:|
| NoPayStation | `PSV_GAMES` | `2026-11-18T13:01:00Z` | 3,784 |
| NoPayStation | `PSP_GAMES` | `2026-11-18T13:01:00Z` | 2,459 |
| NoPayStation | `PSX_GAMES` | `2026-11-18T13:01:00Z` | 1,648 |
| VitaHomebrewDB | `apps.json` | `2026-11-18T13:01:01Z` | 1,372 |
| VitaHomebrewDB | `psp_apps.json` | `2026-11-18T13:01:01Z` | 3,096 |

The NPS import skipped malformed source rows according to the importer policy; the accepted row counts above are the values stored in the generated snapshot's `sources` metadata. The 4,468 homebrew entries are counted from the canonical release records and match the importer's `recordsAdded` value.

## Integrity boundary

- The snapshot is metadata-only.
- Package URLs, zRIF/license values, checksums, package digests and binary payloads are excluded.
- Artwork remains an attributed external reference; this repository does not mirror protected game content.
- The native Vita build compiles a read-only C index from this exact JSON and embeds its schema version, generation timestamp and source SHA-256.

The catalog-summary regression test recalculates the counts and SHA-256 from the checked-in JSON and verifies that this manifest, README and roadmap agree. When a future import changes the snapshot, update this manifest and the visible documentation in the same reviewed change.
