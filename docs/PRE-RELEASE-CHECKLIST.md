# VITA NEXUS · First pre-release gates

This checklist separates the **web companion alpha** from the **native Vita alpha**. A successful browser build does not certify a VPK, and a generated catalog does not prove the native scanner. Keep every gate tied to the exact commit and artifact that was tested.

## Web companion · 0.1.0-alpha

### Required before tagging
- [x] Cyberpunk manga visual system applied to landing, catalog and field notes.
- [x] Static catalog browser with title, identifier and region search.
- [x] PS Vita, PSP, PS1 and homebrew filters.
- [x] Regional release details and progressive loading.
- [x] Metadata-only NPS and homebrew source pipeline.
- [x] Cross-platform recommendation engine with explicit explanations.
- [x] Schema, source importer, catalog snapshot and browser-logic CI.
- [ ] Smoke-test the public Pages URL after the latest successful deployment; deployment run #44715029929 is green and the current main differs from that deployed commit only in native files, so the web source tree is unchanged.
- [ ] Run the mobile/desktop keyboard, touch, focus and reduced-motion acceptance pass.
- [ ] Verify all external artwork and score attribution in the published catalog.
- [ ] Confirm the catalog snapshot has no package download URLs, zRIF/license material or binary payloads.
- [ ] Prepare alpha notes with known gaps and the exact catalog generation timestamp.

### Known limitations
- Editorial enrichment is incomplete; missing ratings, artwork and feature tags must remain unknown rather than guessed.
- Public rankings and community lists are not yet a public community service. Browser-local custom lists are private to the current browser and support versioned JSON backup/restore.
- GitHub Pages is static hosting; public accounts, votes and cloud sync require a separate backend.

## Native application · separate gate

### Build and package
- [x] Pin the VitaSDK 2026.08 toolchain and produce a CI-built VPK.
- [x] Verify ZIP integrity, required SELF/SFO entries, Title ID, unsafe paths, duplicate entries, symlinks, encrypted entries, archive limits and forbidden game/license payloads in CI.
- [x] Set and verify the current native Title ID VNXS00001 in the generated SFO.
- [ ] Confirm Title ID uniqueness and final distribution identity before public release.
- [ ] Confirm byte-identical reproducibility across independent clean builds.
- [x] Generate the native read-only catalog index from the shared schema-v1.0.0 web snapshot, retaining its source SHA-256 and generation metadata.

### Implemented native foundation
- [x] Native Vita shell built with the pinned VitaSDK toolchain, libvita2d and SceCtrl.
- [x] Offline Discover catalog browser and metadata detail sheets.
- [x] Local Library with favorites, backlog, completion and personal score.
- [x] Three local profiles with independent collections and explicit backup/restore.
- [x] Digital D-pad menu state logic with host regression tests.
- [x] App-owned data paths and atomic temporary-file replacement for Library/profile files.
- [x] First editorial magazine interface (native package 00.10).

### Remaining native work and device gates
- [ ] Create and migrate a shared local SQLite database for user-owned state; the current profile/library files are versioned app-owned files, not SQLite.
- [ ] Implement read-only installed-title discovery with bounded metadata parsing; scanning must never move, rename or modify game files.
- [ ] Verify the 00.10 magazine layout, typography, focus visibility and frame pacing on a real PS Vita. The earlier 00.05 baseline was confirmed on a PS Vita 1000; that does not certify the 00.10 visual redesign.
- [ ] Verify D-pad response, analog input, touchscreen behavior, suspend/resume and application lifecycle on a real Vita.
- [ ] Verify offline startup and recovery from missing/corrupt catalog or local-state files.
- [ ] Implement session journal and automatic playtime only after reliable lifecycle signals and recovery behavior are demonstrated.
- [ ] Test install, update and rollback on PS Vita 1000 and 2000 where available.

## Release integrity

- Never bundle commercial game data, license files or package downloads.
- Keep upstream source attribution and asset usage notes with editorial metadata.
- Publish only metadata and artwork references that are safe and permitted to redistribute.
- Keep native and web version numbers independent when their release gates differ.
- A draft release is a review artifact; do not describe it as hardware-certified.
- Record the tested commit SHA, VPK SHA-256, Vita model, firmware, install result and test notes for every physical-device pass.

## Historical evidence · 19 November 2026 (superseded by the 20 November refresh)

