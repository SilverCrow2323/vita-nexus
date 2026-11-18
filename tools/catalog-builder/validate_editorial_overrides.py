#!/usr/bin/env python3
"""Validate curated VITA NEXUS editorial overrides and their provenance."""

from __future__ import annotations

import json
import math
import sys
from datetime import datetime
from pathlib import Path
from typing import Any


def _require(condition: bool, message: str) -> None:
    if not condition:
        raise ValueError(message)


def _text(value: Any, label: str, maximum: int, required: bool = True) -> None:
    if value is None and not required:
        return
    _require(isinstance(value, str), f"{label} must be a string")
    _require(bool(value.strip()), f"{label} must not be empty")
    _require(len(value) <= maximum, f"{label} exceeds {maximum} characters")


def _text_array(value: Any, label: str, maximum_items: int, maximum_text: int) -> None:
    if value is None:
        return
    _require(isinstance(value, list), f"{label} must be an array")
    _require(len(value) <= maximum_items, f"{label} has more than {maximum_items} entries")
    for index, item in enumerate(value):
        _text(item, f"{label}[{index}]", maximum_text)


def _https_url(value: Any, label: str) -> None:
    _text(value, label, 2048)
    _require(value.startswith("https://"), f"{label} must use HTTPS")


def validate_data(data: Any) -> int:
    """Validate an already parsed overrides document; return the number of records."""
    _require(isinstance(data, dict), "root must be an object")
    _require(data.get("schemaVersion") == "1.0.0", "unsupported or missing schemaVersion")
    _text(data.get("description"), "description", 500)
    games = data.get("games")
    _require(isinstance(games, dict), "games must be an object")

    for game_id, record in games.items():
        _text(game_id, "gameId", 200)
        _require(isinstance(record, dict), f"game {game_id} override must be an object")
        editorial = record.get("editorial")
        if editorial is None:
            continue
        _require(isinstance(editorial, dict), f"game {game_id} editorial must be an object")
        allowed = {"rubric", "standfirst", "review", "highlights", "caveats", "curiosities", "credit"}
        unknown = set(editorial) - allowed
        _require(not unknown, f"game {game_id} editorial has unsupported fields: {', '.join(sorted(unknown))}")
        _require(bool(editorial), f"game {game_id} editorial block is empty")

        if "rubric" in editorial:
            _text(editorial["rubric"], f"game {game_id} rubric", 40)
        if "standfirst" in editorial:
            _text(editorial["standfirst"], f"game {game_id} standfirst", 260)
        _text_array(editorial.get("highlights"), f"game {game_id} highlights", 4, 90)
        _text_array(editorial.get("caveats"), f"game {game_id} caveats", 3, 90)
        _text_array(editorial.get("curiosities"), f"game {game_id} curiosities", 4, 180)

        review = editorial.get("review")
        if review is not None:
            _require(isinstance(review, dict), f"game {game_id} review must be an object")
            _require(set(review) <= {"headline", "body", "verdict", "score"},
                     f"game {game_id} review contains unsupported fields")
            for field, maximum in (("headline", 100), ("body", 900), ("verdict", 180)):
                _text(review.get(field), f"game {game_id} review.{field}", maximum)
            score = review.get("score")
            if score is not None:
                _require(isinstance(score, dict), f"game {game_id} review.score must be an object")
                _require(set(score) <= {"value", "scale", "methodology"},
                         f"game {game_id} score contains unsupported fields")
                value = score.get("value")
                scale = score.get("scale")
                _require(isinstance(value, (int, float)) and not isinstance(value, bool) and math.isfinite(value),
                         f"game {game_id} score.value must be a finite number")
                _require(isinstance(scale, (int, float)) and not isinstance(scale, bool) and math.isfinite(scale) and scale > 0,
                         f"game {game_id} score.scale must be a positive finite number")
                _require(0 <= value <= scale, f"game {game_id} score.value must be between 0 and its scale")
                if "methodology" in score:
                    _text(score["methodology"], f"game {game_id} score.methodology", 180)

        credit = editorial.get("credit")
        _require(isinstance(credit, dict), f"game {game_id} editorial.credit is required")
        _require(set(credit) <= {"byline", "reviewedAt", "sources"},
                 f"game {game_id} credit contains unsupported fields")
        _text(credit.get("byline"), f"game {game_id} credit.byline", 80)
        reviewed_at = credit.get("reviewedAt")
        _text(reviewed_at, f"game {game_id} credit.reviewedAt", 40)
        try:
            parsed_reviewed_at = datetime.fromisoformat(reviewed_at.replace("Z", "+00:00"))
            _require(parsed_reviewed_at.tzinfo is not None,
                     f"game {game_id} credit.reviewedAt must include a timezone")
        except ValueError as error:
            raise ValueError(f"game {game_id} credit.reviewedAt must be an ISO date-time") from error
        sources = credit.get("sources", [])
        _require(isinstance(sources, list), f"game {game_id} credit.sources must be an array")
        _require(len(sources) <= 8, f"game {game_id} credit.sources has more than 8 entries")
        for index, source in enumerate(sources):
            _require(isinstance(source, dict), f"game {game_id} source[{index}] must be an object")
            _require(set(source) <= {"title", "url"}, f"game {game_id} source[{index}] has unsupported fields")
            _text(source.get("title"), f"game {game_id} source[{index}].title", 100)
            _https_url(source.get("url"), f"game {game_id} source[{index}].url")

    return len(games)


def main() -> int:
    path = Path(sys.argv[1]) if len(sys.argv) > 1 else Path("web/data/editorial-overrides.json")
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
        count = validate_data(data)
    except (OSError, json.JSONDecodeError, ValueError) as error:
        print(f"EDITORIAL VALIDATION ERROR: {error}", file=sys.stderr)
        return 1
    print(f"Editorial overrides valid: {count} game record(s)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
