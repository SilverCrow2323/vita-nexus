#!/usr/bin/env python3
"""Import the canonical metadata-only catalog snapshot into the host SQLite schema.

This is a host-side importer and migration contract. The current VitaSDK app
continues to use its compiled read-only C index and checksummed personal stores.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import sqlite3
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from catalog_validation import validate_catalog  # noqa: E402
from migrate_native_db import apply_migrations  # noqa: E402

ROOT = Path(__file__).resolve().parents[2]
PLATFORM_BITS = {"psvita": 1, "psp": 2, "ps1": 4, "homebrew": 8}
REGIONS = {"EU", "US", "JP", "AS", "INT", "KR", "CN", "UNKNOWN"}
CONTENT_KINDS = {
    "official": "commercial",
    "homebrew": "homebrew",
    "homebrew-plugin": "homebrew",
    "homebrew-tool": "homebrew",
}


def _text(value: object, default: str = "") -> str:
    return value.strip() if isinstance(value, str) else default


def _required_text(value: object, label: str) -> str:
    result = _text(value)
    if not result:
        raise ValueError(f"{label} must be a non-empty string")
    return result


def _release_size(value: object, label: str) -> int | None:
    if value is None:
        return None
    if isinstance(value, bool) or not isinstance(value, int) or value < 0:
        raise ValueError(f"{label} must be a non-negative integer or null")
    return value


def import_catalog(
    catalog_path: str | Path,
    database_path: str | Path,
    migration_dir: str | Path | None = None,
) -> dict[str, object]:
    source_path = Path(catalog_path)
    source_bytes = source_path.read_bytes()
    catalog = validate_catalog(json.loads(source_bytes.decode("utf-8")))
    schema_version = _required_text(catalog.get("schemaVersion"), "catalog.schemaVersion")
    generated_at = _required_text(catalog.get("generatedAt"), "catalog.generatedAt")
    sources = catalog["sources"]
    games = catalog["games"]
    release_count = catalog["totals"]["releases"]
    snapshot_sha256 = hashlib.sha256(source_bytes).hexdigest()

    normalized_sources = []
    for index, source in enumerate(sources):
        prefix = f"catalog.sources[{index}]"
        source_id = _required_text(source.get("sourceId"), f"{prefix}.sourceId")
        dataset = _required_text(source.get("dataset"), f"{prefix}.dataset")
        name = _required_text(source.get("name"), f"{prefix}.name")
        url = _required_text(source.get("url"), f"{prefix}.url")
        record_count = source.get("recordCount")
        if isinstance(record_count, bool) or not isinstance(record_count, int) or record_count < 0:
            raise ValueError(f"{prefix}.recordCount must be a non-negative integer")
        normalized_sources.append((
            source_id, dataset, name, url, _text(source.get("fetchedAt")),
            record_count, _text(source.get("notes")), 1,
        ))

    normalized_games = []
    normalized_releases = []
    for game_index, game in enumerate(games):
        prefix = f"catalog.games[{game_index}]"
        game_id = _required_text(game.get("gameId"), f"{prefix}.gameId")
        title = _required_text(game.get("title"), f"{prefix}.title")
        content_kind = game.get("contentKind", "official")
        if content_kind not in CONTENT_KINDS:
            raise ValueError(f"{prefix}.contentKind is unsupported: {content_kind!r}")

        releases = game["releases"]
        platform_mask = 0
        for release_index, release in enumerate(releases):
            release_prefix = f"{prefix}.releases[{release_index}]"
            platform = release.get("platform")
            if platform not in PLATFORM_BITS:
                raise ValueError(f"{release_prefix}.platform is unsupported: {platform!r}")
            region = release.get("region")
            if region not in REGIONS:
                raise ValueError(f"{release_prefix}.region is unsupported: {region!r}")
            source_id = _required_text(release.get("sourceId"), f"{release_prefix}.sourceId")
            source_dataset = _required_text(
                release.get("sourceDataset"), f"{release_prefix}.sourceDataset"
            )
            release_id = _required_text(release.get("releaseId"), f"{release_prefix}.releaseId")
            language_codes = release.get("languageCodes", [])
            if not isinstance(language_codes, list) or any(not isinstance(code, str) for code in language_codes):
                raise ValueError(f"{release_prefix}.languageCodes must be an array of strings")
            platform_mask |= PLATFORM_BITS[platform]
            normalized_releases.append((
                release_id,
                game_id,
                platform,
                region,
                ",".join(code.strip() for code in language_codes if code.strip()),
                _text(release.get("titleId")),
                _text(release.get("contentId")),
                "",
                "",
                _text(release.get("version")),
                _release_size(
                    release.get("packageSizeBytes"),
                    f"{release_prefix}.packageSizeBytes",
                ),
                source_id,
                source_dataset,
                1,
            ))

        if not 1 <= platform_mask <= 15:
            raise ValueError(f"{prefix} has no supported release platform")
        release_year = game.get("releaseYear")
        if isinstance(release_year, bool) or not isinstance(release_year, int) or not 1970 <= release_year <= 2100:
            release_year = None
        normalized_games.append((
            game_id,
            title,
            _text(game.get("sortTitle"), title) or title,
            _text(game.get("summary")),
            _text(game.get("description")),
            _text(game.get("developer")),
            _text(game.get("publisher")),
            release_year,
            CONTENT_KINDS[content_kind],
            platform_mask,
            schema_version,
            1,
        ))

    migrations = Path(migration_dir) if migration_dir else ROOT / "native" / "db" / "migrations"
    apply_migrations(database_path, migrations)

    connection = sqlite3.connect(str(database_path))
    try:
        connection.execute("PRAGMA foreign_keys = ON")
        connection.execute("BEGIN IMMEDIATE")
        connection.execute("UPDATE catalog_sources SET is_in_snapshot = 0")
        connection.execute("UPDATE catalog_games SET is_in_snapshot = 0")
        connection.execute("UPDATE catalog_releases SET is_in_snapshot = 0")

        connection.executemany(
            """INSERT INTO catalog_sources
               (source_id, dataset, name, url, fetched_at, record_count, notes, is_in_snapshot)
               VALUES (?, ?, ?, ?, ?, ?, ?, ?)
               ON CONFLICT(source_id, dataset) DO UPDATE SET
                   name=excluded.name, url=excluded.url, fetched_at=excluded.fetched_at,
                   record_count=excluded.record_count, notes=excluded.notes,
                   is_in_snapshot=excluded.is_in_snapshot""",
            normalized_sources,
        )
        connection.executemany(
            """INSERT INTO catalog_games
               (game_id, title, sort_title, summary, description, developer, publisher,
                release_year, content_kind, platform_mask, source_schema_version, is_in_snapshot)
               VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
               ON CONFLICT(game_id) DO UPDATE SET
                   title=excluded.title, sort_title=excluded.sort_title,
                   summary=excluded.summary, description=excluded.description,
                   developer=excluded.developer, publisher=excluded.publisher,
                   release_year=excluded.release_year, content_kind=excluded.content_kind,
                   platform_mask=excluded.platform_mask,
                   source_schema_version=excluded.source_schema_version,
                   is_in_snapshot=excluded.is_in_snapshot""",
            normalized_games,
        )
        connection.executemany(
            """INSERT INTO catalog_releases
               (release_id, game_id, platform, region, language_code, title_id,
                content_id, disc_id, serial, version, package_size_bytes,
                source_id, source_dataset, is_in_snapshot)
               VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
               ON CONFLICT(release_id) DO UPDATE SET
                   game_id=excluded.game_id, platform=excluded.platform,
                   region=excluded.region, language_code=excluded.language_code,
                   title_id=excluded.title_id, content_id=excluded.content_id,
                   disc_id=excluded.disc_id, serial=excluded.serial,
                   version=excluded.version, package_size_bytes=excluded.package_size_bytes,
                   source_id=excluded.source_id, source_dataset=excluded.source_dataset,
                   is_in_snapshot=excluded.is_in_snapshot""",
            normalized_releases,
        )
        connection.execute(
            """INSERT INTO catalog_snapshot
               (singleton, schema_version, snapshot_sha256, generated_at, game_count, release_count)
               VALUES (1, ?, ?, ?, ?, ?)
               ON CONFLICT(singleton) DO UPDATE SET
                   schema_version=excluded.schema_version,
                   snapshot_sha256=excluded.snapshot_sha256,
                   generated_at=excluded.generated_at,
                   imported_at=strftime('%Y-%m-%dT%H:%M:%fZ', 'now'),
                   game_count=excluded.game_count,
                   release_count=excluded.release_count""",
            (schema_version, snapshot_sha256, generated_at, len(games), release_count),
        )
        connection.commit()
    except Exception:
        connection.rollback()
        raise
    finally:
        connection.close()

    return {
        "schema_version": schema_version,
        "games": len(games),
        "releases": release_count,
        "sha256": snapshot_sha256,
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--catalog", default=str(ROOT / "web/data/catalog.json"))
    parser.add_argument("--database", required=True)
    parser.add_argument("--migrations", default=str(ROOT / "native/db/migrations"))
    args = parser.parse_args()

    result = import_catalog(args.catalog, args.database, args.migrations)
    print(
        "Imported catalog schema {schema_version}: {games} games / {releases} releases "
        "(SHA-256 {sha256})".format(**result)
    )
    print("Personal profile and session tables were left intact.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
