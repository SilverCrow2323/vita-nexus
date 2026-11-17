import json
import tempfile
import unittest
from pathlib import Path

from build_native_index import build


class NativeIndexBuilderTests(unittest.TestCase):
    def fixture(self):
        return {
            "schemaVersion": "1.0.0",
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
            self.assertIn('"homebrew"', generated)
            self.assertIn('"Touchscreen / Online co-op"', generated)
            self.assertIn('"Cyberpunk"', generated)
            self.assertIn('"PEGI 12 (Europe)"', generated)
            self.assertIn('"Violence / Bad language"', generated)
            self.assertIn('"https://ratings.test/pegi"', generated)
            self.assertIn('"OpenCritic"', generated)
            self.assertIn('"https://scores.test/game"', generated)
            self.assertIn(", 3u, 3u, 2004u, 82.000f, 100.000f, 42u, 1u}", generated)

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
            self.assertIn('"", "", "", "", "", "", "", ""', generated)
            self.assertIn(", 3u, 3u, 2004u, 0.000f, 0.000f, 0u, 0u}", generated)

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
