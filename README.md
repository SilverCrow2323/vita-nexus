# ✦ VITA NEXUS

<p align="center"><strong>THE BACKLOG NEVER SLEEPS.</strong><br>PS Vita, PSP, PS1 and homebrew. One place to dig up your next obsession.</p>

<p align="center">
  <img alt="Status" src="https://img.shields.io/badge/status-early%20development-8b7cff?style=flat-square">
  <img alt="Platforms" src="https://img.shields.io/badge/platforms-PS%20Vita%20%7C%20PSP%20%7C%20PS1-41c7b7?style=flat-square">
  <img alt="Web" src="https://img.shields.io/badge/web-GitHub%20Pages-20232a?style=flat-square">
</p>

> [!NOTE]
> **The web companion and native Vita alpha foundation are implemented.** The native VPK now includes an offline catalog browser, game detail sheets, a personal Library and three local profiles with backups. An installed-title scanner, SQLite runtime integration, automatic playtime and community sync remain separate future milestones. A versioned SQLite schema and host-only migration harness are now defined, but the Vita app still uses its existing local file stores. The earlier 00.05 baseline was confirmed on a PS Vita 1000; the 00.10 magazine-page redesign is now built in CI and needs a fresh device visual check.

## 🧨 The idea

VITA NEXUS is an **offline-first gaming companion**, not a replacement launcher. It brings four corners of the PlayStation portable ecosystem into one organized place: discover games, understand your collection, find unexpected recommendations, track playtime and build personal rankings.

| Native · PS Vita | Web · GitHub Pages |
|---|---|
| Offline catalog + personal tracker | Public project hub |
| Local profiles and backups | Browseable catalog experience |
| Installed-title scanner (future) | Public rankings and community lists* |
| Automatic playtime (future) | Game pages and discovery |
| On-device library and statistics | Documentation and releases |

\* Public accounts, voting and cloud sync require a separate backend. GitHub Pages is static hosting, not an application server.

## ✨ Product pillars

- **HOME** — an animated editorial dashboard with useful recommendations.
- **GENERATOR** — playful modes such as *Chaos Pick*, *Hidden Treasure*, *Retro Detour* and *Backlog Punishment*.
- **LIBRARY** — a personal game tracker with favorites, backlog, completion and local scores; installed-title scanning remains future work.
- **HOMEBREW** — Vita and PSP homebrew apps, tools and ports with source attribution.
- **DISCOVER** — explore the wider catalog, including games not installed.
- **STORE** — indexed releases and regional variants. An information layer, not a commercial package host.
- **GAME PAGES** — descriptions, artwork, screenshots, age ratings, feature indicators and cross-platform recommendations.
- **RANKINGS** — global charts, personal scores, custom lists and later community submissions.
- **PROFILES** — three local collections with independent stats and portable backup/restore.
- **PLAYTIME** — automatic session tracking where reliable lifecycle signals exist.

## 🧭 Project map

| Area | What lives here |
|---|---|
| [Product vision](docs/PRODUCT-VISION.md) | Goals, audiences and experience principles |
| [Architecture](docs/ARCHITECTURE.md) | Components, boundaries and data contracts |
| [Web companion](docs/WEB-COMPANION.md) | Pages scope, deployment and backend boundary |
| [Editorial content](docs/EDITORIAL-CONTENT.md) | Review pages, scores, bylines and source rules |
| [Artwork management](docs/ARTWORK-MANAGEMENT.md) | Covers, screenshots, rights status and image credits |
| [Documentation style](docs/DOCUMENTATION-STYLEGUIDE.md) | Visual and editorial language |
| [Roadmap](docs/ROADMAP.md) | Milestones and delivery gates |
| [Pre-release checklist](docs/PRE-RELEASE-CHECKLIST.md) | Web alpha and native Vita release gates |
| [NPS catalog import](docs/NPS-CATALOG-IMPORT.md) | Metadata-only TSV ingestion, homebrew source and regional grouping |
| [Recommendation engine](docs/CROSS-PLATFORM-RECOMMENDATIONS.md) | Cross-platform similarity signals and ranking behavior |
| [Web catalog](web/catalog.html) | Searchable PS Vita / PSP / PS1 release browser |
| [Web preview](web/index.html) | Project landing page |
| [Native Vita foundation](native/README.md) | VitaSDK baseline, VPK build and shell status |

