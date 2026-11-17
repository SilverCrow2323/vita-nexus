import copy
import unittest

from import_vitahomebrewdb import merge_catalog, normalize_entry


BASE = {
    "schemaVersion": "1.0.0",
    "generatedAt": "2026-11-17T00:00:00Z",
    "sources": [],
    "totals": {"games": 0, "releases": 0},
    "games": [],
}


class VitaHomebrewImporterTests(unittest.TestCase):
    def test_homebrew_metadata_is_normalized_without_binary_links_or_hashes(self):
        source = {
            "id": "42",
            "name": "Example Homebrew",
            "version": "v1.2",
            "author": "Community Dev",
            "type": "2",
            "description": "A short description.",
            "long_description": "A longer description.",
            "titleid": "TEST00042",
            "date": "2026-08-01",
            "updated": "2026-11-01T12:00:00Z",
            "size": "123456",
            "icon": "a" * 64 + ".png",
            "screenshots": "screenshots/" + "b" * 64 + ".png",
            "source": "https://github.com/example/project",
            "release_page": "https://github.com/example/project/releases",
            "url": "https://example.invalid/package.zip",
            "data": "BINARY_PAYLOAD",
            "hash": "SECRET_HASH",
            "hash2": "SECRET_HASH_2",
        }
        game = normalize_entry(source, "psvita", "apps.json")
        self.assertEqual(game["platforms"], ["homebrew"])
        self.assertEqual(game["contentKind"], "homebrew")
        self.assertEqual(game["releases"][0]["targetPlatform"], "psvita")
        self.assertEqual(game["releases"][0]["titleId"], "TEST00042")
        self.assertIn("/icons/", game["homebrew"]["iconUrl"])
        serialized = str(game)
        self.assertNotIn("package.zip", serialized)
        self.assertNotIn("BINARY_PAYLOAD", serialized)
        self.assertNotIn("SECRET_HASH", serialized)

    def test_placeholder_title_id_is_not_presented_as_real_identifier(self):
        game = normalize_entry(
            {"id": "9", "name": "No ID App", "titleid": "AAAAAAAAA"},
            "psp",
            "psp_apps.json",
        )
        self.assertNotIn("titleId", game["releases"][0])
        self.assertEqual(game["releases"][0]["targetPlatform"], "psp")

    def test_merge_preserves_existing_games_and_adds_platform_specific_ids(self):
        vita = {"id": "1", "name": "Same Title", "type": "2"}
        psp = {"id": "1", "name": "Same Title", "type": "2"}
        result = merge_catalog(
            dict(BASE),
            {"apps.json": [vita], "psp_apps.json": [psp]},
            fetched_at="2026-11-17T12:00:00Z",
        )
        self.assertEqual(result["totals"], {"games": 2, "releases": 2})
        self.assertEqual(
            {game["gameId"] for game in result["games"]},
            {"homebrew-psvita-1", "homebrew-psp-1"},
        )

    def test_reimport_is_idempotent(self):
        dataset = {"apps.json": [{"id": "7", "name": "Stable App"}]}
        first = merge_catalog(copy.deepcopy(BASE), dataset, fetched_at="2026-11-17T12:00:00Z")
        second = merge_catalog(first, dataset, fetched_at="2026-11-17T12:00:00Z")
        self.assertEqual(second["totals"], {"games": 1, "releases": 1})


if __name__ == "__main__":
    unittest.main()
