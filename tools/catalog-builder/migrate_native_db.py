#!/usr/bin/env python3
"""Apply immutable, numbered VITA NEXUS SQLite migrations transactionally.

This host-side tool establishes and tests the schema contract. It does not imply
that the current VitaSDK runtime links SQLite or opens this database yet.
"""

from __future__ import annotations

import argparse
import re
import sqlite3
from pathlib import Path

MIGRATION_NAME = re.compile(r"^(\d{4})_([a-z0-9_]+)\.sql$")


def discover_migrations(directory: Path) -> list[tuple[int, Path]]:
    if not directory.is_dir():
        raise ValueError(f"migration directory does not exist: {directory}")
    migrations: list[tuple[int, Path]] = []
    seen: set[int] = set()
    for path in sorted(directory.iterdir()):
        if not path.is_file():
            continue
        match = MIGRATION_NAME.fullmatch(path.name)
        if not match:
            raise ValueError(f"invalid migration filename: {path.name}")
        version = int(match.group(1))
        if version in seen:
            raise ValueError(f"duplicate migration version: {version}")
        seen.add(version)
        migrations.append((version, path))
    if not migrations:
        raise ValueError("no numbered SQL migrations were found")
    return migrations


def sql_statements(script: str):
    pending = ""
    for line in script.splitlines(keepends=True):
        pending += line
        if sqlite3.complete_statement(pending):
            statement = pending.strip()
            pending = ""
            if statement:
                yield statement
    if pending.strip():
        raise ValueError("migration ends with an incomplete SQL statement")


def apply_migrations(database: str | Path, migration_dir: str | Path) -> tuple[int, list[int]]:
    migrations = discover_migrations(Path(migration_dir))
    versions = [version for version, _path in migrations]
    if versions != list(range(1, versions[-1] + 1)):
        raise ValueError("migration versions must be contiguous and start at 0001")

    connection = sqlite3.connect(str(database))
    applied: list[int] = []
    try:
        connection.execute("PRAGMA foreign_keys = ON")
        current = int(connection.execute("PRAGMA user_version").fetchone()[0])
        if current > versions[-1]:
            raise ValueError(
                f"database schema {current} is newer than available migrations {versions[-1]}"
            )

        for version, path in migrations:
            if version <= current:
                continue
            if version != current + 1:
                raise ValueError(f"cannot apply migration {version} after schema {current}")

            script = path.read_text(encoding="utf-8")
            connection.execute("BEGIN IMMEDIATE")
            try:
                for statement in sql_statements(script):
                    connection.execute(statement)
                connection.execute(
                    "INSERT INTO schema_migrations(version, name) VALUES (?, ?)",
                    (version, path.stem),
                )
                connection.execute(f"PRAGMA user_version = {version}")
                connection.commit()
            except Exception:
                connection.rollback()
                raise
            current = version
            applied.append(version)

        return current, applied
    finally:
        connection.close()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--database", required=True, help="SQLite database path to create/update")
    parser.add_argument(
        "--migrations",
        default=str(Path(__file__).resolve().parents[2] / "native/db/migrations"),
        help="directory containing numbered SQL migrations",
    )
    args = parser.parse_args()

    version, applied = apply_migrations(args.database, args.migrations)
    if applied:
        print("Applied schema migrations: " + ", ".join(str(item) for item in applied))
    else:
        print("Database already matches the latest schema.")
    print(f"VITA NEXUS SQLite schema version: {version}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
