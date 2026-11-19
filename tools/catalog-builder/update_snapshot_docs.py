#!/usr/bin/env python3
"""Synchronize the public catalog snapshot identity across maintained docs."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
from pathlib import Path
from typing import Any


def _replace_once(text: str, pattern: str, replacement: str, label: str) -> str:
    updated, count = re.subn(pattern, replacement, text, count=1, flags=re.MULTILINE)
    if count != 1:
        raise ValueError(f"expected one {label} marker, found {count}")
    return updated


def _manifest(catalog: dict[str, Any], raw: bytes, root: Path, workflow_run_id: str | None) -> str:
    games = catalog["games"]
    releases = [release for game in games for release in game.get("releases", [])]
    homebrew = sum(1 for release in releases if release.get("platform") == "homebrew")
    sha256 = hashlib.sha256(raw).hexdigest()
    git_blob_sha1 = hashlib.sha1(
        b"blob " + str(len(raw)).encode("ascii") + b"\0" + raw
    ).hexdigest()

    workflow = (
        f"[#{workflow_run_id}](https://github.com/SilverCrow2323/vita-nexus/actions/runs/{workflow_run_id})"
        if workflow_run_id
        else "[catalog-sync.yml](../.github/workflows/catalog-sync.yml)"
    )
    source_rows = []
    for source in catalog.get("sources", []):
        source_rows.append(
            "| {name} | `{dataset}` | `{fetched}` | {count:,} |".format(
                name=source.get("name", "Unknown"),
                dataset=source.get("dataset", "unknown"),
                fetched=source.get("fetchedAt", "unknown"),
                count=int(source.get("recordCount", 0)),
            )
        )

    return "\n".join(
        [
            "# VITA NEXUS · Public catalog snapshot manifest",
            "",
            "> **DATA DROP / VERIFIED METADATA ONLY**  ",
            "> This manifest records the exact public JSON snapshot used by the web companion and native read-only catalog index. It does not describe game binaries or package availability.",
            "",
            "## Snapshot identity",
            "",
            "- File: `web/data/catalog.json`",
            f"- Schema version: `{catalog['schemaVersion']}`",
            f"- Generated at (UTC): `{catalog['generatedAt']}`",
            f"- SHA-256: `{sha256}`",
            f"- Git blob SHA-1: `{git_blob_sha1}`",
            f"- Import workflow: {workflow}",
            "",
            "## Canonical counts",
            "",
            "| Record type | Count |",
            "|---|---:|",
            f"| Games | **{len(games):,}** |",
            f"| Releases | **{len(releases):,}** |",
            f"| Homebrew entries | **{homebrew:,}** |",
            "",
            "## Source rows",
            "",
            "| Source | Dataset | Fetched at (UTC) | Accepted rows |",
            "|---|---|---|---:|",
            *source_rows,
            "",
            "The NPS importer records accepted, deduplicated rows per source dataset and reports malformed rows in its source notes. VitaHomebrewDB rows are normalized to descriptive metadata; the homebrew total above is recalculated from canonical release records.",
            "",
            "## Integrity boundary",
            "",
            "- The snapshot is metadata-only.",
            "- Package URLs, zRIF/license values, checksums, package digests and binary payloads are excluded.",
            "- Artwork remains an attributed external reference; this repository does not mirror protected game content.",
            "- The native Vita build compiles a read-only C index from this exact JSON and embeds its schema version, generation timestamp and source SHA-256.",
            "",
            "The catalog-summary regression test recalculates the counts and SHA-256 from the checked-in JSON and verifies that this manifest, README and roadmap agree. The catalog sync workflow regenerates these documents in the same commit as each refreshed snapshot.",
            "",
        ]
    )


def update_snapshot_docs(root: Path, workflow_run_id: str | None = None) -> dict[str, Any]:
    root = Path(root)
    snapshot_path = root / "web" / "data" / "catalog.json"
    raw = snapshot_path.read_bytes()
    catalog = json.loads(raw)
    if not isinstance(catalog, dict) or not isinstance(catalog.get("games"), list):
        raise ValueError("catalog snapshot must contain an object with a games array")
    if not catalog.get("schemaVersion") or not catalog.get("generatedAt"):
        raise ValueError("catalog snapshot must include schemaVersion and generatedAt")

    games = catalog["games"]
    releases = [release for game in games for release in game.get("releases", [])]
    homebrew = sum(1 for release in releases if release.get("platform") == "homebrew")
    if catalog.get("totals") != {"games": len(games), "releases": len(releases)}:
        raise ValueError("catalog totals do not match the canonical records")

    (root / "docs" / "CATALOG-SNAPSHOT-MANIFEST.md").write_text(
        _manifest(catalog, raw, root, workflow_run_id), encoding="utf-8"
    )

    readme_path = root / "README.md"
    readme = readme_path.read_text(encoding="utf-8")
    readme = _replace_once(
        readme,
        r"^\| Canonical game records \|.*$",
        f"| Canonical game records | **{len(games):,}** |",
        "README game-count row",
    )
    readme = _replace_once(
        readme,
        r"^\| Release records \|.*$",
        f"| Release records | **{len(releases):,}** |",
        "README release-count row",
    )
    readme = _replace_once(
        readme,
        r"^\| Vita \+ PSP homebrew entries \|.*$",
        f"| Vita + PSP homebrew entries | **{homebrew:,}** |",
        "README homebrew-count row",
    )
    readme = _replace_once(
        readme,
        r"^<sub>Snapshot generated at .*?</sub>$",
        f"<sub>Snapshot generated at {catalog['generatedAt'].replace('T', ' ').removesuffix('Z')} UTC (schema {catalog['schemaVersion']}). See the [catalog snapshot manifest](docs/CATALOG-SNAPSHOT-MANIFEST.md) for source counts and SHA-256 provenance.</sub>",
        "README snapshot timestamp",
    )
    readme = _replace_once(
        readme,
        r"^\| Public catalog snapshot \|.*$",
        f"| Public catalog snapshot | 🟢 {len(games):,} games · {len(releases):,} releases · {homebrew:,} homebrew entries |",
        "README status snapshot row",
    )
    readme_path.write_text(readme, encoding="utf-8")

    roadmap_path = root / "docs" / "ROADMAP.md"
    roadmap = roadmap_path.read_text(encoding="utf-8")
    roadmap = _replace_once(
        roadmap,
        r"^- \[x\] Public catalog snapshot generated and validated: .*$",
        f"- [x] Public catalog snapshot generated and validated: **{len(games):,} games / {len(releases):,} releases** on {catalog['generatedAt'][:10]}; provenance is recorded in [the snapshot manifest](CATALOG-SNAPSHOT-MANIFEST.md).",
        "roadmap snapshot summary",
    )
    roadmap_path.write_text(roadmap, encoding="utf-8")

    return {
        "generatedAt": catalog["generatedAt"],
        "games": len(games),
        "releases": len(releases),
        "homebrew": homebrew,
        "sha256": hashlib.sha256(raw).hexdigest(),
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[2])
    parser.add_argument("--workflow-run-id", default=os.environ.get("GITHUB_RUN_ID"))
    args = parser.parse_args()
    summary = update_snapshot_docs(args.root, args.workflow_run_id)
    print(
        "Snapshot docs synchronized: "
        f"{summary['games']} games / {summary['releases']} releases / "
        f"{summary['homebrew']} homebrew; SHA-256 {summary['sha256']}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
