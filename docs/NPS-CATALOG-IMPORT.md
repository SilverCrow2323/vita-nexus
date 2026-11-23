# 🗂️ NoPayStation catalog import

VITA NEXUS can read the public NoPayStation TSV indexes as a source of **release metadata** for its informational Store and catalog.

This integration is intentionally read-only and metadata-only.

## Current source indexes

| Dataset | Platform | Source |
|---|---|---|
| PSV_GAMES | PS Vita | https://nopaystation.org/tsv/PSV_GAMES.tsv |
| PSP_GAMES | PSP | https://nopaystation.org/tsv/PSP_GAMES.tsv |
| PSX_GAMES | PS1 Classics | https://nopaystation.org/tsv/PSX_GAMES.tsv |

The source list is maintained by NoPayStation and documents TSV indexes for PKGj, NPS Browser and pyNPS. PKGj's parser maps different columns for Vita, PSP and PS1 datasets; the importer follows those platform-specific layouts rather than assuming every file is identical.

## What VITA NEXUS keeps

- Title ID
- Display title
- Region
- Content ID
- Source modification date
- Package size as a descriptive size field
- Required firmware, when supplied
- Source dataset and provenance

## What VITA NEXUS discards

The generated public catalog does **not** contain package download URLs, zRIF/license values or package digests. The web Store is an informational index, not a package downloader or a mirror of protected content.

Do not add those fields to the public schema, generated JSON, browser UI, logs or test snapshots.

The public validator normalizes key case and punctuation before applying its denylist, and rejects non-object catalog roots with a controlled validation error. Regression tests cover hyphenated, dotted and underscored payload-key variants.

## Importer

Run from the repository root:

~~~bash
python tools/catalog-builder/import_nps_tsv.py --output web/data/catalog.json
~~~

Run unit tests:

~~~bash
cd tools/catalog-builder
python -m unittest -v
~~~

The importer writes a stable JSON snapshot matching [catalog.schema.json](../catalog/schema/catalog.schema.json). It stores a generation timestamp and per-source record counts.

**Identity contract:** every canonical `gameId` and every release `releaseId` must be non-empty and unique across the complete snapshot. The shared native-index validator enforces both identities before generating C data, so direct generator runs fail closed just like the public catalog validation workflow.

**Native SQLite size contract:** `packageSizeBytes` is either `null` (unknown) or a non-negative integer. The SQLite importer rejects booleans, strings, floating-point values and negative numbers instead of silently converting malformed metadata into an unknown size. This validation happens before database migration/import writes, so a bad snapshot cannot replace the last accepted catalog.

## Regional grouping

The first automatic grouping pass is conservative: releases with the same normalized title on the same platform are grouped under one catalog entry. PSP and PS1 records remain distinct, even when the display title is the same.

Cross-platform equivalence is editorial data. Do not automatically merge different platforms using title text alone; sequels, remakes and unrelated games can share names.

## Refresh automation

The scheduled workflow runs daily and can also be started manually from GitHub Actions. When a snapshot changes, it commits the metadata-only JSON file. The Pages workflow then republishes the updated web companion.

## Editorial enrichment

NPS rows do not provide the complete editorial content expected by VITA NEXUS. Descriptions, genre tags, age ratings, screenshots, cover art, hero artwork and logos belong in the separate editorial enrichment layer.

Each external asset and score must have source attribution and usage terms recorded. Missing metadata should stay missing rather than being guessed.

## Homebrew catalog source

PS Vita and PSP homebrew metadata is imported from [VitaHomebrewDB](https://github.com/DrDecki/VitaHomebrewDB), using its public `apps.json` and `psp_apps.json` datasets. The upstream catalog preserves metadata such as app name, author, version, description, changelog, requirements, icon and screenshots.

VITA NEXUS keeps descriptive metadata and references icons/screenshots at the upstream static host. It does **not** copy binaries, package URLs, hashes or payload fields into the public catalog.

### Attribution and ownership

The upstream README explicitly states that the catalog data is not covered by the repository's MIT license: names, descriptions, changelogs, icons and screenshots belong to the original creators. VITA NEXUS therefore:

- links back to VitaHomebrewDB;
- retains the original author and project-source links where supplied;
- points artwork references at the upstream host instead of mirroring assets;
- treats images as externally hosted material, not VITA NEXUS-owned artwork;
- can remove or correct entries when upstream creators request it.

See the [VitaHomebrewDB README and credits](https://github.com/DrDecki/VitaHomebrewDB#license).

A homebrew release is represented as a `homebrew` catalog release with a separate `targetPlatform` value (`psvita` or `psp`). It is not merged into commercial NPS releases. The upstream entry ID remains the stable source item identifier.

The importer deliberately excludes `url`, `data`, `hash`, `hash2`, checksum and package payload fields. Project source and release-page links are retained only as ordinary informational links.

## Latest verified snapshot · 2026-11-23

The canonical snapshot on main was regenerated and validated by [catalog sync run #45162162133](https://github.com/SilverCrow2323/vita-nexus/actions/runs/45162162133).

| Measure | Verified value |
|---|---:|
| Schema version | 1.0.0 |
| Generated at (UTC) | 2026-11-23T11:07:00Z |
| Canonical game records | **9,816** |
| Release records | **12,441** |
| Homebrew entries | **4,550** |

| Source dataset | Fetched at (UTC) | Accepted rows |
|---|---|---:|
| PSV_GAMES | 2026-11-23T11:06:59Z | 3,784 |
| PSP_GAMES | 2026-11-23T11:06:59Z | 2,459 |
| PSX_GAMES | 2026-11-23T11:06:59Z | 1,648 |
| apps.json | 2026-11-23T11:07:00Z | 1,454 |
| psp_apps.json | 2026-11-23T11:07:00Z | 3,096 |

The exact UTF-8 snapshot file web/data/catalog.json has SHA-256 eb04e02430389f5e5ce5770ecbdb2ec0ce57e32bed7d2bd1081cdcc0fc303c63 (Git blob SHA-1 43cf510b0143ecb33837a55e16f29f99a9d5873a). Counts are recalculated from canonical JSON records.

The scheduled importer keeps package URLs, license values, checksums and payloads out of the public catalog. This section is regenerated with the snapshot manifest, README and roadmap in the same catalog-sync commit.
