# VITA NEXUS · Public catalog snapshot manifest

> **DATA DROP / VERIFIED METADATA ONLY**  
> This manifest records the exact public JSON snapshot used by the web companion and native read-only catalog index. It does not describe game binaries or package availability.

## Snapshot identity

- File: `web/data/catalog.json`
- Schema version: `1.0.0`
- Generated at (UTC): `2026-11-19T10:27:26Z`
- SHA-256: `de6fb925167d4065b9d81c9a443cda956db3ef2bbab9bea3e1bf50fad184cacf`
- Git blob SHA-1: `4e6add390e7a8b31a3b9ff19cef1318f50f06b99`
- Import workflow: [#44505804343](https://github.com/SilverCrow2323/vita-nexus/actions/runs/44505804343)

## Canonical counts

| Record type | Count |
|---|---:|
| Games | **9,756** |
| Releases | **12,381** |
| Homebrew entries | **4,490** |

## Source rows

| Source | Dataset | Fetched at (UTC) | Accepted rows |
|---|---|---|---:|
| NoPayStation | `PSV_GAMES` | `2026-11-19T10:27:25Z` | 3,784 |
| NoPayStation | `PSP_GAMES` | `2026-11-19T10:27:25Z` | 2,459 |
| NoPayStation | `PSX_GAMES` | `2026-11-19T10:27:25Z` | 1,648 |
| VitaHomebrewDB | `apps.json` | `2026-11-19T10:27:26Z` | 1,395 |
| VitaHomebrewDB | `psp_apps.json` | `2026-11-19T10:27:26Z` | 3,095 |

The NPS importer records accepted, deduplicated rows per source dataset and reports malformed rows in its source notes. VitaHomebrewDB rows are normalized to descriptive metadata; the homebrew total above is recalculated from canonical release records.

## Integrity boundary

- The snapshot is metadata-only.
- Package URLs, zRIF/license values, checksums, package digests and binary payloads are excluded.
- Artwork remains an attributed external reference; this repository does not mirror protected game content.
- The native Vita build compiles a read-only C index from this exact JSON and embeds its schema version, generation timestamp and source SHA-256.

The catalog-summary regression test recalculates the counts and SHA-256 from the checked-in JSON and verifies that this manifest, README and roadmap agree. The catalog sync workflow regenerates these documents in the same commit as each refreshed snapshot.
