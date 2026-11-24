# VITA NEXUS · Public catalog snapshot manifest

> **DATA DROP / VERIFIED METADATA ONLY**  
> This manifest records the exact public JSON snapshot used by the web companion and native read-only catalog index. It does not describe game binaries or package availability.

## Snapshot identity

- File: `web/data/catalog.json`
- Schema version: `1.0.0`
- Generated at (UTC): `2026-11-24T14:50:38Z`
- SHA-256: `eaab920574f0d0b144f3519a7c372b372a7f32f25ba59afc1a9cd9ccd14ebf1e`
- Git blob SHA-1: `21ed205c26050a7857c7166f0f42b0a9c350df9c`
- Import workflow: [#45418589079](https://github.com/SilverCrow2323/vita-nexus/actions/runs/45418589079)

## Canonical counts

| Record type | Count |
|---|---:|
| Games | **9,885** |
| Releases | **12,510** |
| Homebrew entries | **4,619** |

## Source rows

| Source | Dataset | Fetched at (UTC) | Accepted rows |
|---|---|---|---:|
| NoPayStation | `PSV_GAMES` | `2026-11-24T14:50:37Z` | 3,784 |
| NoPayStation | `PSP_GAMES` | `2026-11-24T14:50:37Z` | 2,459 |
| NoPayStation | `PSX_GAMES` | `2026-11-24T14:50:37Z` | 1,648 |
| VitaHomebrewDB | `apps.json` | `2026-11-24T14:50:38Z` | 1,523 |
| VitaHomebrewDB | `psp_apps.json` | `2026-11-24T14:50:38Z` | 3,096 |

The NPS importer records accepted, deduplicated rows per source dataset and reports malformed rows in its source notes. VitaHomebrewDB rows are normalized to descriptive metadata; the homebrew total above is recalculated from canonical release records.

## Integrity boundary

- The snapshot is metadata-only.
- Package URLs, zRIF/license values, checksums, package digests and binary payloads are excluded.
- Artwork remains an attributed external reference; this repository does not mirror protected game content.
- The native Vita build compiles a read-only C index from this exact JSON and embeds its schema version, generation timestamp and source SHA-256.

The catalog-summary regression test recalculates the counts and SHA-256 from the checked-in JSON and verifies that this manifest, README and roadmap agree. The catalog sync workflow regenerates these documents in the same commit as each refreshed snapshot.
