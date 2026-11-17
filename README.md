# ✦ VITA NEXUS

<p align="center"><strong>YOUR PLAYSTATION UNIVERSE. CONNECTED.</strong><br>A discovery-first companion for PS Vita, PSP, PS1 and homebrew.</p>

<p align="center">
  <img alt="Status" src="https://img.shields.io/badge/status-early%20development-8b7cff?style=flat-square">
  <img alt="Platforms" src="https://img.shields.io/badge/platforms-PS%20Vita%20%7C%20PSP%20%7C%20PS1-41c7b7?style=flat-square">
  <img alt="Web" src="https://img.shields.io/badge/web-GitHub%20Pages-20232a?style=flat-square">
</p>

> [!NOTE]
> **Foundation phase.** This repository currently contains the product blueprint, documentation and first web companion shell. The native app, scanner, catalog engine and tracker are not implemented yet.

## 🌌 The idea

VITA NEXUS is an **offline-first gaming companion**, not a replacement launcher. It brings four corners of the PlayStation portable ecosystem into one organized place: discover games, understand your collection, find unexpected recommendations, track playtime and build personal rankings.

| Native · PS Vita | Web · GitHub Pages |
|---|---|
| Installed-game scanner | Public project hub |
| Automatic playtime tracking | Browseable catalog experience |
| Local multi-user profiles | Public rankings and community lists* |
| Offline recommendations | Game pages and discovery |
| On-device library and statistics | Documentation and releases |

\* Public accounts, voting and cloud sync require a separate backend. GitHub Pages is static hosting, not an application server.

## ✨ Product pillars

- **HOME** — an animated editorial dashboard with useful recommendations.
- **GENERATOR** — playful modes such as *Chaos Pick*, *Hidden Treasure*, *Retro Detour* and *Backlog Punishment*.
- **LIBRARY** — installed PS Vita, PSP and PS1 titles.\n- **HOMEBREW** — Vita and PSP homebrew apps, tools and ports with source attribution.
- **DISCOVER** — explore the wider catalog, including games not installed.
- **STORE** — indexed releases and regional variants. An information layer, not a commercial package host.
- **GAME PAGES** — descriptions, artwork, screenshots, age ratings, feature indicators and cross-platform recommendations.
- **RANKINGS** — global charts, personal scores, custom lists and later community submissions.
- **PROFILES** — separate local histories, preferences, favorites and statistics.
- **PLAYTIME** — automatic session tracking where reliable lifecycle signals exist.

## 🧭 Project map

| Area | What lives here |
|---|---|
| [Product vision](docs/PRODUCT-VISION.md) | Goals, audiences and experience principles |
| [Architecture](docs/ARCHITECTURE.md) | Components, boundaries and data contracts |
| [Web companion](docs/WEB-COMPANION.md) | Pages scope, deployment and backend boundary |
| [Documentation style](docs/DOCUMENTATION-STYLEGUIDE.md) | Visual and editorial language |
| [Roadmap](docs/ROADMAP.md) | Milestones and release gates |
| [NPS catalog import](docs/NPS-CATALOG-IMPORT.md) | Metadata-only TSV ingestion, homebrew source and regional grouping |\n| [Recommendation engine](docs/CROSS-PLATFORM-RECOMMENDATIONS.md) | Cross-platform similarity signals and ranking behavior |\n| [Web catalog](web/catalog.html) | Searchable PS Vita / PSP / PS1 release browser |\n| [Web preview](web/index.html) | Project landing page |

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

VITA NEXUS should feel like a premium game-discovery magazine: cinematic hero artwork, confident typography, precise metadata, playful category names and motion that never gets in the way.

The product UI is **English**. Project documentation is also written in English for consistency and accessibility.

## 🚦 Current status

| Capability | Status |
|---|---|
| Product direction and requirements | 🟢 Defined |
| Repository and documentation foundation | 🟡 In progress |
| GitHub Pages shell | 🟡 First scaffold |
| Native Vita application | ⚪ Not started |
| Catalog import and release matching | ⚪ Not started |
| Installed-game scanner | ⚪ Not started |
| Automatic tracker plugin | ⚪ Not started |
| Rankings and community backend | ⚪ Future phase |

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

<p align="center"><strong>VITA NEXUS</strong><br><sub>Find your next favorite. Rediscover the ones you forgot.</sub></p>
