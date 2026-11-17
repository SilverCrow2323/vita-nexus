# VITA NEXUS · First pre-release gates

This checklist separates the **web catalog alpha** from the future **native Vita alpha**. A successful browser build does not certify a VPK, and a generated catalog does not prove the native scanner.

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
- Rankings and custom lists are not yet a public community service.
- GitHub Pages is static hosting; public accounts, votes and cloud sync require a separate backend.

## Native application · separate gate

- [x] Pin VitaSDK 2026.08 and build a CI VPK with a recorded SHA-256.
- [x] Verify ZIP integrity, eboot.bin, sce_sys/param.sfo, Title ID and forbidden game/license payloads in CI.
- [x] Set Title ID VNXS00001 in the generated SFO and verify it in CI.
- [ ] Confirm Title ID uniqueness/final distribution identity before public release.
- [ ] Confirm byte-identical reproducibility across independent clean builds.
- [ ] Implement local database creation and migrations.
- [ ] Implement read-only installed-title discovery with bounded metadata parsing.
- [x] Implement the first D-pad menu state machine and host regression tests.
- [ ] Verify D-pad response, analog, touchscreen, focus, frame pacing and suspend/resume on a real Vita.
- [ ] Verify offline startup, corrupt/missing catalog handling and interrupted scan recovery.
- [ ] Implement local profiles and session journal before claiming automatic playtime.
- [ ] Test install, update and rollback on PS Vita 1000 and 2000 where available.

## Release integrity
- Never bundle commercial game data, license files or package downloads.
- Keep upstream source attribution and asset usage notes with editorial metadata.
- Publish only the metadata and artwork references that are safe and permitted to redistribute.
- Keep native and web version numbers independent when their release gates differ.
- A draft release is a review artifact; do not describe it as hardware-certified.


## Latest native CI evidence · 17 November 2026

- VitaSDK 2026.08 native build run: [#44126990292](https://github.com/SilverCrow2323/vita-nexus/actions/runs/44126990292) — passed.
- Quality run: [#44126990433](https://github.com/SilverCrow2323/vita-nexus/actions/runs/44126990433) — passed.
- Title ID: `VNXS00001`.
- VPK SHA-256: `a0884b3ea90702a8193b58ece868fd1fd0d701a4ccd4ab66ef44cd75d44d4ef7`.
- Actions artifact #14348266171; 71,973-byte outer ZIP; SHA-256 `d529ac739ca12c1c2a642d8f71d34108cbf6558d7ebe6c1539ce848ddedae42b`.

This is a CI-built package only. No PS Vita console installation, screen rendering, physical controls, suspend/resume or application lifecycle has been tested.
