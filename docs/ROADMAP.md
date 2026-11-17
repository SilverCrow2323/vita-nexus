# 🗺️ Roadmap

The roadmap is organized around testable milestones. A feature is not complete because a screen exists: it must work, survive edge cases and be documented.

> [!IMPORTANT]
> Dates are intentionally omitted until the toolchain, hardware test setup and first native build are established.

## Milestone 0 — Repository and product foundation

- [x] Create the main repository.
- [x] Define native + web product direction.
- [x] Establish initial architecture and documentation.
- [x] Add the GitHub Pages publishing workflow and web shell.
- [ ] Add contribution, security and licensing policy.
- [x] Choose and pin the VitaSDK 2026.08/toolchain baseline.
- [x] Establish quality CI for catalog schema/data, importer tests, JavaScript syntax and browser-logic regression tests.

## Milestone 1 — Native proof of concept

- [x] Establish a pinned VitaSDK 2026.08 CMake/VPK project baseline and CI build job.
- [x] Add the first controller-driven native shell with honest placeholder module states.
- [x] Build a minimal VPK in the pinned VitaSDK CI container.
- [x] Implement the first controller-driven native screen (hardware visual check still pending).
- [x] Implement controller-driven digital D-pad navigation across the native screens.
- [ ] Add analog-stick and touchscreen input.
- [ ] Create and migrate the local SQLite database for user-owned state.
- [x] Generate a read-only native index from the shared schema v1.0.0 catalog snapshot.
- [ ] Scan a supported set of PS Vita application metadata.
- [ ] Validate read-only behavior on hardware.

**Exit gate:** a reproducible VPK build, stable input/rendering and a scanner that reports findings without modifying files.

## Milestone 2 — Catalog foundation

- [x] Define and version the canonical game/release schema.
- [x] Add the first metadata-only NoPayStation TSV source adapter and provenance tracking.
- [x] Import metadata-only Vita, PSP, PS1 and homebrew catalog data.
- [x] Group regional editions while preserving release identifiers.
- [x] Validate source rows, duplicate IDs and ambiguous title grouping.
- [x] Generate and validate the live static catalog snapshot consumed by the web browser.

**Exit gate:** the live source import must pass validation and the same canonical records must be consumed by the native database importer and web frontend.

## Milestone 3 — Discovery experience

- [x] Animated web Home Hero and responsive navigation.
- [x] Native personal Library view with filters, status flags and personal scores.
- [x] Native Discover catalog browser (offline, controller-driven, shared catalog source).
- [x] Native game metadata detail sheet with attributed catalog fields and honest missing-data states.
- [x] Native Discover platform and A–Z initial filters with filtered result navigation.
- [x] Web Store/catalog index and regional release grouping.
- [x] Game detail sheets with attributed artwork and editorial indicators.
- [x] Cross-platform "You may also like" recommendation engine.
- [ ] Game Generator categories and explanations.

**Exit gate:** recommendations are explainable, cross-platform and based on available data rather than invented metadata.

## Milestone 4 — Personal gaming

- [x] Three independent local profiles with active-profile persistence and backups.
- [x] Favorites, personal scores, backlog and completed states.
- [ ] Session/event journal.
- [ ] Tracker proof of concept for native Vita games.
- [ ] Validate suspend/resume and crash recovery.
- [ ] Investigate and test PSP/Adrenaline and PS1/POPS tracking individually.
- [x] Per-profile collection statistics.
- [ ] Session history, session correction and playtime reconciliation.

**Exit gate:** reliable session attribution and recoverable local profiles on supported test setups.

## Milestone 5 — Rankings and community

- [ ] Personal and platform rankings.
- [x] Browser-local custom list creation, ordering and removal.
- [x] Versioned JSON export/import for personal lists (local-only; no account or backend).
- [ ] Rating aggregation with vote counts and confidence.
- [ ] Define optional backend and moderation model.
- [ ] Public community submissions and voting only after backend implementation.

## Milestone 6 — Release candidate

- [ ] Test on PS Vita 1000 and 2000.
- [ ] Test low-storage and offline scenarios.
- [ ] Test missing or corrupt catalog data.
- [ ] Test interrupted scans and tracker recovery.
- [ ] Verify accessibility and reduced-motion behavior.
- [ ] Publish user guide, compatibility notes and release artifacts.

## Release philosophy

The first public release must be useful even if advanced tracking or community features are still limited. Unsupported environments should be reported accurately, not hidden behind a green checkmark.


## 🌐 Web companion progress

The static website now has a project landing page, searchable catalog browser and visual documentation portal.

- [x] Responsive landing page and navigation.
- [x] Catalog search by title, identifiers and region.
- [x] PS Vita / PSP / PS1 / Homebrew filters.
- [x] Regional release detail modal.
- [x] Homebrew icons, screenshots, source attribution and target-platform labels.
- [x] Editorial fields for game features, age ratings and external scores.
- [x] Cross-platform recommendation rendering and metadata-match ranking.
- [x] Visual documentation portal with project chapters.
- [x] Browser-side catalog logic regression tests wired into CI (editorial metadata fallback coverage).
- [x] Public catalog snapshot generated and validated: **9,732 games / 12,357 releases** on 2026-11-17.
- [ ] Add curated metadata and relation seeds for reviewed commercial titles.
- [x] Configure the GitHub Pages deployment workflow for the web companion; keep deployment health in the pre-release checklist.
- [ ] Add static ranking snapshots and public ranking methodology.
- [x] Browser-local custom lists with ordered entries and JSON backup/restore.


## Native foundation progress

The native application lives in `native/` and uses the pinned VitaSDK 2026.08 toolchain, libvita2d and SceCtrl. The current 00.10 line includes Home, offline Discover, editorial game files, personal Library, three local profiles, backups, bundled typography, lightweight transitions and a consistent PSM-inspired magazine page system across every screen. A 00.05 smoke test was confirmed on a PS Vita 1000; the 00.10 layouts still need device revalidation. Installed-title discovery and automatic session tracking remain unimplemented.



## Native catalog progress

The native Discover screen browses the shared public snapshot offline. CI generates a compact C index from `web/data/catalog.json`; the index carries the same source-backed age ratings, review scores, genres, features, themes and editorial text used by the web companion. The native app also contains its own local Library, three profiles and backups. SQLite migration, installed-title scanning and automatic playtime remain outside the current build.


### Native Discover filters

The native catalog browser now filters the shared offline index by platform and alphabetical initial. Multi-platform records match each applicable platform; Triangle cycles All / Vita / PSP / PS1 / Homebrew, L/R cycles A–Z, and Square clears both filters. A free-text virtual keyboard remains a separate future enhancement.
