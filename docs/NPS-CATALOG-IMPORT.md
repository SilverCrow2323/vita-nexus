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

## Regional grouping

The first automatic grouping pass is conservative: releases with the same normalized title on the same platform are grouped under one catalog entry. PSP and PS1 records remain distinct, even when the display title is the same.

Cross-platform equivalence is editorial data. Do not automatically merge different platforms using title text alone; sequels, remakes and unrelated games can share names.

## Refresh automation

The scheduled workflow runs daily and can also be started manually from GitHub Actions. When a snapshot changes, it commits the metadata-only JSON file. The Pages workflow then republishes the updated web companion.

## Editorial enrichment

NPS rows do not provide the complete editorial content expected by VITA NEXUS. Descriptions, genre tags, age ratings, screenshots, cover art, hero artwork and logos belong in the separate editorial enrichment layer.

Each external asset and score must have source attribution and usage terms recorded. Missing metadata should stay missing rather than being guessed.
