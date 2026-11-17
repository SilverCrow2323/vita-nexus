from __future__ import annotations

import tempfile
import zipfile
from pathlib import Path

import pytest

from tools.verify_vpk import VpkValidationError, validate_vpk


TITLE_ID = "VNXS00001"


def write_vpk(path: Path, entries: dict[str, bytes] | None = None, *, include_fonts: bool = True, include_license: bool = True) -> None:
    payload = entries or {
        "eboot.bin": b"native-self-placeholder",
        "sce_sys/param.sfo": bytes.fromhex("005449544C455F494400564E5853303030303100"),
    }
    payload = dict(payload)
    if include_fonts:
        payload.setdefault("assets/fonts/BarlowCondensed-Medium.ttf", bytes.fromhex("00010000") + bytes(32))
        payload.setdefault("assets/fonts/BarlowCondensed-Bold.ttf", bytes.fromhex("00010000") + bytes(32))
        if include_license:
            payload.setdefault("assets/fonts/OFL.txt", b"SIL OPEN FONT LICENSE Version 1.1")
    with zipfile.ZipFile(path, "w", compression=zipfile.ZIP_STORED) as archive:
        for name, data in payload.items():
            archive.writestr(name, data)

def test_valid_vpk_reports_integrity_and_title_id(tmp_path: Path):
    package = tmp_path / "valid.vpk"
    write_vpk(package)

    report = validate_vpk(package, TITLE_ID)

    assert report["title_id"] == TITLE_ID
    assert report["entries"] == 5
    assert len(report["sha256"]) == 64
    assert report["size_bytes"] == package.stat().st_size


def test_rejects_missing_required_vpk_entry(tmp_path: Path):
    package = tmp_path / "missing.vpk"
    write_vpk(package, {"sce_sys/param.sfo": b"VNXS00001"})

    with pytest.raises(VpkValidationError, match="missing required"):
        validate_vpk(package, TITLE_ID)


def test_rejects_wrong_title_id(tmp_path: Path):
    package = tmp_path / "wrong-title.vpk"
    write_vpk(package, {"eboot.bin": b"self", "sce_sys/param.sfo": b"OTHER0001"})

    with pytest.raises(VpkValidationError, match="does not contain Title ID"):
        validate_vpk(package, TITLE_ID)


@pytest.mark.parametrize("name", ["game.iso", "backup/Disc.ISO", "license.zrif", "sce_sys/work.bin"])
def test_rejects_game_or_license_payloads(tmp_path: Path, name: str):
    package = tmp_path / "forbidden.vpk"
    write_vpk(package, {
        "eboot.bin": b"self",
        "sce_sys/param.sfo": b"VNXS00001",
        name: b"not allowed",
    })

    with pytest.raises(VpkValidationError, match="not allowed"):
        validate_vpk(package, TITLE_ID)


@pytest.mark.parametrize("name", ["../escape.bin", "/absolute.bin", "sce_sys\\escape.bin"])
def test_rejects_unsafe_archive_paths(tmp_path: Path, name: str):
    package = tmp_path / "unsafe.vpk"
    write_vpk(package, {
        "eboot.bin": b"self",
        "sce_sys/param.sfo": b"VNXS00001",
        name: b"bad path",
    })

    with pytest.raises(VpkValidationError, match="unsafe archive path"):
        validate_vpk(package, TITLE_ID)


def test_rejects_non_zip_file(tmp_path: Path):
    package = tmp_path / "not-a-vpk.vpk"
    package.write_bytes(b"not a zip")

    with pytest.raises(VpkValidationError, match="not a valid ZIP"):
        validate_vpk(package, TITLE_ID)


def test_rejects_invalid_title_id_shape(tmp_path: Path):
    package = tmp_path / "valid.vpk"
    write_vpk(package)

    with pytest.raises(VpkValidationError, match="exactly nine"):
        validate_vpk(package, "bad")


def test_rejects_missing_bundled_font(tmp_path: Path):
    package = tmp_path / "missing-font.vpk"
    write_vpk(package, include_fonts=False)

    with pytest.raises(VpkValidationError, match="missing required"):
        validate_vpk(package, TITLE_ID)


def test_rejects_invalid_bundled_font_signature(tmp_path: Path):
    package = tmp_path / "invalid-font.vpk"
    write_vpk(package, {
        "eboot.bin": b"self",
        "sce_sys/param.sfo": b"VNXS00001",
        "assets/fonts/BarlowCondensed-Medium.ttf": b"not-a-font",
    })

    with pytest.raises(VpkValidationError, match="invalid TrueType/OpenType signature"):
        validate_vpk(package, TITLE_ID)


def test_rejects_missing_font_license(tmp_path: Path):
    package = tmp_path / "missing-font-license.vpk"
    write_vpk(package, include_license=False)

    with pytest.raises(VpkValidationError, match="missing required"):
        validate_vpk(package, TITLE_ID)


def test_rejects_invalid_font_license(tmp_path: Path):
    package = tmp_path / "invalid-font-license.vpk"
    write_vpk(package, {
        "eboot.bin": b"self",
        "sce_sys/param.sfo": b"VNXS00001",
        "assets/fonts/OFL.txt": b"not the OFL",
    })
    with pytest.raises(VpkValidationError, match="font license"):
        validate_vpk(package, TITLE_ID)
