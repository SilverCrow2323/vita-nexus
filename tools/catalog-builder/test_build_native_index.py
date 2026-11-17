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
                "releases": [
                    {"platform": "psvita"},
                    {"platform": "psp"},
                    {"platform": "psvita"},
                ],
            }],
        }

    def test_emits_index_and_detail_metadata(self):
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
            self.assertIn(", 3u, 3u, 2004u}", generated)

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
