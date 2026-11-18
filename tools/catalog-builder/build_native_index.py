#!/usr/bin/env python3
"""Compile the shared JSON catalog into a compact, read-only native index."""

from __future__ import annotations

import hashlib
import json
import math
import sys
from pathlib import Path

from catalog_validation import validate_catalog

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


def age_guidance(game: dict) -> str:
    """Prefer PEGI in the European region; never invent a classification."""
    ratings = game.get("ageRatings", [])
    if not isinstance(ratings, list):
        return ""
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
        if descriptors:
            label += f" / {descriptors}"
        candidates.append((AGE_SYSTEM_PRIORITY[system], index, label))
    if not candidates:
        return ""
    return min(candidates)[2][:180]


def external_score_label(game: dict) -> str:
    """Preserve the supplied scale/provider/votes, without assuming a denominator."""
    scores = game.get("externalScores", [])
    if not isinstance(scores, list):
        return ""
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
            scale = 0
        provider = optional_text(score.get("provider"), 48)
        if not provider:
            continue
        votes = score.get("voteCount")
        if isinstance(votes, bool) or not isinstance(votes, int) or votes < 0:
            votes = 0
        label = f"{value:g}"
        if scale:
            label += f" / {scale:g}"
        label += f" - {provider}"
        if votes:
            label += f" ({min(votes, 4294967295)} votes)"
        return label[:140]
    return ""


def editorial_tag_line(game: dict) -> str:
    tags = []
    kind = optional_text(game.get("contentKind"), 24)
    if kind:
        tags.append(kind.replace("-", " ").upper())
    features = joined_values(game.get("features"), 120)
    themes = joined_values(game.get("themes"), 80)
    if features:
        tags.extend(part.strip() for part in features.split(" / ") if part.strip())
    if themes:
        tags.extend(part.strip() for part in themes.split(" / ") if part.strip())
    return " / ".join(tags)[:180]


def build(source: Path, destination: Path) -> int:
    source_bytes = source.read_bytes()
    catalog = validate_catalog(json.loads(source_bytes.decode("utf-8")))
    schema_version = catalog["schemaVersion"]
    generated_at = optional_text(catalog.get("generatedAt"), 64)
    source_sha256 = hashlib.sha256(source_bytes).hexdigest()
    games = catalog["games"]

    rows = []
    seen = set()
    for game in games:
        game_id = game["gameId"]
        title = game["title"]
        releases = game["releases"]
        if game_id in seen:
            raise ValueError(f"duplicate gameId: {game_id!r}")
        seen.add(game_id)
        mask = 0
        for release in releases:
            mask |= PLATFORM_BITS[release["platform"]]

        year = game.get("releaseYear")
        if not isinstance(year, int) or isinstance(year, bool) or not 1970 <= year <= 2100:
            year = 0
        rows.append((
            game_id, title.strip(),
            optional_text(game.get("summary"), 260),
            optional_text(game.get("developer"), 100),
            optional_text(game.get("publisher"), 100),
            joined_values(game.get("genres"), 120),
            editorial_tag_line(game), age_guidance(game),
            external_score_label(game), mask, len(releases), year
        ))

    destination.parent.mkdir(parents=True, exist_ok=True)
    with destination.open("w", encoding="utf-8", newline="\n") as out:
        out.write('#include "catalog_index.h"\n\n')
        out.write(f"const char vita_nexus_catalog_schema_version[] = {c_string(schema_version)};\n")
        out.write(f"const char vita_nexus_catalog_generated_at[] = {c_string(generated_at)};\n")
        out.write(f"const char vita_nexus_catalog_source_sha256[] = {c_string(source_sha256)};\n\n")
        out.write("const VitaNexusCatalogEntry vita_nexus_catalog[] = {\n")
        for game_id, title, summary, developer, publisher, genres, tags,
             age, score, mask, count, year in rows:
            out.write(
                f"    {{{c_string(game_id)}, {c_string(title)}, {c_string(summary)}, "
                f"{c_string(developer)}, {c_string(publisher)}, {c_string(genres)}, "
                f"{c_string(tags)}, {c_string(age)}, {c_string(score)}, "
                f"{mask}u, {count}u, {year}u}},\n"
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
