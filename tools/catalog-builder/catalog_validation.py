"""Fail-closed shape validation for the native catalog index input.

The public catalog is schema-validated by the quality workflow. This guard also
protects direct generator invocations and gives malformed input actionable
errors instead of leaking AttributeError/TypeError tracebacks.
"""

from __future__ import annotations

PLATFORM_BITS = {"psvita": 1, "psp": 2, "ps1": 4, "homebrew": 8}


def validate_catalog(catalog: object) -> dict:
    if not isinstance(catalog, dict):
        raise ValueError("catalog root must be an object")

    if catalog.get("schemaVersion") != "1.0.0":
        raise ValueError("unsupported catalog schema; expected 1.0.0")

    games = catalog.get("games")
    if not isinstance(games, list):
        raise ValueError("catalog.games must be an array")

    for game_index, game in enumerate(games):
        game_path = f"catalog.games[{game_index}]"
        if not isinstance(game, dict):
            raise ValueError(f"{game_path} must be an object")

        game_id = game.get("gameId")
        if not isinstance(game_id, str) or not game_id.strip():
            raise ValueError(f"{game_path}.gameId must be a non-empty string")

        title = game.get("title")
        if not isinstance(title, str) or not title.strip():
            raise ValueError(f"{game_path}.title must be a non-empty string")

        releases = game.get("releases")
        if not isinstance(releases, list) or not releases:
            raise ValueError(f"{game_path}.releases must be a non-empty array")
        if len(releases) > 65535:
            raise ValueError(f"{game_path}.releases exceeds the native index limit (65535)")

        for release_index, release in enumerate(releases):
            release_path = f"{game_path}.releases[{release_index}]"
            if not isinstance(release, dict):
                raise ValueError(f"{release_path} must be an object")
            platform = release.get("platform")
            if not isinstance(platform, str) or platform not in PLATFORM_BITS:
                raise ValueError(f"{release_path}.platform is unsupported: {platform!r}")

    return catalog
