import hashlib
import json
import unittest
from pathlib import Path

from catalog_validation import validate_catalog


class CatalogShapeValidationTests(unittest.TestCase):
    def valid_catalog(self):
        return {
            "schemaVersion": "1.0.0",
            "games": [{
                "gameId": "test-001",
                "title": "Test title",
                "releases": [{"releaseId": "test-001-psvita-eu", "platform": "psvita"}],
            }],
        }

    def test_rejects_non_object_root(self):
        with self.assertRaisesRegex(ValueError, "catalog root must be an object"):
            validate_catalog([])

    def test_rejects_non_object_game_with_path(self):
        data = self.valid_catalog()
        data["games"][0] = None
        with self.assertRaisesRegex(ValueError, r"catalog.games\[0\] must be an object"):
            validate_catalog(data)

    def test_rejects_empty_game_identity(self):
        data = self.valid_catalog()
        data["games"][0]["gameId"] = "  "
        with self.assertRaisesRegex(ValueError, "gameId must be a non-empty string"):
            validate_catalog(data)

    def test_rejects_duplicate_game_identity(self):
        data = self.valid_catalog()
        data["games"].append(json.loads(json.dumps(data["games"][0])))
        with self.assertRaisesRegex(ValueError, "gameId duplicates 'test-001'"):
            validate_catalog(data)

    def test_rejects_non_array_releases(self):
        data = self.valid_catalog()
        data["games"][0]["releases"] = {}
        with self.assertRaisesRegex(ValueError, "releases must be a non-empty array"):
            validate_catalog(data)

    def test_rejects_non_object_release_with_path(self):
        data = self.valid_catalog()
        data["games"][0]["releases"][0] = None
        with self.assertRaisesRegex(ValueError, r"catalog.games\[0\].releases\[0\] must be an object"):
            validate_catalog(data)

    def test_rejects_missing_release_identity(self):
        data = self.valid_catalog()
        data["games"][0]["releases"][0].pop("releaseId")
        with self.assertRaisesRegex(ValueError, "releaseId must be a non-empty string"):
            validate_catalog(data)

    def test_rejects_duplicate_release_identity_across_games(self):
        data = self.valid_catalog()
        data["games"].append({
            "gameId": "test-002",
            "title": "Another title",
            "releases": [{"releaseId": "test-001-psvita-eu", "platform": "psp"}],
        })
        with self.assertRaisesRegex(ValueError, "releaseId duplicates 'test-001-psvita-eu'"):
            validate_catalog(data)

    def test_rejects_unknown_platform_with_path(self):
        data = self.valid_catalog()
        data["games"][0]["releases"][0]["platform"] = "unknown"
        with self.assertRaisesRegex(ValueError, "platform is unsupported"):
            validate_catalog(data)

    def test_rejects_oversized_release_count(self):
        data = self.valid_catalog()
        data["games"][0]["releases"] = [
            {"releaseId": f"release-{index}", "platform": "psvita"}
            for index in range(65536)
        ]
        with self.assertRaisesRegex(ValueError, "native index limit"):
            validate_catalog(data)

    def test_accepts_supported_platforms(self):
        data = self.valid_catalog()
        data["games"][0]["releases"] = [
            {"releaseId": "release-psvita", "platform": "psvita"},
            {"releaseId": "release-psp", "platform": "psp"},
            {"releaseId": "release-ps1", "platform": "ps1"},
            {"releaseId": "release-homebrew", "platform": "homebrew"},
        ]
        self.assertIs(validate_catalog(data), data)


class CatalogSnapshotDocumentationTests(unittest.TestCase):
    def test_public_snapshot_counts_and_provenance_match_docs(self):
        root = Path(__file__).resolve().parents[2]
        snapshot_path = root / "web" / "data" / "catalog.json"
        raw_snapshot = snapshot_path.read_bytes()
        snapshot = json.loads(raw_snapshot)
        games = snapshot["games"]
        releases = [release for game in games for release in game["releases"]]
        homebrew = sum(1 for release in releases if release["platform"] == "homebrew")
        digest = hashlib.sha256(raw_snapshot).hexdigest()

        manifest = (root / "docs" / "CATALOG-SNAPSHOT-MANIFEST.md").read_text(encoding="utf-8")
        self.assertIn(f"Schema version: `{snapshot['schemaVersion']}`", manifest)
        self.assertIn(f"Generated at (UTC): `{snapshot['generatedAt']}`", manifest)
        self.assertIn(f"| Games | **{len(games):,}** |", manifest)
        self.assertIn(f"| Releases | **{len(releases):,}** |", manifest)
        self.assertIn(f"| Homebrew entries | **{homebrew:,}** |", manifest)
        self.assertIn(f"SHA-256: `{digest}`", manifest)

        readme = (root / "README.md").read_text(encoding="utf-8")
        self.assertIn(f"| Canonical game records | **{len(games):,}** |", readme)
        self.assertIn(f"| Release records | **{len(releases):,}** |", readme)
        self.assertIn(f"| Vita + PSP homebrew entries | **{homebrew:,}** |", readme)
        self.assertIn("docs/CATALOG-SNAPSHOT-MANIFEST.md", readme)

        roadmap = (root / "docs" / "ROADMAP.md").read_text(encoding="utf-8")
        self.assertIn(f"**{len(games):,} games / {len(releases):,} releases**", roadmap)
        self.assertIn("CATALOG-SNAPSHOT-MANIFEST.md", roadmap)


if __name__ == "__main__":
    unittest.main()
