"""Fail-closed validation shared by the public catalog and native index builder.

The source registry is batch-level: one provider may publish several datasets.
Every release must resolve the exact (sourceId, sourceDataset) pair.
"""

from __future__ import annotations

from urllib.parse import urlsplit

PLATFORM_BITS = {"psvita": 1, "psp": 2, "ps1": 4, "homebrew": 8}


def _non_empty_text(value: object) -> bool:
    return isinstance(value, str) and bool(value.strip())


def _validate_source_url(value: object, source_path: str) -> None:
    if not _non_empty_text(value) or value != value.strip():
        raise ValueError(f"{source_path} must be a clean HTTPS URL")
    try:
        parsed = urlsplit(value)
        hostname = parsed.hostname
        _ = parsed.port
    except ValueError as exc:
        raise ValueError(f"{source_path} is malformed: {exc}") from exc
    if (
        parsed.scheme.lower() != "https"
        or not hostname
        or parsed.username is not None
        or parsed.password is not None
        or parsed.fragment
        or any(ord(char) < 32 or ord(char) == 127 for char in value)
    ):
        raise ValueError(f"{source_path} must use HTTPS with a host and no user-info or fragment")


def validate_catalog(catalog: object) -> dict:
    if not isinstance(catalog, dict):
        raise ValueError("catalog root must be an object")

    if catalog.get("schemaVersion") != "1.0.0":
        raise ValueError("unsupported catalog schema; expected 1.0.0")

    sources = catalog.get("sources")
    if not isinstance(sources, list):
        raise ValueError("catalog.sources must be an array")

    # sourceId identifies the provider, not an individual dataset. Keep the
    # pair so NoPayStation and VitaHomebrewDB can each register multiple feeds.
    source_pairs: set[tuple[str, str]] = set()
    for source_index, source in enumerate(sources):
        source_path = f"catalog.sources[{source_index}]"
        if not isinstance(source, dict):
            raise ValueError(f"{source_path} must be an object")

        source_id = source.get("sourceId")
        dataset = source.get("dataset")
        if not _non_empty_text(source_id):
            raise ValueError(f"{source_path}.sourceId must be a non-empty string")
        if not _non_empty_text(dataset):
            raise ValueError(f"{source_path}.dataset must be a non-empty string")
        _validate_source_url(source.get("url"), f"{source_path}.url")

        record_count = source.get("recordCount")
        if isinstance(record_count, bool) or not isinstance(record_count, int) or record_count < 0:
            raise ValueError(f"{source_path}.recordCount must be a non-negative integer")

        source_pair = (source_id.strip().casefold(), dataset.strip().casefold())
        if source_pair in source_pairs:
            raise ValueError(
                f"{source_path} duplicates source dataset "
                f"{source_pair[0]!r}/{source_pair[1]!r}"
            )
        source_pairs.add(source_pair)

    games = catalog.get("games")
    if not isinstance(games, list):
        raise ValueError("catalog.games must be an array")

    seen_game_ids: set[str] = set()
    seen_release_ids: set[str] = set()
    release_count = 0

    for game_index, game in enumerate(games):
        game_path = f"catalog.games[{game_index}]"
        if not isinstance(game, dict):
            raise ValueError(f"{game_path} must be an object")

        game_id = game.get("gameId")
        if not _non_empty_text(game_id):
            raise ValueError(f"{game_path}.gameId must be a non-empty string")
        if game_id in seen_game_ids:
            raise ValueError(f"{game_path}.gameId duplicates {game_id!r}")
        seen_game_ids.add(game_id)

        title = game.get("title")
        if not _non_empty_text(title):
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

            release_id = release.get("releaseId")
            if not _non_empty_text(release_id):
                raise ValueError(f"{release_path}.releaseId must be a non-empty string")
            if release_id in seen_release_ids:
                raise ValueError(f"{release_path}.releaseId duplicates {release_id!r}")
            seen_release_ids.add(release_id)

            platform = release.get("platform")
            if not isinstance(platform, str) or platform not in PLATFORM_BITS:
                raise ValueError(f"{release_path}.platform is unsupported: {platform!r}")

            source_id = release.get("sourceId")
            source_dataset = release.get("sourceDataset")
            if not _non_empty_text(source_id) or not _non_empty_text(source_dataset):
                raise ValueError(f"{release_path} must declare sourceId and sourceDataset")
            if (source_id.strip().casefold(), source_dataset.strip().casefold()) not in source_pairs:
                raise ValueError(
                    f"{release_path} references unknown source pair "
                    f"{source_id!r}/{source_dataset!r}"
                )
            release_count += 1

    totals = catalog.get("totals")
    if not isinstance(totals, dict):
        raise ValueError("catalog.totals must be an object")

    expected_totals = {"games": len(games), "releases": release_count}
    for key, expected in expected_totals.items():
        value = totals.get(key)
        if isinstance(value, bool) or not isinstance(value, int) or value < 0:
            raise ValueError(f"catalog.totals.{key} must be a non-negative integer")
        if value != expected:
            raise ValueError(
                f"catalog.totals.{key} is {value}, expected {expected}"
            )

    return catalog
