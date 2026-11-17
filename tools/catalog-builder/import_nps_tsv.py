#!/usr/bin/env python3
"""Import public NoPayStation TSV indexes into the VITA NEXUS metadata catalog.

The importer deliberately discards package URLs, zRIF/license values and package
digests. It only emits descriptive index metadata required for catalog browsing.
"""

from __future__ import annotations

import argparse
import csv
import io
import json
import re
import sys
import unicodedata
import urllib.request
from collections import defaultdict
from datetime import datetime, timezone
from pathlib import Path
from typing import Any


SCHEMA_VERSION = "1.0.0"
SOURCE_BASE = "https://nopaystation.org/tsv"

DATASETS = {
    "PSV_GAMES": {
        "platform": "psvita",
        "name": 2,
        "region": 1,
        "content_id": 5,
        "updated_at": 6,
        "size": 8,
        "firmware": 10,
    },
    "PSP_GAMES": {
        "platform": "psp",
        "name": 3,
        "region": 1,
        "content_id": 5,
        "updated_at": 6,
        "size": 9,
        "firmware": None,
    },
    "PSX_GAMES": {
        "platform": "ps1",
        "name": 2,
        "region": 1,
        "content_id": 4,
        "updated_at": 5,
        "size": 8,
        "firmware": None,
    },
}

REGION_ALIASES = {
    "EU": "EU",
    "EUR": "EU",
    "US": "US",
    "USA": "US",
    "NA": "US",
    "JP": "JP",
    "JPN": "JP",
    "AS": "AS",
    "ASA": "AS",
    "ASIA": "AS",
    "INT": "INT",
    "KR": "KR",
    "CN": "CN",
}


def clean(value: str | None) -> str:
    return (value or "").replace("\ufeff", "").strip()


def safe_int(value: str | None) -> int | None:
    raw = clean(value)
    if not raw:
        return None
    try:
        number = int(raw)
    except ValueError:
        return None
    return number if number >= 0 else None


def normalize_title(value: str) -> str:
    value = unicodedata.normalize("NFKD", value).casefold()
    value = "".join(ch for ch in value if not unicodedata.combining(ch))
    return " ".join(re.findall(r"[a-z0-9]+", value))


def slugify(value: str) -> str:
    normalized = normalize_title(value)
    return re.sub(r"[^a-z0-9]+", "-", normalized).strip("-") or "untitled"


def read_text(url: str, timeout: int) -> str:
    request = urllib.request.Request(
        url,
        headers={"User-Agent": "VITA-NEXUS-CatalogBuilder/0.1 (+https://github.com/SilverCrow2323/vita-nexus)"},
    )
    with urllib.request.urlopen(request, timeout=timeout) as response:
        raw = response.read()
    return raw.decode("utf-8-sig", errors="replace")


def parse_dataset(dataset: str, text: str, source_url: str) -> tuple[list[dict[str, Any]], int]:
    if dataset not in DATASETS:
        raise ValueError(f"Unsupported dataset: {dataset}")

    layout = DATASETS[dataset]
    platform = layout["platform"]
    reader = csv.reader(io.StringIO(text), delimiter="\t")
    rows = iter(reader)
    next(rows, None)  # NPS TSVs have a header row; PKGj also skips the first line.

    releases: list[dict[str, Any]] = []
    skipped = 0

    for line_number, fields in enumerate(rows, start=2):
        if not fields or all(not clean(field) for field in fields):
            continue

        required_indexes = [
            0,
            layout["region"],
            layout["name"],
            layout["content_id"],
            layout["updated_at"],
            layout["size"],
        ]
        if layout["firmware"] is not None:
            required_indexes.append(layout["firmware"])

        if len(fields) <= max(required_indexes):
            skipped += 1
            continue

        title_id = clean(fields[0]).upper()
        name = clean(fields[layout["name"]])
        if not title_id or not name:
            skipped += 1
            continue

        raw_region = clean(fields[layout["region"]]).upper()
        region = REGION_ALIASES.get(raw_region, "UNKNOWN")
        content_id = clean(fields[layout["content_id"]]) or None
        updated_at = clean(fields[layout["updated_at"]])
        size_bytes = safe_int(fields[layout["size"]])
        firmware_index = layout["firmware"]
        required_firmware = clean(fields[firmware_index]) if firmware_index is not None else ""

        release_id = ":".join(
            [platform, title_id, region, content_id or str(line_number)]
        )

        releases.append(
            {
                "releaseId": release_id,
                "platform": platform,
                "titleId": title_id,
                "contentId": content_id,
                "region": region,
                "languageCodes": [],
                "version": "",
                "packageSizeBytes": size_bytes,
                "requiredFirmware": required_firmware,
                "sourceId": "nopaystation",
                "sourceDataset": dataset,
                "sourceUpdatedAt": updated_at,
            }
        )

    return releases, skipped


