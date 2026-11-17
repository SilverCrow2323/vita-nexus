#!/usr/bin/env python3
"""Compile the shared JSON catalog into a compact, read-only native index."""

from __future__ import annotations

import json
import sys
from pathlib import Path

PLATFORM_BITS = {"psvita": 1, "psp": 2, "ps1": 4, "homebrew": 8}


def c_string(value: str) -> str:
    # JSON string escaping is valid for the quote/backslash/control subset used here.
    return json.dumps(value, ensure_ascii=False)


def build(source: Path, destination: Path) -> int:
    catalog = json.loads(source.read_text(encoding="utf-8"))
    if catalog.get("schemaVersion") != "1.0.0":
        raise ValueError("unsupported catalog schema; expected 1.0.0")
    games = catalog.get("games")
    if not isinstance(games, list):
        raise ValueError("catalog.games must be an array")

    rows = []
    seen = set()
    for game in games:
        game_id = game.get("gameId")
        title = game.get("title")
        releases = game.get("releases")
        if not isinstance(game_id, str) or not game_id or game_id in seen:
            raise ValueError(f"missing or duplicate gameId: {game_id!r}")
        if not isinstance(title, str) or not title.strip():
            raise ValueError(f"game {game_id} has no title")
        if not isinstance(releases, list) or not releases:
            raise ValueError(f"game {game_id} has no releases")
        seen.add(game_id)
        mask = 0
        for release in releases:
            platform = release.get("platform")
            if platform not in PLATFORM_BITS:
                raise ValueError(f"unsupported platform {platform!r} in {game_id}")
            mask |= PLATFORM_BITS[platform]
        if len(releases) > 65535:
            raise ValueError(f"too many releases for {game_id}")
        rows.append((game_id, title, mask, len(releases)))

    destination.parent.mkdir(parents=True, exist_ok=True)
    with destination.open("w", encoding="utf-8", newline="\n") as out:
        out.write('#include "catalog_index.h"\n\n')
        out.write("const VitaNexusCatalogEntry vita_nexus_catalog[] = {\n")
        for game_id, title, mask, count in rows:
            out.write(f"    {{{c_string(game_id)}, {c_string(title)}, {mask}u, {count}u}},\n")
        out.write("};\n\n")
        out.write("const size_t vita_nexus_catalog_count =\n")
        out.write("    sizeof(vita_nexus_catalog) / sizeof(vita_nexus_catalog[0]);\n")
    return len(rows)


def main() -> int:
    source = Path(sys.argv[1]) if len(sys.argv) > 1 else Path("web/data/catalog.json")
    destination = Path(sys.argv[2]) if len(sys.argv) > 2 else Path("native/generated/catalog_index.c")
    count = build(source, destination)
    print(f"Native catalog index generated: {count} games -> {destination}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
