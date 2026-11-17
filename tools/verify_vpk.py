#!/usr/bin/env python3
"""Validate a Vita VPK container before CI publishes it."""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import stat
import sys
import zipfile
from pathlib import Path, PurePosixPath


class VpkValidationError(ValueError):
    """Raised when a VPK does not satisfy the project packaging contract."""


FORBIDDEN_SUFFIXES = (".iso", ".zrif", ".rif", ".rap", ".pkg")
FORBIDDEN_NAMES = {"game.iso", "disc-image.txt", "work.bin"}
MAX_UNCOMPRESSED_BYTES = 32 * 1024 * 1024


def validate_vpk(path: str | Path, title_id: str) -> dict[str, object]:
    package = Path(path)
    if not package.is_file() or package.stat().st_size == 0:
        raise VpkValidationError("VPK is missing or empty")
    if not re.fullmatch(r"[A-Z0-9]{9}", title_id):
        raise VpkValidationError("Title ID must be exactly nine uppercase letters/digits")
    if not zipfile.is_zipfile(package):
        raise VpkValidationError("VPK is not a valid ZIP container")

    digest = hashlib.sha256()
    with package.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)

    try:
        with zipfile.ZipFile(package) as archive:
            bad_entry = archive.testzip()
            if bad_entry is not None:
                raise VpkValidationError(f"corrupt ZIP entry: {bad_entry}")

            names: set[str] = set()
            folded_names: set[str] = set()
            total_size = 0
            for info in archive.infolist():
                name = info.filename
                if not name or name.startswith("/") or "\\" in name:
                    raise VpkValidationError(f"unsafe archive path: {name!r}")
                parts = PurePosixPath(name).parts
                if ".." in parts or ":" in parts:
                    raise VpkValidationError(f"unsafe archive path: {name!r}")
                if name in names or name.casefold() in folded_names:
                    raise VpkValidationError(f"duplicate archive path: {name}")
                names.add(name)
                folded_names.add(name.casefold())

                mode = (info.external_attr >> 16) & 0xFFFF
                if stat.S_ISLNK(mode):
                    raise VpkValidationError(f"symbolic links are not allowed: {name}")
                if info.flag_bits & 0x1:
                    raise VpkValidationError(f"encrypted ZIP entries are not allowed: {name}")

                leaf = PurePosixPath(name).name.casefold()
                if leaf in FORBIDDEN_NAMES or name.casefold().endswith(FORBIDDEN_SUFFIXES):
                    raise VpkValidationError(f"game or license payload is not allowed: {name}")
                total_size += info.file_size
                if total_size > MAX_UNCOMPRESSED_BYTES:
                    raise VpkValidationError("uncompressed VPK exceeds the 32 MiB safety limit")

            required = {"eboot.bin", "sce_sys/param.sfo"}
            missing = sorted(required - names)
            if missing:
                raise VpkValidationError("missing required VPK entries: " + ", ".join(missing))

            if archive.getinfo("eboot.bin").file_size == 0:
                raise VpkValidationError("eboot.bin is empty")
            sfo = archive.read("sce_sys/param.sfo")
            if title_id.encode("ascii") not in sfo:
                raise VpkValidationError(f"sce_sys/param.sfo does not contain Title ID {title_id}")
    except zipfile.BadZipFile as exc:
        raise VpkValidationError("VPK ZIP structure is invalid") from exc

    return {
        "path": str(package),
        "size_bytes": package.stat().st_size,
        "sha256": digest.hexdigest(),
        "title_id": title_id,
        "entries": len(names),
        "uncompressed_bytes": total_size,
    }


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("vpk", type=Path)
    parser.add_argument("--title-id", required=True)
    args = parser.parse_args(argv)
    try:
        report = validate_vpk(args.vpk, args.title_id)
    except VpkValidationError as exc:
        print(f"[VPK VERIFY] FAIL: {exc}", file=sys.stderr)
        return 1
    print("[VPK VERIFY] PASS")
    print(json.dumps(report, indent=2, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
