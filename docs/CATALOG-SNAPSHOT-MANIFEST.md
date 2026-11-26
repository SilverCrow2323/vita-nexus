# VITA NEXUS · Public catalog snapshot manifest

> **DATA DROP / VERIFIED METADATA ONLY**  
> This manifest records the exact public JSON snapshot used by the web companion and native read-only catalog index. It does not describe game binaries or package availability.

## Snapshot identity

- File: `web/data/catalog.json`
- Schema version: `1.0.0`
- Generated at (UTC): `2026-11-26T13:52:42Z`
- SHA-256: `0ff7411007f4cf9b2b9aecf48c8f3287108c2ca9bfedcb8e0233ec257afb1f74`
- Git blob SHA-1: `1efb1b31cee4a2152f7159e41a359c7b352dc927`
- Import workflow: [#45833391711](https://github.com/SilverCrow2323/vita-nexus/actions/runs/45833391711)

## Canonical counts

| Record type | Count |
|---|---:|
| Games | **9,965** |
| Releases | **12,590** |
| Homebrew entries | **4,699** |

## Source rows

| Source | Dataset | Fetched at (UTC) | Accepted rows |
|---|---|---|---:|
| NoPayStation | `PSV_GAMES` | `2026-11-26T13:52:41Z` | 3,784 |
| NoPayStation | `PSP_GAMES` | `2026-11-26T13:52:41Z` | 2,459 |
| NoPayStation | `PSX_GAMES` | `2026-11-26T13:52:41Z` | 1,648 |
| VitaHomebrewDB | `apps.json` | `2026-11-26T13:52:42Z` | 1,536 |
| VitaHomebrewDB | `psp_apps.json` | `2026-11-26T13:52:42Z` | 3,163 |

The NPS importer records accepted, deduplicated rows per source dataset and reports malformed rows in its source notes. VitaHomebrewDB rows are normalized to descriptive metadata; the homebrew total above is recalculated from canonical release records.

## Integrity boundary

- The snapshot is metadata-only.
- Package URLs, zRIF/license values, checksums, package digests and binary payloads are excluded.
- Artwork remains an attributed external reference; this repository does not mirror protected game content.
- The native Vita build compiles a read-only C index from this exact JSON and embeds its schema version, generation timestamp and source SHA-256.

The catalog-summary regression test recalculates the counts and SHA-256 from the checked-in JSON and verifies that this manifest, README and roadmap agree. The catalog sync workflow regenerates these documents in the same commit as each refreshed snapshot.
