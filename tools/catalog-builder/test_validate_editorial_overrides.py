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



    def test_accepts_artwork_with_verified_rights_and_credit(self):
        data = valid_document()
        data["games"]["vita-test-game"]["artwork"] = {
            "cover": {
                "url": "https://cdn.example.org/cover.jpg",
                "alt": "Game cover artwork",
                "caption": "European retail cover",
                "source": "Publisher press kit",
                "sourceUrl": "https://publisher.example.org/press",
                "credit": "Publisher / Studio",
                "license": "Permission granted by publisher",
                "rightsStatus": "permission-granted",
                "rightsReviewedAt": "2026-11-18T02:00:00+01:00",
                "width": 800,
                "height": 1200
            },
            "screenshots": [{
                "url": "https://cdn.example.org/screenshot-01.jpg",
                "alt": "Gameplay screenshot showing a city street",
                "source": "Official press kit",
                "credit": "Publisher / Studio",
                "license": "Permission granted by publisher",
                "rightsStatus": "permission-granted",
                "rightsReviewedAt": "2026-11-18T02:00:00+01:00",
                "width": 1920,
                "height": 1080
            }]
        }
        self.assertEqual(validate_data(data), 1)

    def test_rejects_artwork_without_rights_review(self):
        data = valid_document()
        data["games"]["vita-test-game"]["artwork"] = {
            "cover": {
                "url": "https://cdn.example.org/cover.jpg",
                "alt": "Game cover",
                "source": "Publisher",
                "credit": "Publisher",
                "license": "Unknown",
                "rightsStatus": "verified-license"
            }
        }
        with self.assertRaisesRegex(ValueError, "rightsReviewedAt is required"):
            validate_data(data)

    def test_rejects_artwork_pending_or_rejected_as_unapproved_state_value(self):
        data = valid_document()
        data["games"]["vita-test-game"]["artwork"] = {
            "cover": {
                "url": "https://cdn.example.org/cover.jpg",
                "alt": "Game cover",
                "source": "Publisher",
                "credit": "Publisher",
                "license": "Pending",
                "rightsStatus": "awaiting-magic",
                "rightsReviewedAt": "2026-11-18T02:00:00Z"
            }
        }
        with self.assertRaisesRegex(ValueError, "rightsStatus is not supported"):
            validate_data(data)

    def test_rejects_duplicate_screenshot_urls(self):
        data = valid_document()
        asset = {
            "url": "https://cdn.example.org/screenshot.jpg",
            "alt": "Gameplay screenshot",
            "source": "Official press kit",
            "credit": "Publisher",
            "license": "Permission granted",
            "rightsStatus": "permission-granted",
            "rightsReviewedAt": "2026-11-18T02:00:00Z"
        }
        data["games"]["vita-test-game"]["artwork"] = {"screenshots": [asset, dict(asset)]}
        with self.assertRaisesRegex(ValueError, "duplicate screenshot URL"):
            validate_data(data)

    def test_validates_artwork_even_without_an_editorial_review(self):
        data = valid_document()
        del data["games"]["vita-test-game"]["editorial"]
        data["games"]["vita-test-game"]["artwork"] = {
            "logo": {
                "url": "https://cdn.example.org/logo.png",
                "alt": "Game logo",
                "source": "Official website",
                "credit": "Rights holder",
                "license": "CC BY 4.0",
                "rightsStatus": "verified-license",
                "rightsReviewedAt": "2026-11-18T02:00:00Z"
            }
        }
        self.assertEqual(validate_data(data), 1)


if __name__ == "__main__":
    unittest.main()
