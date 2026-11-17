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
- [ ] Choose and pin the VitaSDK/toolchain baseline.
- [ ] Establish CI for formatting and static checks.

## Milestone 1 — Native proof of concept

- [ ] Build a minimal VPK.
- [ ] Render the first native screen.
- [ ] Handle D-pad, analog and touchscreen input.
- [ ] Create and migrate the local SQLite database.
- [ ] Scan a supported set of PS Vita application metadata.
- [ ] Validate read-only behavior on hardware.

**Exit gate:** a reproducible VPK build, stable input/rendering and a scanner that reports findings without modifying files.

## Milestone 2 — Catalog foundation

- [x] Define and version the canonical game/release schema.
- [x] Add the first metadata-only NoPayStation TSV source adapter and provenance tracking.
- [ ] Import Vita, PSP, PS1 and homebrew catalog data.
- [ ] Implement region and edition matching.
- [ ] Validate duplicate and ambiguous releases.
- [x] Add the static catalog snapshot contract and web browser; first live import remains to be run.

**Exit gate:** the live source import must pass validation and the same canonical records must be consumed by the native database importer and web frontend.

## Milestone 3 — Discovery experience

- [ ] Animated Home Hero.
- [ ] Library and Discover views.
- [ ] Store index browser and regional release grouping.
- [ ] Game details with attributed artwork.
- [ ] Cross-platform "You may also like" recommendations.
- [ ] Game Generator categories and explanations.

**Exit gate:** recommendations are explainable, cross-platform and based on available data rather than invented metadata.

## Milestone 4 — Personal gaming

- [ ] Multiple local profiles.
- [ ] Favorites, personal scores and backlog states.
- [ ] Session/event journal.
- [ ] Tracker proof of concept for native Vita games.
- [ ] Validate suspend/resume and crash recovery.
- [ ] Investigate and test PSP/Adrenaline and PS1/POPS tracking individually.
- [ ] Personal statistics and session correction.

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
- [ ] Enable GitHub Pages in repository settings and confirm a successful deploy.
- [ ] Add static ranking snapshots and public ranking methodology.
- [x] Browser-local custom lists with ordered entries and JSON backup/restore.