## 📚 Public catalog snapshot

The metadata-only catalog build currently contains:

| Collection | Entries |
|---|---:|
| Canonical game records | **9,795** |
| Release records | **12,420** |
| Vita + PSP homebrew entries | **4,529** |

<sub>Snapshot generated at 2026-11-21 12:37:41 UTC (schema 1.0.0). See the [catalog snapshot manifest](docs/CATALOG-SNAPSHOT-MANIFEST.md) for source counts and SHA-256 provenance.</sub>

Browse the [catalog interface](web/catalog.html) or inspect the [raw catalog snapshot](web/data/catalog.json).

### Personal lists — local-first
The web catalog also supports private browser-local collections: create lists, add titles from game sheets, reorder or remove entries, and export/import versioned JSON backups. Lists stay in the current browser and are never uploaded or shared. Public community rankings and voting remain a separate future backend feature.

## 🧩 Architecture at a glance

~~~mermaid
flowchart LR
    C["Shared catalog & metadata"] --> N["Native PS Vita app"]
    C --> W["Web companion"]
    N --> S["Local SQLite"]
    N --> T["NEXUS Tracker plugin"]
    W --> P["Static GitHub Pages"]
    P -. "future API integration" .-> B["Optional community backend"]
~~~

The native app owns console access and local play data. The website consumes published catalog data. A future community service may handle public accounts, votes and shared lists; it remains optional and separate.

## 🎨 Design language

VITA NEXUS looks like an underground cyberpunk manga zine: rough ink, halftone dots, ripped-poster edges, loud condensed type and electric pink, acid yellow, cyan and violet. It should feel printed, pasted and a little dangerous—not like a generic SaaS dashboard.

Keep the interface readable. Use short labels, plain English and direct copy. No corporate filler, fake urgency or claims that a planned feature already works. The product UI and project documentation stay in **English**.

## 🚦 Current status

| Capability | Status |
|---|---|
| Product direction, architecture and visual system | 🟢 Defined |
| Cyberpunk manga web portal and catalog browser | 🟢 Implemented |
| NPS + Vita/PSP homebrew metadata pipeline | 🟢 Implemented; metadata-only |
| Public catalog snapshot | 🟢 9,795 games · 12,420 releases · 4,529 homebrew entries |
| Regional editions and release details | 🟢 Implemented |
| Cross-platform recommendation engine | 🟢 Implemented; curated editorial coverage is growing |
| Native PS Vita application / VPK | 🟢 Implemented · native 00.10 (magazine redesign; visual recheck pending) |
| Offline Discover + metadata detail sheets | 🟢 Implemented · shared catalog snapshot |
| Local Library, 3 profiles and backups | 🟢 Implemented · app-owned files only |
| Installed-title scanner / SQLite migration | ⚪ Not implemented |
| Automatic playtime tracker | ⚪ Not implemented |
| Community accounts, votes and cloud sync | ⚪ Future backend phase |

## 🛠 Development principles

1. **Offline first:** native essentials work without a network connection.
2. **Read-only discovery:** scanning never moves, renames or modifies game files.
3. **Honest metadata:** ratings, classifications, images and availability need provenance.
4. **One game, many releases:** group regional editions without losing release identifiers.
5. **Cross-platform by design:** recommendations connect PS Vita, PSP and PS1.
6. **Modular and testable:** scanner, catalog, tracker, UI and storage evolve independently.
7. **No false promises:** unknown compatibility stays clearly marked as unknown.

## 🤝 Contributing

The contribution workflow will be documented as implementation begins. Feature proposals and bug reports can be opened through [GitHub Issues](https://github.com/SilverCrow2323/vita-nexus/issues).

---

<p align="center"><strong>VITA NEXUS</strong><br><sub>Good games don’t stay buried.</sub></p>
