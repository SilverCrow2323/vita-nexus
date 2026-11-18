import unittest

from validate_editorial_overrides import validate_data


def valid_document():
    return {
        "schemaVersion": "1.0.0",
        "description": "Curated editorial metadata with provenance.",
        "games": {
            "vita-test-game": {
                "editorial": {
                    "rubric": "NEXUS FIELD REVIEW",
                    "standfirst": "A compact editorial standfirst.",
                    "review": {
                        "headline": "A focused headline",
                        "body": "A review that explains its judgement.",
                        "verdict": "Worth a place in the collection.",
                        "score": {"value": 84, "scale": 100, "methodology": "Editorial assessment."},
                    },
                    "highlights": ["Excellent pacing"],
                    "caveats": ["A few rough edges"],
                    "curiosities": ["A useful piece of development trivia."],
                    "credit": {
                        "byline": "VITA NEXUS Editorial Desk",
                        "reviewedAt": "2026-11-18T00:00:00Z",
                        "sources": [{"title": "Source record", "url": "https://example.org/game"}],
                    },
                }
            }
        },
    }


class EditorialOverrideValidationTests(unittest.TestCase):
    def test_accepts_editorial_review_with_provenance(self):
        self.assertEqual(validate_data(valid_document()), 1)

    def test_rejects_score_above_its_scale(self):
        data = valid_document()
        data["games"]["vita-test-game"]["editorial"]["review"]["score"]["value"] = 101
        with self.assertRaisesRegex(ValueError, "between 0 and its scale"):
            validate_data(data)

    def test_rejects_non_https_source(self):
        data = valid_document()
        data["games"]["vita-test-game"]["editorial"]["credit"]["sources"][0]["url"] = "http://example.org/game"
        with self.assertRaisesRegex(ValueError, "must use HTTPS"):
            validate_data(data)

    def test_rejects_unattributed_editorial_content(self):
        data = valid_document()
        del data["games"]["vita-test-game"]["editorial"]["credit"]
        with self.assertRaisesRegex(ValueError, "editorial.credit is required"):
            validate_data(data)

    def test_rejects_oversized_curiosity_sidebar(self):
        data = valid_document()
        data["games"]["vita-test-game"]["editorial"]["curiosities"] = ["one", "two", "three", "four", "five"]
        with self.assertRaisesRegex(ValueError, "more than 4 entries"):
            validate_data(data)


if __name__ == "__main__":
    unittest.main()
