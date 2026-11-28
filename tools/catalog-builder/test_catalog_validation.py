import hashlib
import json
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

from catalog_validation import validate_catalog
from validate_catalog import inspect_forbidden, main as validate_catalog_main


class CatalogShapeValidationTests(unittest.TestCase):
    def test_public_validator_rejects_punctuation_variants_of_payload_keys(self):
        for key in ("download-url", "package.url", "sha-256", "license-key", "download_url"):
            with self.subTest(key=key):
                with self.assertRaises(SystemExit) as result:
                    inspect_forbidden({"nested": {key: "redacted"}})
                self.assertEqual(result.exception.code, 1)

    def test_public_validator_rejects_non_object_root_cleanly(self):
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "catalog.json"
            path.write_text("[]", encoding="utf-8")
            with patch("sys.argv", ["validate_catalog.py", str(path)]):
                with self.assertRaises(SystemExit) as result:
                    validate_catalog_main()
            self.assertEqual(result.exception.code, 1)

    def valid_catalog(self):
        return {
            "schemaVersion": "1.0.0",
            "sources": [{
                "sourceId": "nopaystation",
                "dataset": "PSV_GAMES",
                "url": "https://example.com/PSV_GAMES.tsv",
                "recordCount": 1,
            }],
            "totals": {"games": 1, "releases": 1},
            "games": [{
                "gameId": "test-001",
                "title": "Test title",
                "platforms": ["psvita"],
                "releases": [{
                    "releaseId": "test-001-psvita-eu",
                    "platform": "psvita",
                    "region": "EU",
                    "sourceId": "nopaystation",
                    "sourceDataset": "PSV_GAMES",
                }],
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
        datasets = [
            ("nopaystation", "PSV_GAMES", "psvita"),
            ("nopaystation", "PSP_GAMES", "psp"),
            ("nopaystation", "PSX_GAMES", "ps1"),
            ("vitahomebrewdb", "apps.json", "homebrew"),
        ]
        data["sources"] = [
            {"sourceId": source_id, "dataset": dataset, "url": f"https://example.com/{dataset}", "recordCount": 1}
            for source_id, dataset, _platform in datasets
        ]
        data["games"][0]["platforms"] = [platform for _source, _dataset, platform in datasets]
        data["games"][0]["releases"] = [
            {
                "releaseId": f"release-{platform}",
                "platform": platform,
                "sourceId": source_id,
                "sourceDataset": dataset,
            }
            for source_id, dataset, platform in datasets
        ]
        data["totals"]["releases"] = len(datasets)
        self.assertIs(validate_catalog(data), data)

    def test_accepts_repeated_provider_id_for_distinct_datasets(self):
        data = self.valid_catalog()
        data["sources"].append({
            "sourceId": "nopaystation",
            "dataset": "PSP_GAMES",
            "recordCount": 1,
        })
        data["games"][0]["releases"].append({
            "releaseId": "test-001-psp-eu",
            "platform": "psp",
            "region": "EU",
            "sourceId": "nopaystation",
            "sourceDataset": "PSP_GAMES",
        })
        data["totals"]["releases"] = 2
        self.assertIs(validate_catalog(data), data)

    def test_rejects_release_without_registered_source_pair(self):
        data = self.valid_catalog()
        data["games"][0]["releases"][0]["sourceDataset"] = "PSP_GAMES"
        with self.assertRaisesRegex(ValueError, "references unknown source pair"):
            validate_catalog(data)

    def test_rejects_non_object_source_row(self):
        data = self.valid_catalog()
        data["sources"][0] = None
        with self.assertRaisesRegex(ValueError, r"catalog.sources\[0\] must be an object"):
            validate_catalog(data)

    def test_rejects_invalid_source_record_count(self):
        data = self.valid_catalog()
        data["sources"][0]["recordCount"] = True
        with self.assertRaisesRegex(ValueError, "recordCount must be a non-negative integer"):
            validate_catalog(data)

    def test_rejects_malformed_totals_without_attribute_error(self):
        data = self.valid_catalog()
        data["totals"] = "not-an-object"
        with self.assertRaisesRegex(ValueError, "catalog.totals must be an object"):
            validate_catalog(data)

    def test_rejects_totals_that_do_not_match_canonical_arrays(self):
        data = self.valid_catalog()
        data["totals"]["releases"] = 2
        with self.assertRaisesRegex(ValueError, "catalog.totals.releases is 2, expected 1"):
            validate_catalog(data)

    def test_public_validator_rejects_unregistered_source_pair(self):
        data = self.valid_catalog()
        data["games"][0]["releases"][0]["sourceDataset"] = "PSP_GAMES"
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "catalog.json"
            path.write_text(json.dumps(data), encoding="utf-8")
            with patch("sys.argv", ["validate_catalog.py", str(path)]):
                with self.assertRaises(SystemExit) as result:
                    validate_catalog_main()
            self.assertEqual(result.exception.code, 1)


    def test_rejects_duplicate_provider_dataset_pair(self):
        data = self.valid_catalog()
        data["sources"].append({
            "sourceId": "nopaystation",
            "dataset": "PSV_GAMES",
            "recordCount": 1,
        })
        with self.assertRaisesRegex(ValueError, "duplicates source dataset"):
            validate_catalog(data)

    def test_allows_multiple_datasets_from_the_same_provider(self):
        data = self.valid_catalog()
        data["sources"].append({
            "sourceId": "nopaystation",
            "dataset": "PSP_GAMES",
            "recordCount": 0,
        })
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



    def test_rejects_non_https_or_ambiguous_source_urls(self):
        for url in (
            "http://example.com/feed.tsv",
            "https:///missing-host",
            "https://user:pass@example.com/feed.tsv",
            "https://example.com/feed.tsv#fragment",
            "https://example.com:bad/feed.tsv",
        ):
            with self.subTest(url=url):
                data = self.valid_catalog()
                data["sources"][0]["url"] = url
                with self.assertRaisesRegex(ValueError, "HTTPS"):
                    validate_catalog(data)

    def test_rejects_casefolded_duplicate_provider_dataset_pairs(self):
        data = self.valid_catalog()
        data["sources"].append({
            "sourceId": "NoPayStation",
            "dataset": "psv_games",
            "url": "https://example.com/duplicate.tsv",
            "recordCount": 0,
        })
        with self.assertRaisesRegex(ValueError, "duplicates source dataset"):
            validate_catalog(data)


if __name__ == "__main__":
    unittest.main()
