-- VITA NEXUS local database schema v2.
-- Track snapshot membership without deleting rows referenced by personal state.

ALTER TABLE catalog_games
    ADD COLUMN is_in_snapshot INTEGER NOT NULL DEFAULT 1
        CHECK (is_in_snapshot IN (0, 1));

ALTER TABLE catalog_releases
    ADD COLUMN is_in_snapshot INTEGER NOT NULL DEFAULT 1
        CHECK (is_in_snapshot IN (0, 1));

CREATE INDEX catalog_games_snapshot_idx ON catalog_games(is_in_snapshot, title);
CREATE INDEX catalog_releases_snapshot_idx ON catalog_releases(is_in_snapshot, platform, region);

CREATE TABLE catalog_sources (
    source_id TEXT NOT NULL CHECK (length(trim(source_id)) BETWEEN 1 AND 96),
    dataset TEXT NOT NULL CHECK (length(trim(dataset)) BETWEEN 1 AND 128),
    name TEXT NOT NULL CHECK (length(trim(name)) > 0),
    url TEXT NOT NULL CHECK (length(trim(url)) > 0),
    fetched_at TEXT NOT NULL DEFAULT '',
    record_count INTEGER NOT NULL CHECK (record_count >= 0),
    notes TEXT NOT NULL DEFAULT '',
    is_in_snapshot INTEGER NOT NULL DEFAULT 1 CHECK (is_in_snapshot IN (0, 1)),
    PRIMARY KEY (source_id, dataset)
);

CREATE INDEX catalog_sources_snapshot_idx
    ON catalog_sources(is_in_snapshot, source_id, dataset);
