#!/usr/bin/env python3
"""Merge VitaHomebrewDB Vita/PSP records into the public VITA NEXUS catalog.

Only descriptive metadata and source-hosted artwork references are retained.
Binary download URLs, package payloads and checksum/hash material are discarded.
"""

from __future__ import annotations

import argparse
import json
import re
import sys
import urllib.request
from datetime import datetime, timezone
from pathlib import Path
from typing import Any


SOURCE_REPOSITORY = "https://github.com/DrDecki/VitaHomebrewDB"
SOURCE_RAW_BASE = "https://raw.githubusercontent.com/DrDecki/VitaHomebrewDB/main"
SOURCE_ASSET_BASE = "https://drdecki.github.io/VitaHomebrewDB"
SOURCE_README = SOURCE_REPOSITORY + "/blob/main/README.md"

DATASETS = (
    ("apps.json", "psvita"),
    ("psp_apps.json", "psp"),
)

FORBIDDEN_KEYS = {
    "url",
    "downloadurl",
    "packageurl",
    "data",
    "hash",
    "hash2",
    "sha256",
    "md5",
    "zrif",
    "licensekey",
}


def clean(value: Any) -> str:
    if value is None:
        return ""
    if isinstance(value, (dict, list)):
        return ""
    return str(value).strip()


def safe_int(value: Any) -> int | None:
    try:
        number = int(clean(value))
    except (TypeError, ValueError):
        return None
    return number if number >= 0 else None


def safe_https(value: Any) -> str:
    candidate = clean(value)
    if candidate.startswith("https://") and len(candidate) <= 2048:
        return candidate
    return ""


def source_asset(path: Any, folder: str) -> str:
    candidate = clean(path).replace("\\", "/")
    if not candidate:
        return ""
    candidate = candidate.lstrip("/")
    if candidate.startswith("https://"):
        return candidate if len(candidate) <= 2048 else ""
    if ".." in candidate.split("/"):
        return ""
    if candidate.startswith(folder + "/"):
        candidate = candidate[len(folder) + 1 :]
    if "/" in candidate or not re.fullmatch(r"[A-Za-z0-9._-]+", candidate):
        return ""
    return f"{SOURCE_ASSET_BASE}/{folder}/{candidate}"


def split_screenshots(value: Any) -> list[str]:
    raw = clean(value)
    if not raw:
        return []
    parts = raw.split(";") if ";" in raw else [raw]
    result: list[str] = []
    for part in parts:
        candidate = clean(part)
        if candidate.startswith("screenshots/"):
            filename = candidate[len("screenshots/") :]
        else:
            filename = candidate
        url = source_asset(filename, "screenshots")
        if url and url not in result:
            result.append(url)
    return result[:12]


def read_json(url: str, timeout: int) -> list[dict[str, Any]]:
    request = urllib.request.Request(
        url,
        headers={
            "User-Agent": "VITA-NEXUS-CatalogBuilder/0.2 (+https://github.com/SilverCrow2323/vita-nexus)",
            "Accept": "application/json",
        },
    )
    with urllib.request.urlopen(request, timeout=timeout) as response:
        payload = response.read()
    data = json.loads(payload.decode("utf-8-sig"))
    if not isinstance(data, list):
        raise ValueError(f"Expected a JSON array from {url}")
    return [entry for entry in data if isinstance(entry, dict)]