def build_catalog(
    dataset_texts: dict[str, str],
    source_base: str = SOURCE_BASE,
) -> dict[str, Any]:
    grouped: dict[tuple[str, str], dict[str, Any]] = {}
    source_rows: list[dict[str, Any]] = []
    generated_at = datetime.now(timezone.utc).replace(microsecond=0).isoformat().replace("+00:00", "Z")

    for dataset in DATASETS:
        if dataset not in dataset_texts:
            continue

        source_url = f"{source_base.rstrip('/')}/{dataset}.tsv"
        parsed_releases, skipped = parse_dataset(dataset, dataset_texts[dataset], source_url)
        layout = DATASETS[dataset]
        name_index = layout["name"]
        reader = csv.reader(io.StringIO(dataset_texts[dataset]), delimiter="\t")
        rows = iter(reader)
        next(rows, None)

        names_by_release: dict[str, str] = {}
        for line_number, fields in enumerate(rows, start=2):
            if len(fields) > name_index and clean(fields[0]) and clean(fields[name_index]):
                names_by_release[":".join([
                    layout["platform"],
                    clean(fields[0]).upper(),
                    REGION_ALIASES.get(clean(fields[layout["region"]]).upper(), "UNKNOWN"),
                    clean(fields[layout["content_id"]]) if len(fields) > layout["content_id"] else "",
                ])] = clean(fields[name_index])

        for release in parsed_releases:
            title = names_by_release.get(
                ":".join([
                    release["platform"],
                    release["titleId"],
                    release["region"],
                    release["contentId"] or "",
                ]),
                release["titleId"],
            )
            key = (release["platform"], normalize_title(title))
            if key not in grouped:
                grouped[key] = {
                    "gameId": f"{release['platform']}-{slugify(title)}",
                    "title": title,
                    "sortTitle": title,
                    "platforms": [release["platform"]],
                    "releases": [],
                }
            grouped[key]["releases"].append(release)

        source_rows.append(
            {
                "sourceId": "nopaystation",
                "name": "NoPayStation",
                "dataset": dataset,
                "url": source_url,
                "fetchedAt": generated_at,
                "recordCount": len(parsed_releases),
                "notes": f"Metadata-only import; {skipped} malformed rows skipped. Package URLs, license values and digests are discarded.",
            }
        )

    games = list(grouped.values())
    for game in games:
        game["releases"].sort(
            key=lambda item: (item["region"], item["titleId"], item["releaseId"])
        )
    games.sort(key=lambda item: (item["platforms"][0], item["title"].casefold()))

    release_count = sum(len(game["releases"]) for game in games)
    return {
        "schemaVersion": SCHEMA_VERSION,
        "generatedAt": generated_at,
        "sources": source_rows,
        "totals": {"games": len(games), "releases": release_count},
        "games": games,
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=Path("web/data/catalog.json"))
    parser.add_argument("--source-base", default=SOURCE_BASE)
    parser.add_argument("--timeout", type=int, default=45)
    parser.add_argument("--datasets", nargs="+", choices=sorted(DATASETS), default=list(DATASETS))
    args = parser.parse_args()

    dataset_texts: dict[str, str] = {}
    for dataset in args.datasets:
        url = f"{args.source_base.rstrip('/')}/{dataset}.tsv"
        print(f"Fetching metadata index: {dataset}", file=sys.stderr)
        dataset_texts[dataset] = read_text(url, timeout=args.timeout)

    catalog = build_catalog(dataset_texts, args.source_base)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(
        json.dumps(catalog, ensure_ascii=False, indent=2) + "\n",
        encoding="utf-8",
    )
    print(
        f"Wrote {catalog['totals']['games']} games / "
        f"{catalog['totals']['releases']} releases to {args.output}",
        file=sys.stderr,
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
