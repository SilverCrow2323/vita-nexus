import hashlib
import json
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from update_snapshot_docs import update_snapshot_docs


class SnapshotDocumentationSyncTests(unittest.TestCase):
    def test_updates_manifest_readme_and_roadmap_from_one_snapshot(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            (root / "web" / "data").mkdir(parents=True)
            (root / "docs").mkdir()
            catalog = {
                "schemaVersion": "1.0.0",
                "generatedAt": "2026-11-19T10:07:51Z",
                "totals": {"games": 2, "releases": 3},
                "sources": [
                    {"name": "NoPayStation", "dataset": "PSV_GAMES",
                     "fetchedAt": "2026-11-19T10:07:50Z", "recordCount": 2},
                    {"name": "VitaHomebrewDB", "dataset": "apps.json",
                     "fetchedAt": "2026-11-19T10:07:51Z", "recordCount": 1},
                ],
                "games": [
                    {"gameId": "one", "releases": [
                        {"platform": "psvita"}, {"platform": "homebrew"}]},
                    {"gameId": "two", "releases": [{"platform": "psp"}]},
                ],
            }
            raw = (json.dumps(catalog, indent=2) + "\n").encode("utf-8")
            (root / "web" / "data" / "catalog.json").write_bytes(raw)
            (root / "README.md").write_text(
                "| Canonical game records | **0** |\n"
                "| Release records | **0** |\n"
                "| Vita + PSP homebrew entries | **0** |\n"
                "<sub>Snapshot generated at old UTC (schema old). See the [catalog snapshot manifest](docs/CATALOG-SNAPSHOT-MANIFEST.md) for source counts and SHA-256 provenance.</sub>\n"
                "| Public catalog snapshot | 🟢 old |\n",
                encoding="utf-8",
            )
            (root / "docs" / "ROADMAP.md").write_text(
                "- [x] Public catalog snapshot generated and validated: **0 games / 0 releases** on old; provenance is recorded in [the snapshot manifest](CATALOG-SNAPSHOT-MANIFEST.md).\n",
                encoding="utf-8",
            )

            (root / "docs" / "NPS-CATALOG-IMPORT.md").write_text(
                "## Importer\n\nKeep the current snapshot below.\n\n"
                "## Latest verified snapshot · old\nold data\n",
                encoding="utf-8",
            )

            summary = update_snapshot_docs(root, workflow_run_id="12345")
            manifest = (root / "docs" / "CATALOG-SNAPSHOT-MANIFEST.md").read_text(
                encoding="utf-8"
            )
            readme = (root / "README.md").read_text(encoding="utf-8")
            roadmap = (root / "docs" / "ROADMAP.md").read_text(encoding="utf-8")

            self.assertEqual(summary["games"], 2)
            self.assertEqual(summary["releases"], 3)
            self.assertEqual(summary["homebrew"], 1)
            self.assertEqual(summary["sha256"], hashlib.sha256(raw).hexdigest())
            self.assertIn("Generated at (UTC): `2026-11-19T10:07:51Z`", manifest)
            self.assertIn("SHA-256: `" + hashlib.sha256(raw).hexdigest() + "`", manifest)
            self.assertIn("[#12345]", manifest)
            self.assertIn("| Games | **2** |", manifest)
            self.assertIn("| Releases | **3** |", manifest)
            self.assertIn("| Homebrew entries | **1** |", manifest)
            self.assertIn("| Canonical game records | **2** |", readme)
            self.assertIn("| Release records | **3** |", readme)
            self.assertIn("| Vita + PSP homebrew entries | **1** |", readme)
            self.assertIn("2026-11-19 10:07:51 UTC", readme)
            self.assertIn("**2 games / 3 releases** on 2026-11-19", roadmap)
            import_doc = (root / "docs" / "NPS-CATALOG-IMPORT.md").read_text(encoding="utf-8")
            self.assertIn("## Latest verified snapshot · 2026-11-19", import_doc)
            self.assertIn("| Canonical game records | **2** |", import_doc)
            self.assertIn("| Release records | **3** |", import_doc)
            self.assertIn("| Homebrew entries | **1** |", import_doc)
            self.assertIn("| PSV_GAMES | 2026-11-19T10:07:50Z | 2 |", import_doc)
            self.assertIn("catalog sync run #12345", import_doc)
            self.assertIn(hashlib.sha256(raw).hexdigest(), import_doc)


    def test_catalog_sync_stages_every_snapshot_document_written_by_updater(self):
        root = Path(__file__).resolve().parents[2]
        workflow = (root / ".github" / "workflows" / "catalog-sync.yml").read_text(
            encoding="utf-8"
        )
        self.assertRegex(
            workflow,
            r"(?m)^\s*git add .*docs/NPS-CATALOG-IMPORT\.md$",
        )


if __name__ == "__main__":
    unittest.main()
