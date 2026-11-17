#!/usr/bin/env python3
"""Compile the shared JSON catalog into a compact, read-only native index."""

from __future__ import annotations

import json
import math
import sys
from pathlib import Path

PLATFORM_BITS = {"psvita": 1, "psp": 2, "ps1": 4, "homebrew": 8}
AGE_SYSTEM_PRIORITY = {"PEGI": 0, "ESRB": 1, "CERO": 2, "USK": 3, "ACB": 4, "OTHER": 5}


def c_string(value: str) -> str:
    return json.dumps(value, ensure_ascii=False)


def optional_text(value: object, limit: int) -> str:
    return value.strip()[:limit] if isinstance(value, str) else ""


def joined_values(value: object, limit: int) -> str:
    if not isinstance(value, list):
        return ""
    parts = [item.strip() for item in value if isinstance(item, str) and item.strip()]
    return " / ".join(parts)[:limit]


def age_metadata(game: dict) -> tuple[str, str, str]:
    """Prefer PEGI for the European handheld audience; retain honest source data."""
    ratings = game.get("ageRatings", [])
    if not isinstance(ratings, list):
        return "", "", ""
    candidates = []
    for index, rating in enumerate(ratings):
        if not isinstance(rating, dict):
            continue
        system = optional_text(rating.get("system"), 12).upper()
        value = optional_text(rating.get("value"), 20)
        if system not in AGE_SYSTEM_PRIORITY or not value:
            continue
        territory = optional_text(rating.get("territory"), 24)
        label = f"{system} {value}" + (f" ({territory})" if territory else "")
        descriptors = joined_values(rating.get("descriptors"), 100)
        source = optional_text(rating.get("source"), 120)
        candidates.append((AGE_SYSTEM_PRIORITY[system], index, label, descriptors, source))
    if not candidates:
        return "", "", ""
    _, _, label, descriptors, source = min(candidates)
    return label[:64], descriptors, source


def score_metadata(game: dict) -> tuple[str, str, float, float, int, int]:
    """Use the first valid catalog score; never invent a scale or a vote count."""
    scores = game.get("externalScores", [])
    if not isinstance(scores, list):
        return "", "", 0.0, 0.0, 0, 0
    for score in scores:
        if not isinstance(score, dict):
            continue
        value = score.get("value")
        if isinstance(value, bool) or not isinstance(value, (int, float)) or not math.isfinite(value):
            continue
        if value < 0 or value > 10000:
            continue
        scale = score.get("scale")
        if isinstance(scale, bool) or not isinstance(scale, (int, float)) or not math.isfinite(scale) or scale <= 0 or scale > 10000:
            scale = 0.0
        provider = optional_text(score.get("provider"), 48)
        source = optional_text(score.get("source"), 120)
        if not provider:
            continue
        votes = score.get("voteCount")
        if isinstance(votes, bool) or not isinstance(votes, int) or votes < 0:
            votes = 0
        votes = min(votes, 4294967295)
        return provider, source, float(value), float(scale), votes, 1
    return "", "", 0.0, 0.0, 0, 0


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

        year = game.get("releaseYear")
        if not isinstance(year, int) or isinstance(year, bool) or not 1970 <= year <= 2100:
            year = 0
        genres = joined_values(game.get("genres"), 120)
        content_kind = optional_text(game.get("contentKind"), 24)
        features = joined_values(game.get("features"), 140)
        themes = joined_values(game.get("themes"), 100)
        age_rating, age_descriptors, age_source = age_metadata(game)
        score_provider, score_source, score_value, score_scale, score_votes, has_score = score_metadata(game)
        rows.append((
            game_id, title.strip(), optional_text(game.get("summary"), 260),
            optional_text(game.get("developer"), 100), optional_text(game.get("publisher"), 100),
            genres, content_kind, features, themes, age_rating, age_descriptors, age_source,
            score_provider, score_source, mask, len(releases), year,
            score_value, score_scale, score_votes, has_score
        ))

    destination.parent.mkdir(parents=True, exist_ok=True)
    with destination.open("w", encoding="utf-8", newline="\n") as out:
        out.write('#include "catalog_index.h"\n\n')
        out.write("const VitaNexusCatalogEntry vita_nexus_catalog[] = {\n")
        for row in rows:
            (game_id, title, summary, developer, publisher, genres, content_kind,
             features, themes, age_rating, age_descriptors, age_source,
             score_provider, score_source, mask, count, year,
             score_value, score_scale, score_votes, has_score) = row
            out.write(
                f"    {{{c_string(game_id)}, {c_string(title)}, {c_string(summary)}, "
                f"{c_string(developer)}, {c_string(publisher)}, {c_string(genres)}, "
                f"{c_string(content_kind)}, {c_string(features)}, {c_string(themes)}, "
                f"{c_string(age_rating)}, {c_string(age_descriptors)}, {c_string(age_source)}, "
                f"{c_string(score_provider)}, {c_string(score_source)}, "
                f"{mask}u, {count}u, {year}u, {score_value:.3f}f, {score_scale:.3f}f, "
                f"{score_votes}u, {has_score}u}},\n"
            )
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
