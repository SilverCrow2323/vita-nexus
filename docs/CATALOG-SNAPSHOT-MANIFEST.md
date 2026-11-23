# VITA NEXUS · Public catalog snapshot manifest

> **DATA DROP / VERIFIED METADATA ONLY**  
> This manifest records the exact public JSON snapshot used by the web companion and native read-only catalog index. It does not describe game binaries or package availability.

## Snapshot identity

- File: `web/data/catalog.json`
- Schema version: `1.0.0`
- Generated at (UTC): `2026-11-23T11:07:00Z`
- SHA-256: `eb04e02430389f5e5ce5770ecbdb2ec0ce57e32bed7d2bd1081cdcc0fc303c63`
- Git blob SHA-1: `43cf510b0143ecb33837a55e16f29f99a9d5873a`
- Import workflow: [#45162162133](https://github.com/SilverCrow2323/vita-nexus/actions/runs/45162162133)

## Canonical counts

| Record type | Count |
|---|---:|
| Games | **9,816** |
| Releases | **12,441** |
| Homebrew entries | **4,550** |

## Source rows

| Source | Dataset | Fetched at (UTC) | Accepted rows |
|---|---|---|---:|
| NoPayStation | `PSV_GAMES` | `2026-11-23T11:06:59Z` | 3,784 |
| NoPayStation | `PSP_GAMES` | `2026-11-23T11:06:59Z` | 2,459 |
| NoPayStation | `PSX_GAMES` | `2026-11-23T11:06:59Z` | 1,648 |
| VitaHomebrewDB | `apps.json` | `2026-11-23T11:07:00Z` | 1,454 |
| VitaHomebrewDB | `psp_apps.json` | `2026-11-23T11:07:00Z` | 3,096 |

The NPS importer records accepted, deduplicated rows per source dataset and reports malformed rows in its source notes. VitaHomebrewDB rows are normalized to descriptive metadata; the homebrew total above is recalculated from canonical release records.

## Integrity boundary

- The snapshot is metadata-only.
- Package URLs, zRIF/license values, checksums, package digests and binary payloads are excluded.
- Artwork remains an attributed external reference; this repository does not mirror protected game content.
- The native Vita build compiles a read-only C index from this exact JSON and embeds its schema version, generation timestamp and source SHA-256.

The catalog-summary regression test recalculates the counts and SHA-256 from the checked-in JSON and verifies that this manifest, README and roadmap agree. The catalog sync workflow regenerates these documents in the same commit as each refreshed snapshot.
