import unittest

from catalog_validation import validate_catalog


class CatalogShapeValidationTests(unittest.TestCase):
    def valid_catalog(self):
        return {
            "schemaVersion": "1.0.0",
            "games": [{
                "gameId": "test-001",
                "title": "Test title",
                "releases": [{"platform": "psvita"}],
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

    def test_rejects_unknown_platform_with_path(self):
        data = self.valid_catalog()
        data["games"][0]["releases"][0]["platform"] = "unknown"
        with self.assertRaisesRegex(ValueError, "platform is unsupported"):
            validate_catalog(data)

    def test_rejects_oversized_release_count(self):
        data = self.valid_catalog()
        data["games"][0]["releases"] = [{"platform": "psvita"}] * 65536
        with self.assertRaisesRegex(ValueError, "native index limit"):
            validate_catalog(data)

    def test_accepts_supported_platforms(self):
        data = self.valid_catalog()
        data["games"][0]["releases"] = [
            {"platform": "psvita"},
            {"platform": "psp"},
            {"platform": "ps1"},
            {"platform": "homebrew"},
        ]
        self.assertIs(validate_catalog(data), data)


if __name__ == "__main__":
    unittest.main()
