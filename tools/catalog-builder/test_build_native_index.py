import hashlib
import json
import tempfile
import unittest
from pathlib import Path

from build_native_index import build


class NativeIndexBuilderTests(unittest.TestCase):
    def fixture(self):
        return {
            "schemaVersion": "1.0.0",
            "generatedAt": "2026-11-18T09:00:00Z",
            "games": [{
                "gameId": "vita-test-001",
                "title": 'A "Quoted" Game',
                "summary": "A compact test summary.",
                "developer": "Studio Test",
                "publisher": "Publisher Test",
                "releaseYear": 2004,
                "genres": ["Action", "Adventure"],
                "contentKind": "homebrew",
                "features": ["Touchscreen", "Online co-op"],
                "themes": ["Cyberpunk"],
                "ageRatings": [
                    {"system": "ESRB", "value": "T", "territory": "US",
                     "descriptors": ["Violence"], "source": "https://ratings.test/esrb"},
                    {"system": "PEGI", "value": "12", "territory": "Europe",
                     "descriptors": ["Violence", "Bad language"], "source": "https://ratings.test/pegi"},
                ],
                "externalScores": [
                    {"provider": "OpenCritic", "value": 82, "scale": 100,
                     "voteCount": 42, "source": "https://scores.test/game"},
                ],
                "releases": [
                    {"platform": "psvita"},
                    {"platform": "psp"},
                    {"platform": "psvita"},
                ],
            }],
        }

    def test_emits_index_and_editorial_metadata(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            source = root / "catalog.json"
            output = root / "catalog_index.c"
            source.write_text(json.dumps(self.fixture()), encoding="utf-8")
            self.assertEqual(build(source, output), 1)
            generated = output.read_text(encoding="utf-8")
            self.assertIn('"vita-test-001"', generated)
            self.assertIn('A \\"Quoted\\" Game', generated)
            self.assertIn('"A compact test summary."', generated)
            self.assertIn('"Studio Test"', generated)
            self.assertIn('"Action / Adventure"', generated)
            self.assertIn('"HOMEBREW / Touchscreen / Online co-op / Cyberpunk"', generated)
            self.assertIn('"PEGI 12 (Europe) / Violence / Bad language"', generated)
            self.assertIn('"82 / 100 - OpenCritic (42 votes)"', generated)
            self.assertIn(", 3u, 3u, 2004u}", generated)
            expected_digest = hashlib.sha256(source.read_bytes()).hexdigest()
            self.assertIn('const char vita_nexus_catalog_schema_version[] = "1.0.0";', generated)
            self.assertIn('const char vita_nexus_catalog_generated_at[] = "2026-11-18T09:00:00Z";', generated)
            self.assertIn(f'const char vita_nexus_catalog_source_sha256[] = "{expected_digest}";', generated)

    def test_missing_editorial_metadata_stays_unknown(self):
        data = self.fixture()
        game = data["games"][0]
        for key in ("contentKind", "features", "themes", "ageRatings", "externalScores"):
            game.pop(key, None)
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            source = root / "catalog.json"
            output = root / "catalog_index.c"
            source.write_text(json.dumps(data), encoding="utf-8")
            build(source, output)
            generated = output.read_text(encoding="utf-8")
            self.assertIn('"Action / Adventure", "", "", ""', generated)

    def test_external_score_without_scale_does_not_assume_one(self):
        data = self.fixture()
        data["games"][0]["externalScores"][0].pop("scale")
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            source = root / "catalog.json"
            output = root / "catalog_index.c"
            source.write_text(json.dumps(data), encoding="utf-8")
            build(source, output)
            generated = output.read_text(encoding="utf-8")
            self.assertIn('"82 - OpenCritic (42 votes)"', generated)

    def test_generation_is_deterministic_and_source_digest_changes(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            source = root / "catalog.json"
            first = root / "first.c"
            second = root / "second.c"
            data = self.fixture()
            source.write_text(json.dumps(data, ensure_ascii=False), encoding="utf-8")
            build(source, first)
            build(source, second)
            self.assertEqual(first.read_bytes(), second.read_bytes())
            original_digest = hashlib.sha256(source.read_bytes()).hexdigest()
            self.assertIn(original_digest, first.read_text(encoding="utf-8"))
            data["games"][0]["title"] = "Changed source title"
            source.write_text(json.dumps(data, ensure_ascii=False), encoding="utf-8")
            build(source, second)
            changed_digest = hashlib.sha256(source.read_bytes()).hexdigest()
            self.assertNotEqual(original_digest, changed_digest)
            self.assertIn(changed_digest, second.read_text(encoding="utf-8"))

    def test_rejects_unsupported_schema_version(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            source = root / "catalog.json"
            output = root / "catalog_index.c"
            data = self.fixture()
            data["schemaVersion"] = "2.0.0"
            source.write_text(json.dumps(data), encoding="utf-8")
            with self.assertRaises(ValueError):
                build(source, output)

    def test_rejects_unsupported_platform(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            source = root / "catalog.json"
            output = root / "catalog_index.c"
            data = self.fixture()
            data["games"][0]["releases"][0]["platform"] = "unknown"
            source.write_text(json.dumps(data), encoding="utf-8")
            with self.assertRaises(ValueError):
                build(source, output)


if __name__ == "__main__":
    unittest.main()
