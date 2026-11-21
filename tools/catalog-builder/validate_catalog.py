#!/usr/bin/env python3
"""Validate the public VITA NEXUS catalog snapshot."""

from __future__ import annotations

import json
import re
import sys
from pathlib import Path

FORBIDDEN_KEYS = {
    "downloadurl", "downloaduri", "binaryurl", "packageurl", "pkgurl", "data", "hash", "hash2",
    "sha256", "md5", "zrif", "licensekey", "digest"
}
ALLOWED_PLATFORMS = {"psvita", "psp", "ps1", "homebrew"}
ALLOWED_REGIONS = {"EU", "US", "JP", "AS", "INT", "KR", "CN", "UNKNOWN"}


def fail(message: str) -> None:
    print("CATALOG VALIDATION ERROR: " + message, file=sys.stderr)
    raise SystemExit(1)


def inspect_forbidden(value: object, location: str = "root") -> None:
    if isinstance(value, dict):
        for key, child in value.items():
            normalized = re.sub(r"[^a-z0-9]", "", str(key).casefold())
            if normalized in FORBIDDEN_KEYS:
                fail(f"forbidden package, payload or integrity field at {location}.{key}")
            inspect_forbidden(child, f"{location}.{key}")
    elif isinstance(value, list):
        for index, child in enumerate(value):
            inspect_forbidden(child, f"{location}[{index}]")


def main() -> int:
    path = Path(sys.argv[1]) if len(sys.argv) > 1 else Path("web/data/catalog.json")
    try:
        catalog = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        fail(f"cannot read valid JSON from {path}: {error}")

    if not isinstance(catalog, dict):
        fail("catalog root must be an object")

    if catalog.get("schemaVersion") != "1.0.0":
        fail("unsupported or missing schemaVersion")
    if not isinstance(catalog.get("games"), list):
        fail("games must be an array")

    inspect_forbidden(catalog)
    seen_game_ids: set[str] = set()
    seen_release_ids: set[str] = set()
    release_count = 0

    for game in catalog["games"]:
        if not isinstance(game, dict):
            fail("every game must be an object")
        game_id = game.get("gameId")
        if not isinstance(game_id, str) or not game_id:
            fail("every game needs a non-empty gameId")
        if game_id in seen_game_ids:
            fail(f"duplicate gameId: {game_id}")
        seen_game_ids.add(game_id)
        if not isinstance(game.get("title"), str) or not game["title"].strip():
            fail(f"game {game_id} has no title")

        releases = game.get("releases")
        if not isinstance(releases, list) or not releases:
            fail(f"game {game_id} must have at least one release")

        for item in releases:
            if not isinstance(item, dict):
                fail(f"game {game_id} contains a non-object release")
            release_count += 1
            release_id = item.get("releaseId")
            if not isinstance(release_id, str) or not release_id:
                fail(f"game {game_id} has a release without releaseId")
            if release_id in seen_release_ids:
                fail(f"duplicate releaseId: {release_id}")
            seen_release_ids.add(release_id)
            if item.get("platform") not in ALLOWED_PLATFORMS:
                fail(f"unsupported platform in {release_id}")
            if item.get("region") not in ALLOWED_REGIONS:
                fail(f"unsupported region in {release_id}")
            for required in ("sourceId", "sourceDataset"):
                if not isinstance(item.get(required), str) or not item[required]:
                    fail(f"release {release_id} is missing {required}")
            if item.get("platform") == "homebrew":
                if item.get("targetPlatform") not in {"psvita", "psp"}:
                    fail(f"homebrew release {release_id} is missing targetPlatform")
                metadata = game.get("homebrew", {})
                if not metadata.get("sourceCatalogUrl"):
                    fail(f"homebrew game {game_id} is missing source attribution")

    totals = catalog.get("totals", {})
    if totals.get("games") != len(catalog["games"]):
        fail("totals.games does not match the games array")
    if totals.get("releases") != release_count:
        fail("totals.releases does not match the release count")

    print(f"Catalog valid: {len(catalog['games'])} games / {release_count} releases")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
