<div align="center">

# ✦ VITA NEXUS / FIELD MANUAL

**The backlog never sleeps. The documentation shouldn't either.**

[OPEN THE LIVE WEB PORTAL ↗](https://silvercrow2323.github.io/vita-nexus/) · [Browse the game catalog](https://silvercrow2323.github.io/vita-nexus/catalog.html) · [Field Notes](https://silvercrow2323.github.io/vita-nexus/docs.html)

<sub>SPDW FACTORY · VITA NEXUS DOCUMENTATION INDEX</sub>

</div>

---

> **Read this first:** VITA NEXUS has two connected surfaces. The native application is designed for offline PS Vita use; the website is a static public portal and catalogue. A live website does not imply that backend-dependent community services exist.

## 🗺️ Choose your route

| Looking for… | Start here |
|---|---|
| Product goals and non-goals | [Product vision](PRODUCT-VISION.md) |
| Runtime ownership and data boundaries | [Architecture](ARCHITECTURE.md) |
| Website, GitHub Pages and backend boundary | [Web companion](WEB-COMPANION.md) |
| Editorial reviews, scores and source policy | [Editorial content](EDITORIAL-CONTENT.md) |
| Covers, screenshots, credits and rights | [Artwork management](ARTWORK-MANAGEMENT.md) |
| Writing and graphic language | [Documentation style guide](DOCUMENTATION-STYLEGUIDE.md) |
| Milestones and delivery order | [Roadmap](ROADMAP.md) |
| Alpha and release acceptance gates | [Pre-release checklist](PRE-RELEASE-CHECKLIST.md) |
| NPS metadata import | [NPS catalog import](NPS-CATALOG-IMPORT.md) |
| Cross-platform recommendations | [Recommendation engine](CROSS-PLATFORM-RECOMMENDATIONS.md) |

## 🌐 The live site

**[https://silvercrow2323.github.io/vita-nexus/](https://silvercrow2323.github.io/vita-nexus/)**

- [Home / project portal](https://silvercrow2323.github.io/vita-nexus/)
- [Catalog browser](https://silvercrow2323.github.io/vita-nexus/catalog.html)
- [Field Notes / web documentation](https://silvercrow2323.github.io/vita-nexus/docs.html)

GitHub Pages publishes the `web/` directory through `.github/workflows/pages.yml`. The site is static HTML, CSS, JavaScript and versioned JSON. It can browse published data and store personal lists in the current browser, but public accounts, voting and cloud sync require a separate backend.

## 🧭 Native vs web

| Surface | Owns | Does not claim |
|---|---|---|
| Native PS Vita app | Offline discovery, local Library, local profiles and on-device UI | Cloud sync or automatic installed-title tracking until implemented |
| Web portal | Public catalogue, editorial pages, discovery and browser-local lists | A server-side account system or shared community database |
| Documentation | Product contracts, design rules, architecture and release gates | A substitute for hardware validation |

## 🧪 Trust and release language

- **Implemented** means source exists; state separately whether it has passed tests or device checks.
- **Needs testing** means a build exists but the real PS Vita acceptance pass is pending.
- **Planned** means it is not implemented.
- Never invent scores, age ratings, compatibility claims, release metadata or artwork rights.
- Keep game artwork attribution and rights status beside the assets that use it.

## 🏭 Maintainer links

- [Repository](https://github.com/SilverCrow2323/vita-nexus)
- [Issues](https://github.com/SilverCrow2323/vita-nexus/issues)
- [Releases](https://github.com/SilverCrow2323/vita-nexus/releases)
- [Wiki / repository docs](https://github.com/SilverCrow2323/vita-nexus/tree/main/docs)
- [Web source](https://github.com/SilverCrow2323/vita-nexus/tree/main/web)
- [Native application](https://github.com/SilverCrow2323/vita-nexus/tree/main/native)

---

<div align="center">

**VITA NEXUS · SPDW FACTORY**

*Good games don't stay buried. Good docs don't either.*

</div>