Main HEAD at this evidence capture: `e24d0d7bb02d880738e6f0f664f9601c22983d65`. Native profile hardening PR [#23](https://github.com/SilverCrow2323/vita-nexus/pull/23) is merged as this commit.

- [x] Quality workflow [#44499800658](https://github.com/SilverCrow2323/vita-nexus/actions/runs/44499800658) passed on the post-merge main HEAD.
- [x] Native Vita build [#44499800692](https://github.com/SilverCrow2323/vita-nexus/actions/runs/44499800692) passed on the same HEAD.
- [x] Generated VPK: 1,314,811 bytes; SHA-256 `66658f4ec7387f3961a5e109adc3486e5c0d2f0e00662cd56f9c0cdfe5298991`; Title ID `VNXS00001`.
- [x] Actions artifact [#14517011058](https://github.com/SilverCrow2323/vita-nexus/actions/runs/44499800692/artifacts/14517011058): outer ZIP digest `sha256:1453f01ed0d58f54a05e416c7deca78981573f6526430d4325aa68702e463dc5`.
- [x] Profile parser hardening from PR #23 is merged. Canonical slot/checksum validation and rejected-import preservation are covered by host tests.
- [x] Reconciled the public catalog summary against the exact canonical snapshot: schema `1.0.0`, generated at `2026-11-18T13:01:01Z`, **9,734 games / 12,359 releases / 4,468 homebrew entries**.
- [x] Recorded the catalog snapshot SHA-256 `e60cfe04a3d1b89024980a9f1a83682eaa4c1f40742f8060aaf9807c78245bb0` and Git blob SHA-1 `ba0e263ebdec8d32af1438649323a11f7e265b27` in [the catalog snapshot manifest](CATALOG-SNAPSHOT-MANIFEST.md).
- [ ] Re-run the 00.10 visual, input, focus, lifecycle, install/update/rollback and offline-recovery acceptance on a real PS Vita 1000/2000.

The workflows prove host tests, VitaSDK cross-build and VPK packaging only. They do not prove physical installation or input behavior, and no public release is published.


## Exact-head evidence refresh · 20 November 2026

Current main HEAD: `6046df1583ffe54a8c88328f91e7c9202b5ec31e`. Native personal-library parser hardening PR [#31](https://github.com/SilverCrow2323/vita-nexus/pull/31) is merged at this HEAD.

- [x] Quality workflow [#44723365959](https://github.com/SilverCrow2323/vita-nexus/actions/runs/44723365959): passed on the current main HEAD.
- [x] Native Vita build [#44723365958](https://github.com/SilverCrow2323/vita-nexus/actions/runs/44723365958): passed on the same HEAD, including pinned VitaSDK compilation and VPK integrity verification.
- [x] Generated VPK: 1,284,937 bytes; SHA-256 `275970467edb76885b631811babf5f0b646e5ab0b5ce42b00dd21ad5c22786a0`; Title ID `VNXS00001`.
- [x] Actions artifact [#14621964499](https://github.com/SilverCrow2323/vita-nexus/actions/runs/44723365958/artifacts/14621964499): 1,285,193-byte outer ZIP; digest `sha256:30b6f3533aeb51480c30509951ee73c72e178ef5feec1bdf3efce92d15a8a56f`.
- [x] PR #31 parser hardening is merged: the `VNXLIB|1` reader now rejects signs, whitespace, leading-zero and overflowing decimal fields, and requires an exact eight-hex-digit checksum trailer. Host regression tests verify malformed loads preserve the prior in-memory collection.
- [x] Public catalog snapshot remains schema `1.0.0`, generated at `2026-11-19T13:00:08Z`: **9,756 games / 12,381 releases / 4,490 homebrew entries**. The manifest records source provenance and SHA-256; catalog sync run [#44529105744](https://github.com/SilverCrow2323/vita-nexus/actions/runs/44529105744) succeeded, followed by quality/native checks [#44529162909](https://github.com/SilverCrow2323/vita-nexus/actions/runs/44529162909) and [#44529163312](https://github.com/SilverCrow2323/vita-nexus/actions/runs/44529163312).
- [x] Latest Pages deployment [#44715029929](https://github.com/SilverCrow2323/vita-nexus/actions/runs/44715029929) succeeded on commit `8686f073e5f40d2cc9487ccf2dd545d52c992143`. Comparing that commit with current main shows only `native/README.md`, `native/src/library_store.c` and `tests/native_library_store_test.c` changed; the deployed web source tree therefore matches current main. The public URL itself still needs a direct smoke check.
- [ ] Re-run the 00.10 visual, input, focus, lifecycle, install/update/rollback and offline-recovery acceptance on a real PS Vita 1000/2000.

SQLite schema/migrations and installed-title discovery remain unimplemented. The earlier 00.05 Vita 1000 baseline is not evidence for the 00.10 magazine UI. The successful CI VPK is not a physical-device test, and no public release is published.

---

## Exact native alpha evidence · 21 November 2026

This evidence record pins the current main build and the exact VPK file downloaded and inspected from GitHub Actions. It is an **alpha artifact**, not a published GitHub Release or a hardware-certified package.

| Field | Verified value |
|---|---|
| Source HEAD | `8afc13694c5989a105eca07c5b00482aaa212a51` |
| Native build | [#44904872013](https://github.com/SilverCrow2323/vita-nexus/actions/runs/44904872013) — success |
| Quality | [#44904872062](https://github.com/SilverCrow2323/vita-nexus/actions/runs/44904872062) — success |
| Pages deployment | [#44904872043](https://github.com/SilverCrow2323/vita-nexus/actions/runs/44904872043) — success |
| Actions artifact | [#14707527114](https://github.com/SilverCrow2323/vita-nexus/actions/runs/44904872013/artifacts/14707527114) |
| Outer ZIP | 1,290,512 bytes · SHA-256 `c2437cdcf411ad783277574602823352f0f452d54e45516aacd16f6022f530de` |
| Extracted VPK | 1,290,260 bytes · SHA-256 `4f34c0a9fbe27d0a31e31c7546d3011593b79a2ad9211c35654e298927188ad2` |
| SFO identity | `VNXS00001` · `APP_VER=00.10` · `VERSION=00.00` |

The artifact ZIP was downloaded and extracted; the VPK's SHA-256 was calculated from its actual bytes. CI independently verifies the VPK structure and Title ID.

The previous 00.05 baseline was confirmed on a PS Vita 1000. That does **not** certify the 00.10 magazine redesign or this exact VPK. Real-device installation, typography/layout, focus, controls/touch, suspend/resume, offline recovery and shutdown remain pending.

The machine-readable [native alpha candidate manifest](releases/native-alpha-2026-11-21.json) is the source of truth for these exact checksums, workflow links and remaining gates. No stable release or tag is published.
