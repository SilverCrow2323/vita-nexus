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

- [ ] Pin the VitaSDK toolchain and produce a reproducible VPK.
- [ ] Resolve the native app ID/title metadata and verify the package manifest.
- [ ] Implement local database creation and migrations.
- [ ] Implement read-only installed-title discovery with bounded metadata parsing.
- [ ] Verify D-pad, analog, touchscreen, focus and suspend/resume on a real Vita.
- [ ] Verify offline startup, corrupt/missing catalog handling and interrupted scan recovery.
- [ ] Implement local profiles and session journal before claiming automatic playtime.
- [ ] Test install, update and rollback on PS Vita 1000 and 2000 where available.

## Release integrity
- Never bundle commercial game data, license files or package downloads.
- Keep upstream source attribution and asset usage notes with editorial metadata.
- Publish only the metadata and artwork references that are safe and permitted to redistribute.
- Keep native and web version numbers independent when their release gates differ.
- A draft release is a review artifact; do not describe it as hardware-certified.
