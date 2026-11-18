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
- [ ] Confirm the latest GitHub Pages deployment is green and the public URL serves the same HEAD.
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
