import json
import unittest

from import_nps_tsv import build_catalog


def row(size: int, **values: str) -> list[str]:
    fields = [""] * size
    for key, value in values.items():
        fields[int(key)] = value
    return fields


class NpsCatalogImporterTests(unittest.TestCase):
    def test_vita_regional_editions_group_together(self):
        eu = row(11, **{
            "0": "PCSF00001",
            "1": "EU",
            "2": "Example Adventure",
            "3": "MUST_NOT_BE_EXPORTED_URL",
            "4": "MUST_NOT_BE_EXPORTED_LICENSE",
            "5": "EP0000-PCSF00001_00-EXAMPLE000000000",
            "6": "2026-01-01 00:00:00",
            "7": "Example Adventure",
            "8": "123456",
            "9": "MUST_NOT_BE_EXPORTED_DIGEST",
            "10": "3.60",
        })
        us = row(11, **{
            "0": "PCSA00001",
            "1": "US",
            "2": "Example Adventure",
            "3": "MUST_NOT_BE_EXPORTED_URL",
            "4": "MUST_NOT_BE_EXPORTED_LICENSE",
            "5": "UP0000-PCSA00001_00-EXAMPLE000000000",
            "6": "2026-01-02 00:00:00",
            "7": "Example Adventure",
            "8": "123456",
            "9": "MUST_NOT_BE_EXPORTED_DIGEST",
            "10": "3.60",
        })
        text = "header\n" + "\n".join("\t".join(fields) for fields in (eu, us))
        catalog = build_catalog({"PSV_GAMES": text})

        self.assertEqual(catalog["totals"], {"games": 1, "releases": 2})
        game = catalog["games"][0]
        self.assertEqual(game["title"], "Example Adventure")
        self.assertEqual({release["region"] for release in game["releases"]}, {"EU", "US"})
        serialized = json.dumps(catalog)
        self.assertNotIn("MUST_NOT_BE_EXPORTED_URL", serialized)
        self.assertNotIn("MUST_NOT_BE_EXPORTED_LICENSE", serialized)
        self.assertNotIn("MUST_NOT_BE_EXPORTED_DIGEST", serialized)

    def test_psp_and_ps1_remain_separate_platform_records(self):
        psp = row(10, **{
            "0": "ULUS00001",
            "1": "US",
            "3": "Retro Racer",
            "4": "DROP_URL",
            "5": "UP0000-ULUS00001_00-RETRO0000000000",
            "6": "2026-02-01 00:00:00",
            "9": "987654",
        })
        ps1 = row(9, **{
            "0": "SLUS00001",
            "1": "US",
            "2": "Retro Racer",
            "3": "DROP_URL",
            "4": "UP0000-SLUS00001_00-RETRO0000000000",
            "5": "2026-02-01 00:00:00",
            "8": "765432",
        })
        catalog = build_catalog({
            "PSP_GAMES": "header\n" + "\t".join(psp),
            "PSX_GAMES": "header\n" + "\t".join(ps1),
        })
        self.assertEqual(catalog["totals"], {"games": 2, "releases": 2})
        self.assertEqual({game["platforms"][0] for game in catalog["games"]}, {"psp", "ps1"})

    def test_malformed_rows_are_skipped(self):
        catalog = build_catalog({"PSV_GAMES": "header\nshort\n"})
        self.assertEqual(catalog["totals"], {"games": 0, "releases": 0})
        self.assertEqual(catalog["sources"][0]["recordCount"], 0)


if __name__ == "__main__":
    unittest.main()
