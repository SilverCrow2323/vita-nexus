-- VITA NEXUS local database schema v1.
-- Public catalog rows are imported metadata; profile and session rows are user-owned.
-- This migration is host-tested first and is not yet linked into the Vita runtime.

CREATE TABLE schema_migrations (
    version INTEGER PRIMARY KEY CHECK (version > 0),
    name TEXT NOT NULL UNIQUE,
    sha256 TEXT NOT NULL
        CHECK (length(sha256) = 64 AND sha256 NOT GLOB '*[^0-9a-f]*'),
    applied_at TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%fZ', 'now'))
);

CREATE TABLE catalog_snapshot (
    singleton INTEGER PRIMARY KEY CHECK (singleton = 1),
    schema_version TEXT NOT NULL CHECK (schema_version = '1.0.0'),
    snapshot_sha256 TEXT NOT NULL
        CHECK (length(snapshot_sha256) = 64 AND snapshot_sha256 NOT GLOB '*[^0-9a-f]*'),
    generated_at TEXT NOT NULL,
    imported_at TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%fZ', 'now')),
    game_count INTEGER NOT NULL CHECK (game_count >= 0),
    release_count INTEGER NOT NULL CHECK (release_count >= 0)
);

CREATE TABLE catalog_games (
    game_id TEXT PRIMARY KEY CHECK (length(trim(game_id)) BETWEEN 1 AND 128),
    title TEXT NOT NULL CHECK (length(trim(title)) > 0),
    sort_title TEXT NOT NULL DEFAULT '',
    summary TEXT NOT NULL DEFAULT '',
    description TEXT NOT NULL DEFAULT '',
    developer TEXT NOT NULL DEFAULT '',
    publisher TEXT NOT NULL DEFAULT '',
    release_year INTEGER CHECK (release_year IS NULL OR release_year BETWEEN 1970 AND 2100),
    content_kind TEXT NOT NULL DEFAULT 'commercial'
        CHECK (content_kind IN ('commercial', 'homebrew')),
    platform_mask INTEGER NOT NULL CHECK (platform_mask BETWEEN 1 AND 15),
    source_schema_version TEXT NOT NULL CHECK (source_schema_version = '1.0.0')
);

CREATE TABLE catalog_releases (
    release_id TEXT PRIMARY KEY CHECK (length(trim(release_id)) BETWEEN 1 AND 160),
    game_id TEXT NOT NULL REFERENCES catalog_games(game_id) ON DELETE CASCADE,
    platform TEXT NOT NULL CHECK (platform IN ('psvita', 'psp', 'ps1', 'homebrew')),
    region TEXT NOT NULL DEFAULT '',
    language_code TEXT NOT NULL DEFAULT '',
    title_id TEXT NOT NULL DEFAULT '',
    content_id TEXT NOT NULL DEFAULT '',
    disc_id TEXT NOT NULL DEFAULT '',
    serial TEXT NOT NULL DEFAULT '',
    version TEXT NOT NULL DEFAULT '',
    package_size_bytes INTEGER CHECK (package_size_bytes IS NULL OR package_size_bytes >= 0),
    source_id TEXT NOT NULL CHECK (length(trim(source_id)) > 0),
    source_dataset TEXT NOT NULL CHECK (length(trim(source_dataset)) > 0)
);

CREATE INDEX catalog_releases_game_idx ON catalog_releases(game_id);
CREATE INDEX catalog_releases_platform_region_idx ON catalog_releases(platform, region);
CREATE INDEX catalog_releases_title_id_idx ON catalog_releases(title_id) WHERE title_id <> '';

CREATE TABLE profiles (
    profile_id TEXT PRIMARY KEY CHECK (
        length(profile_id) BETWEEN 1 AND 48 AND
        profile_id NOT GLOB '*[^A-Za-z0-9._-]*'
    ),
    display_name TEXT NOT NULL CHECK (length(trim(display_name)) BETWEEN 1 AND 48),
    is_active INTEGER NOT NULL DEFAULT 0 CHECK (is_active IN (0, 1)),
    created_at TEXT NOT NULL,
    updated_at TEXT NOT NULL
);

CREATE UNIQUE INDEX profiles_single_active_idx
    ON profiles(is_active) WHERE is_active = 1;

CREATE TABLE profile_games (
    profile_id TEXT NOT NULL REFERENCES profiles(profile_id) ON DELETE CASCADE,
    game_id TEXT NOT NULL REFERENCES catalog_games(game_id) ON DELETE CASCADE,
    flags INTEGER NOT NULL DEFAULT 0 CHECK (flags BETWEEN 0 AND 7),
    rating INTEGER NOT NULL DEFAULT 0 CHECK (rating BETWEEN 0 AND 10),
    personal_note TEXT NOT NULL DEFAULT '',
    updated_at TEXT NOT NULL,
    PRIMARY KEY (profile_id, game_id),
    CHECK (flags > 0 OR rating > 0)
);

CREATE INDEX profile_games_game_idx ON profile_games(game_id);

CREATE TABLE play_sessions (
    session_id TEXT PRIMARY KEY CHECK (length(trim(session_id)) BETWEEN 1 AND 96),
    profile_id TEXT NOT NULL REFERENCES profiles(profile_id) ON DELETE CASCADE,
    game_id TEXT NOT NULL REFERENCES catalog_games(game_id) ON DELETE RESTRICT,
    platform TEXT NOT NULL CHECK (platform IN ('psvita', 'psp', 'ps1', 'homebrew')),
    started_at TEXT NOT NULL,
    ended_at TEXT,
    duration_seconds INTEGER NOT NULL DEFAULT 0 CHECK (duration_seconds >= 0),
    end_reason TEXT NOT NULL DEFAULT 'open'
        CHECK (end_reason IN ('open', 'exit', 'suspend', 'recovered', 'manual')),
    identification_confidence TEXT NOT NULL
        CHECK (identification_confidence IN ('high', 'medium', 'low')),
    CHECK (ended_at IS NULL OR ended_at >= started_at),
    CHECK ((ended_at IS NULL AND end_reason = 'open') OR
           (ended_at IS NOT NULL AND end_reason <> 'open'))
);

CREATE INDEX play_sessions_profile_started_idx
    ON play_sessions(profile_id, started_at DESC);
CREATE INDEX play_sessions_game_started_idx
    ON play_sessions(game_id, started_at DESC);

CREATE TABLE session_events (
    event_id TEXT PRIMARY KEY CHECK (length(trim(event_id)) BETWEEN 1 AND 96),
    session_id TEXT REFERENCES play_sessions(session_id) ON DELETE SET NULL,
    profile_id TEXT NOT NULL REFERENCES profiles(profile_id) ON DELETE CASCADE,
    game_id TEXT REFERENCES catalog_games(game_id) ON DELETE SET NULL,
    platform TEXT NOT NULL CHECK (platform IN ('psvita', 'psp', 'ps1', 'homebrew')),
    event_type TEXT NOT NULL CHECK (
        event_type IN ('start', 'suspend', 'resume', 'exit', 'recovery', 'manual')
    ),
    occurred_at TEXT NOT NULL,
    source TEXT NOT NULL CHECK (length(trim(source)) > 0),
    identification_confidence TEXT NOT NULL
        CHECK (identification_confidence IN ('high', 'medium', 'low'))
);

CREATE INDEX session_events_profile_time_idx
    ON session_events(profile_id, occurred_at DESC);
CREATE INDEX session_events_session_time_idx
    ON session_events(session_id, occurred_at);
