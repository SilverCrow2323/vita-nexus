from __future__ import annotations

import sqlite3
import tempfile
import unittest
from pathlib import Path

from migrate_native_db import apply_migrations

ROOT = Path(__file__).resolve().parents[2]
MIGRATIONS = ROOT / "native" / "db" / "migrations"


class NativeDatabaseMigrationTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temporary = tempfile.TemporaryDirectory()
        self.database = Path(self.temporary.name) / "vita-nexus.db"

    def tearDown(self) -> None:
        self.temporary.cleanup()

    def connect(self) -> sqlite3.Connection:
        connection = sqlite3.connect(self.database)
        connection.execute("PRAGMA foreign_keys = ON")
        return connection

    def migrate(self) -> tuple[int, list[int]]:
        return apply_migrations(self.database, MIGRATIONS)

    def seed_catalog(self, connection: sqlite3.Connection) -> None:
        connection.execute(
            """INSERT INTO catalog_games
               (game_id, title, content_kind, platform_mask, source_schema_version)
               VALUES ('game-001', 'Test game', 'commercial', 1, '1.0.0')"""
        )
        connection.execute(
            """INSERT INTO catalog_releases
               (release_id, game_id, platform, region, source_id, source_dataset)
               VALUES ('release-001', 'game-001', 'psvita', 'EU', 'nopaystation', 'PSV_GAMES')"""
        )

    def seed_profile(self, connection: sqlite3.Connection) -> None:
        connection.execute(
            """INSERT INTO profiles(profile_id, display_name, is_active, created_at, updated_at)
               VALUES ('main', 'Main profile', 1, '2026-11-21T00:00:00Z', '2026-11-21T00:00:00Z')"""
        )

    def test_initial_migration_creates_versioned_tables_and_indexes(self) -> None:
        version, applied = self.migrate()
        self.assertEqual(version, 1)
        self.assertEqual(applied, [1])

        with self.connect() as connection:
            self.assertEqual(connection.execute("PRAGMA user_version").fetchone()[0], 1)
            tables = {
                row[0]
                for row in connection.execute(
                    "SELECT name FROM sqlite_master WHERE type='table'"
                )
            }
            self.assertTrue({
                "schema_migrations", "catalog_snapshot", "catalog_games",
                "catalog_releases", "profiles", "profile_games",
                "play_sessions", "session_events",
            }.issubset(tables))
            indexes = {
                row[0]
                for row in connection.execute(
                    "SELECT name FROM sqlite_master WHERE type='index'"
                )
            }
            self.assertIn("profiles_single_active_idx", indexes)
            self.assertIn("catalog_releases_platform_region_idx", indexes)
            self.assertEqual(
                connection.execute(
                    "SELECT version, name FROM schema_migrations"
                ).fetchall(),
                [(1, "0001_catalog_and_personal_state")],
            )

    def test_migration_is_idempotent_and_does_not_rewrite_applied_history(self) -> None:
        self.assertEqual(self.migrate(), (1, [1]))
        self.assertEqual(self.migrate(), (1, []))
        with self.connect() as connection:
            self.assertEqual(
                connection.execute("SELECT COUNT(*) FROM schema_migrations").fetchone()[0],
                1,
            )

    def test_foreign_keys_profile_flags_and_session_lifecycle_are_enforced(self) -> None:
        self.migrate()
        with self.connect() as connection:
            self.seed_catalog(connection)
            self.seed_profile(connection)
            connection.execute(
                """INSERT INTO profile_games
                   (profile_id, game_id, flags, rating, updated_at)
                   VALUES ('main', 'game-001', 3, 8, '2026-11-21T00:00:00Z')"""
            )
            connection.execute(
                """INSERT INTO play_sessions
                   (session_id, profile_id, game_id, platform, started_at,
                    identification_confidence)
                   VALUES ('session-001', 'main', 'game-001', 'psvita',
                           '2026-11-21T00:00:00Z', 'medium')"""
            )
            connection.execute(
                """INSERT INTO session_events
                   (event_id, session_id, profile_id, game_id, platform,
                    event_type, occurred_at, source, identification_confidence)
                   VALUES ('event-001', 'session-001', 'main', 'game-001',
                           'psvita', 'start', '2026-11-21T00:00:00Z',
                           'native-vita', 'medium')"""
            )
            self.assertEqual(
                connection.execute("PRAGMA foreign_key_check").fetchall(), []
            )
            with self.assertRaises(sqlite3.IntegrityError):
                connection.execute(
                    """INSERT INTO profile_games
                       (profile_id, game_id, flags, rating, updated_at)
                       VALUES ('missing', 'game-001', 1, 0, 'now')"""
                )
            with self.assertRaises(sqlite3.IntegrityError):
                connection.execute(
                    """INSERT INTO profile_games
                       (profile_id, game_id, flags, rating, updated_at)
                       VALUES ('main', 'game-001', 0, 0, 'now')"""
                )
            with self.assertRaises(sqlite3.IntegrityError):
                connection.execute(
                    """INSERT INTO profile_games
                       (profile_id, game_id, flags, rating, updated_at)
                       VALUES ('main', 'game-001', 8, 0, 'now')"""
                )
            with self.assertRaises(sqlite3.IntegrityError):
                connection.execute(
                    """INSERT INTO play_sessions
                       (session_id, profile_id, game_id, platform, started_at,
                        ended_at, duration_seconds, end_reason, identification_confidence)
                       VALUES ('bad-session', 'main', 'game-001', 'psvita',
                               '2026-11-21T01:00:00Z', '2026-11-21T00:00:00Z',
                               0, 'exit', 'high')"""
                )

    def test_snapshot_release_and_profile_constraints_reject_invalid_rows(self) -> None:
        self.migrate()
        with self.connect() as connection:
            with self.assertRaises(sqlite3.IntegrityError):
                connection.execute(
                    """INSERT INTO catalog_snapshot
                       (singleton, schema_version, snapshot_sha256, generated_at,
                        game_count, release_count)
                       VALUES (1, '1.0.0', 'not-a-digest', '2026-11-21T00:00:00Z', 0, 0)"""
                )
            with self.assertRaises(sqlite3.IntegrityError):
                connection.execute(
                    """INSERT INTO catalog_games
                       (game_id, title, content_kind, platform_mask, source_schema_version)
                       VALUES ('bad-game', 'Bad game', 'commercial', 0, '1.0.0')"""
                )
            self.seed_catalog(connection)
            with self.assertRaises(sqlite3.IntegrityError):
                connection.execute(
                    """INSERT INTO catalog_releases
                       (release_id, game_id, platform, package_size_bytes, source_id, source_dataset)
                       VALUES ('bad-release', 'game-001', 'psvita', -1, 'nopaystation', 'PSV_GAMES')"""
                )
            self.seed_profile(connection)
            with self.assertRaises(sqlite3.IntegrityError):
                connection.execute(
                    """INSERT INTO profiles(profile_id, display_name, is_active, created_at, updated_at)
                       VALUES ('second', 'Second profile', 1, 'now', 'now')"""
                )

    def test_profile_delete_cascades_only_user_owned_rows(self) -> None:
        self.migrate()
        with self.connect() as connection:
            self.seed_catalog(connection)
            self.seed_profile(connection)
            connection.execute(
                """INSERT INTO profile_games
                   (profile_id, game_id, flags, rating, updated_at)
                   VALUES ('main', 'game-001', 1, 0, 'now')"""
            )
            connection.execute(
                """INSERT INTO play_sessions
                   (session_id, profile_id, game_id, platform, started_at,
                    identification_confidence)
                   VALUES ('session-001', 'main', 'game-001', 'psvita', 'now', 'high')"""
            )
            connection.execute("DELETE FROM profiles WHERE profile_id='main'")
            self.assertEqual(connection.execute("SELECT COUNT(*) FROM profile_games").fetchone()[0], 0)
            self.assertEqual(connection.execute("SELECT COUNT(*) FROM play_sessions").fetchone()[0], 0)
            self.assertEqual(connection.execute("SELECT COUNT(*) FROM catalog_games").fetchone()[0], 1)

    def test_failed_migration_rolls_back_schema_and_version(self) -> None:
        migration_dir = Path(self.temporary.name) / "broken-migrations"
        migration_dir.mkdir()
        (migration_dir / "0001_broken.sql").write_text(
            "CREATE TABLE should_rollback (value TEXT);\nTHIS IS NOT VALID;\n",
            encoding="utf-8",
        )
        with self.assertRaises(sqlite3.OperationalError):
            apply_migrations(self.database, migration_dir)
        with self.connect() as connection:
            self.assertEqual(connection.execute("PRAGMA user_version").fetchone()[0], 0)
            table = connection.execute(
                "SELECT 1 FROM sqlite_master WHERE type='table' AND name='should_rollback'"
            ).fetchone()
            self.assertIsNone(table)

    def test_newer_database_and_non_contiguous_migrations_fail_closed(self) -> None:
        with self.connect() as connection:
            connection.execute("PRAGMA user_version = 9")
        with self.assertRaisesRegex(ValueError, "newer than available migrations"):
            apply_migrations(self.database, MIGRATIONS)

        another_db = Path(self.temporary.name) / "gap.db"
        migration_dir = Path(self.temporary.name) / "gap-migrations"
        migration_dir.mkdir()
        (migration_dir / "0002_missing.sql").write_text(
            "CREATE TABLE ignored (value TEXT);\n", encoding="utf-8"
        )
        with self.assertRaisesRegex(ValueError, "contiguous and start at 0001"):
            apply_migrations(another_db, migration_dir)


if __name__ == "__main__":
    unittest.main()