def normalize_entry(entry: dict[str, Any], target_platform: str, dataset: str) -> dict[str, Any] | None:
    name = clean(entry.get("name"))
    upstream_id = clean(entry.get("id"))
    if not name or not upstream_id:
        return None

    safe_id = re.sub(r"[^A-Za-z0-9_-]+", "-", upstream_id).strip("-")
    if not safe_id:
        return None

    source_entry_id = f"{target_platform}:{safe_id}"
    game_id = f"homebrew-{target_platform}-{safe_id}"
    icon_url = source_asset(entry.get("icon"), "icons")
    screenshot_urls = split_screenshots(entry.get("screenshots"))

    version = clean(entry.get("version"))
    title_id = clean(entry.get("titleid")).upper()
    if title_id in {"AAAAAAAAA", "UNKNOWN", "N/A", "NONE"}:
        title_id = ""

    package_size = safe_int(entry.get("size"))
    release_date = clean(entry.get("updated")) or clean(entry.get("date"))
    year_match = re.match(r"^(19|20|21)\d{2}", clean(entry.get("date")))
    release_year = int(year_match.group(0)) if year_match else None

    short_description = clean(entry.get("description"))
    long_description = clean(entry.get("long_description")) or short_description
    author = clean(entry.get("author"))
    project_url = safe_https(entry.get("source"))
    release_page_url = safe_https(entry.get("release_page"))

    homebrew = {
        "author": author,
        "version": version,
        "categoryCode": clean(entry.get("type")),
        "requirements": clean(entry.get("requirements")),
        "changelog": clean(entry.get("changelog")),
        "sourceCatalog": "VitaHomebrewDB",
        "sourceCatalogEntryId": source_entry_id,
        "sourceCatalogUrl": SOURCE_README,
        "iconUrl": icon_url,
        "screenshotUrls": screenshot_urls,
        "sourceProjectUrl": project_url,
        "releasePageUrl": release_page_url,
    }

    release = {
        "releaseId": f"homebrew:{source_entry_id}",
        "platform": "homebrew",
        "targetPlatform": target_platform,
        "region": "INT",
        "languageCodes": [],
        "version": version,
        "packageSizeBytes": package_size,
        "requiredFirmware": "",
        "sourceId": "vitahomebrewdb",
        "sourceDataset": dataset,
        "sourceUpdatedAt": release_date,
        "sourceItemId": safe_id,
    }
    if title_id:
        release["titleId"] = title_id

    game: dict[str, Any] = {
        "gameId": game_id,
        "title": name,
        "sortTitle": name,
        "platforms": ["homebrew"],
        "summary": short_description,
        "description": long_description,
        "developer": author,
        "releaseYear": release_year,
        "contentKind": "homebrew",
        "homebrew": homebrew,
        "releases": [release],
    }

    # Explicitly guard against accidentally leaking binary/package or integrity fields.
    serialized_keys = {str(key).replace("_", "").casefold() for key in entry}
    if serialized_keys.intersection(FORBIDDEN_KEYS):
        # Source rows contain these keys by design; none are copied to the public record.
        pass

    return game


def merge_catalog(
    catalog: dict[str, Any],
    datasets: dict[str, list[dict[str, Any]]],
    fetched_at: str | None = None,
) -> dict[str, Any]:
    if catalog.get("schemaVersion") != "1.0.0":
        raise ValueError("Input catalog schemaVersion must be 1.0.0")
    games = catalog.get("games")
    if not isinstance(games, list):
        raise ValueError("Input catalog must contain a games array")

    existing_ids = {game.get("gameId") for game in games if isinstance(game, dict)}
    sources = list(catalog.get("sources", []))
    fetched_at = fetched_at or datetime.now(timezone.utc).replace(microsecond=0).isoformat().replace("+00:00", "Z")

    imported_total = 0
    for dataset, target_platform in DATASETS:
        entries = datasets.get(dataset, [])
        imported = 0
        for entry in entries:
            game = normalize_entry(entry, target_platform, dataset)
            if not game or game["gameId"] in existing_ids:
                continue
            games.append(game)
            existing_ids.add(game["gameId"])
            imported += 1
        imported_total += imported
        sources.append(
            {
                "sourceId": "vitahomebrewdb",
                "name": "VitaHomebrewDB",
                "dataset": dataset,
                "url": SOURCE_REPOSITORY,
                "fetchedAt": fetched_at,
                "recordCount": imported,
                "notes": (
                    "Descriptive metadata only. Direct package URLs, payloads, checksums and hashes are excluded. "
                    "Artwork is referenced from the upstream host; ownership remains with original creators. "
                    "Credit VitaHomebrewDB and the original homebrew authors."
                ),
            }
        )

    release_count = sum(
        len(game.get("releases", []))
        for game in games
        if isinstance(game, dict)
    )
    catalog["games"] = sorted(
        games,
        key=lambda game: (
            game.get("platforms", [""])[0],
            clean(game.get("title")).casefold(),
        ),
    )
    catalog["sources"] = sources
    catalog["generatedAt"] = fetched_at
    catalog["totals"] = {"games": len(catalog["games"]), "releases": release_count}
    catalog.setdefault("homebrewImport", {})["lastImported"] = fetched_at
    catalog["homebrewImport"]["recordsAdded"] = imported_total
    return catalog


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--catalog", type=Path, default=Path("web/data/catalog.json"))
    parser.add_argument("--output", type=Path, default=Path("web/data/catalog.json"))
    parser.add_argument("--timeout", type=int, default=60)
    args = parser.parse_args()

    catalog = json.loads(args.catalog.read_text(encoding="utf-8"))
    datasets: dict[str, list[dict[str, Any]]] = {}
    for dataset, _target in DATASETS:
        url = f"{SOURCE_RAW_BASE}/{dataset}"
        print(f"Fetching homebrew metadata: {dataset}", file=sys.stderr)
        datasets[dataset] = read_json(url, timeout=args.timeout)

    merged = merge_catalog(catalog, datasets)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(merged, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(
        f"Added {merged['homebrewImport']['recordsAdded']} homebrew entries; "
        f"catalog now has {merged['totals']['games']} games / {merged['totals']['releases']} releases.",
        file=sys.stderr,
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
