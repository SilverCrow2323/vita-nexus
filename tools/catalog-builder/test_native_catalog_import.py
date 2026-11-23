from __future__ import annotations

import hashlib
import json
import sqlite3
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(Path(__file__).resolve().parent))
from import_catalog_sqlite import import_catalog  # noqa: E402

MIGRATIONS = ROOT / "native" / "db" / "migrations"


def make_catalog(game_specs: list[tuple[str, str, str]], generated_at: str) -> dict:
    games = []
    for game_id, title, platform in game_specs:
        games.append({
            "gameId": game_id,
            "title": title,
            "sortTitle": title,
            "platforms": [platform],
            "summary": f"{title} summary",
            "description": f"{title} description",
            "developer": "SPDW Test Studio",
            "publisher": "SPDW Test",
            "releaseYear": 2005,
            "contentKind": "homebrew" if platform == "homebrew" else "official",
            "releases": [{
                "releaseId": f"release-{game_id}",
                "platform": platform,
                "region": "EU",
                "titleId": f"PCSE{len(game_id):05d}" if platform != "homebrew" else "",
                "contentId": None,
                "languageCodes": ["en", "it"],
                "version": "1.00",
                "packageSizeBytes": 4096,
                "sourceId": "fixture",
                "sourceDataset": "metadata",
                "sourceUpdatedAt": generated_at,
            }],
        })
    return {
        "schemaVersion": "1.0.0",
        "generatedAt": generated_at,
        "sources": [{
            "sourceId": "fixture",
            "name": "Fixture Metadata",
            "dataset": "metadata",
            "url": "https://example.invalid/catalog",
            "fetchedAt": generated_at,
            "recordCount": len(games),
            "notes": "Deterministic unit-test fixture",
        }],
        "games": games,
        "totals": {
            "games": len(games),
            "releases": len(games),
        },
    }


class NativeCatalogImportTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temporary = tempfile.TemporaryDirectory()
        self.root = Path(self.temporary.name)
        self.database = self.root / "vita-nexus.db"
        self.catalog = self.root / "catalog.json"

    def tearDown(self) -> None:
        self.temporary.cleanup()

    def write_catalog(self, value: dict) -> bytes:
        payload = (json.dumps(value, sort_keys=True, separators=(",", ":")) + "\n").encode()
        self.catalog.write_bytes(payload)
        return payload

    def connect(self) -> sqlite3.Connection:
        connection = sqlite3.connect(self.database)
        connection.execute("PRAGMA foreign_keys = ON")
        return connection

    def test_import_records_snapshot_source_provenance_and_is_idempotent(self) -> None:
        payload = self.write_catalog(make_catalog([
            ("vita-game", "Vita Game", "psvita"),
            ("homebrew-app", "Homebrew App", "homebrew"),
        ], "2026-11-23T00:00:00Z"))

        first = import_catalog(self.catalog, self.database, MIGRATIONS)
        second = import_catalog(self.catalog, self.database, MIGRATIONS)
        expected_sha = hashlib.sha256(payload).hexdigest()
        self.assertEqual(first, second)
        self.assertEqual(first["games"], 2)
        self.assertEqual(first["releases"], 2)
        self.assertEqual(first["sha256"], expected_sha)

        with self.connect() as connection:
            self.assertEqual(connection.execute("PRAGMA user_version").fetchone()[0], 2)
            self.assertEqual(
                connection.execute(
                    "SELECT schema_version, snapshot_sha256, game_count, release_count "
                    "FROM catalog_snapshot WHERE singleton=1"
                ).fetchone(),
                ("1.0.0", expected_sha, 2, 2),
            )
            self.assertEqual(
                connection.execute(
                    "SELECT source_id, dataset, record_count, is_in_snapshot FROM catalog_sources"
                ).fetchall(),
                [("fixture", "metadata", 2, 1)],
            )
            rows = connection.execute(
                "SELECT game_id, content_kind, platform_mask, is_in_snapshot "
                "FROM catalog_games ORDER BY game_id"
            ).fetchall()
            self.assertEqual(rows, [
                ("homebrew-app", "homebrew", 8, 1),
                ("vita-game", "commercial", 1, 1),
            ])
            release = connection.execute(
                "SELECT language_code, region, package_size_bytes, is_in_snapshot "
                "FROM catalog_releases WHERE release_id='release-vita-game'"
            ).fetchone()
            self.assertEqual(release, ("en,it", "EU", 4096, 1))

    def test_removed_catalog_rows_are_retained_for_user_owned_history(self) -> None:
        first = make_catalog([
            ("keep-game", "Keep Game", "psvita"),
            ("saved-game", "Saved Game", "psp"),
        ], "2026-11-23T00:00:00Z")
        self.write_catalog(first)
        import_catalog(self.catalog, self.database, MIGRATIONS)

        with self.connect() as connection:
            connection.execute(
                """INSERT INTO profiles
                   (profile_id, display_name, is_active, created_at, updated_at)
                   VALUES ('main', 'Main profile', 1, 'now', 'now')"""
            )
            connection.execute(
                """INSERT INTO profile_games
                   (profile_id, game_id, flags, rating, updated_at)
                   VALUES ('main', 'saved-game', 1, 8, 'now')"""
            )
            connection.execute(
                """INSERT INTO play_sessions
                   (session_id, profile_id, game_id, platform, started_at,
                    identification_confidence)
                   VALUES ('session-001', 'main', 'saved-game', 'psp', 'now', 'high')"""
            )

        self.write_catalog(make_catalog([
            ("keep-game", "Keep Game Updated", "psvita"),
        ], "2026-11-23T01:00:00Z"))
        import_catalog(self.catalog, self.database, MIGRATIONS)

        with self.connect() as connection:
            self.assertEqual(
                connection.execute(
                    "SELECT title, is_in_snapshot FROM catalog_games WHERE game_id='keep-game'"
                ).fetchone(),
                ("Keep Game Updated", 1),
            )
            self.assertEqual(
                connection.execute(
                    "SELECT is_in_snapshot FROM catalog_games WHERE game_id='saved-game'"
                ).fetchone(),
                (0,),
            )
            self.assertEqual(
                connection.execute(
                    "SELECT is_in_snapshot FROM catalog_releases WHERE release_id='release-saved-game'"
                ).fetchone(),
                (0,),
            )
            self.assertEqual(
                connection.execute("SELECT COUNT(*) FROM profile_games").fetchone()[0], 1
            )
            self.assertEqual(
                connection.execute("SELECT COUNT(*) FROM play_sessions").fetchone()[0], 1
            )
            self.assertEqual(
                connection.execute(
                    "SELECT game_count, release_count FROM catalog_snapshot WHERE singleton=1"
                ).fetchone(),
                (1, 1),
            )

    def test_invalid_snapshot_does_not_replace_the_last_import(self) -> None:
        self.write_catalog(make_catalog([
            ("stable-game", "Stable Game", "psvita"),
        ], "2026-11-23T00:00:00Z"))
        import_catalog(self.catalog, self.database, MIGRATIONS)

        invalid = make_catalog([
            ("other-game", "Other Game", "psp"),
        ], "2026-11-23T01:00:00Z")
        invalid["totals"]["games"] = 99
        self.write_catalog(invalid)
        with self.assertRaisesRegex(ValueError, "catalog.totals.games"):
            import_catalog(self.catalog, self.database, MIGRATIONS)

        with self.connect() as connection:
            self.assertEqual(
                connection.execute(
                    "SELECT game_id, is_in_snapshot FROM catalog_games WHERE is_in_snapshot=1"
                ).fetchall(),
                [("stable-game", 1)],
            )
            self.assertEqual(
                connection.execute(
                    "SELECT game_count FROM catalog_snapshot WHERE singleton=1"
                ).fetchone(),
                (1,),
            )


if __name__ == "__main__":
    unittest.main()
