# 🌐 Web companion

The website is a first-class VITA NEXUS surface, not a second native app.

## Hosting choice

The initial web companion is published with **GitHub Pages** from the **web/** directory using GitHub Actions.

Expected project URL:

**https://silvercrow2323.github.io/vita-nexus/**

The workflow publishes the contents of web/ to this project URL. A successful quality check is not itself a Pages deployment; use the Pages workflow result to confirm the published site. When the metadata sync workflow completes successfully on main, it also triggers a follow-up quality run, native VPK build and Pages deployment against the refreshed catalog snapshot.

## Catalog browser

The first browser route is `catalog.html`. It reads `data/catalog.json` and `data/editorial-overrides.json`, and supports title/identifier/region search, platform filters, sorting, regional release details and progressive loading.

The public JSON contract is defined in `catalog/schema/catalog.schema.json`. The Homebrew filter includes separate Vita and PSP target labels, with upstream author, version, requirements, source links and screenshot references where supplied. The metadata-only source importer is documented in [NPS catalog import](NPS-CATALOG-IMPORT.md).

## What Pages can host

GitHub Pages serves static HTML, CSS, JavaScript and build output. That is ideal for a polished project landing page, documentation and release notes, a read-only catalog backed by versioned JSON, browser-side search and filters, game detail pages, and static public ranking snapshots.

It is not an application server. Do not place API secrets, private user data or credentials in the published site.

## What needs a separate backend

| Feature | Static Pages alone? | Future approach |
|---|---:|---|
| Browse catalog JSON | ✅ | Static data files |
| Search and filter | ✅ | Browser-side index |
| Read public rankings | ✅ | Published JSON snapshots |
| Save a list in the current browser | ✅ | Local browser storage / JSON export-import |
| Public accounts and profiles | ❌ | Authenticated backend |
| Submit and vote on public lists | ❌ | API, validation and moderation |
| Cloud sync with a Vita | ❌ | Opt-in sync service |
| Read Vita memory-card contents | ❌ | Native app only |

The planned native application must remain fully useful offline. A web sign-in must never be required for local profiles or playtime tracking.

## Initial deployment

The workflow is stored at **.github/workflows/pages.yml**. It publishes only the contents of **web/** when that directory or the workflow changes on **main**.

To activate it:

1. Open the repository's **Actions** tab.
2. Select **Deploy VITA NEXUS Web** and inspect the latest run.
3. Confirm the deployment job completed and open its published URL.
4. If deployment is blocked, check **Settings → Pages** and ensure **GitHub Actions** is selected as the source.

## Web experience roadmap

### Phase 1 — Project portal and catalog shell

The responsive landing page, underground cyberpunk manga design system, searchable catalog browser and editorial game sheets are implemented. The public metadata snapshot was generated and validated on 17 November 2026: 9,732 canonical games, 12,357 releases and 4,466 Vita/PSP homebrew entries.

### Phase 2 — Enriched catalog

The game-sheet renderer now supports compact PEGI / age-classification and external-score stamps, plus a PSM-inspired editorial review spread. Curated entries can publish a standfirst, original review, NEXUS score, verdict, strengths, caveats, back-page curiosities and byline/source links through the separate [editorial content contract](EDITORIAL-CONTENT.md). The current override file intentionally contains no fabricated reviews; publish each entry only after its copy and sources have been checked.

Artwork, screenshots, verified developers/publishers, richer age classifications and cross-platform relationships continue to expand separately. Each item must retain attribution and usage terms.

### Phase 3 — Rankings and personal lists

Publish static global charts and support browser-local custom lists with JSON export/import. Native profiles and local playtime remain separate from website state.

### Phase 4 — Community services

Only after a backend is selected: public accounts, submissions, voting, moderation and optional cross-device sync.

## Performance and accessibility

- Mobile-first responsive layouts.
- Keyboard and touch navigation.
- Semantic HTML and visible focus.
- Reduced-motion support.
- Lazy-loaded artwork with explicit dimensions.
- Search/filter should remain useful without external scripts or third-party trackers.
- Keep the initial payload small; publish artwork separately from metadata where practical.


## Browser-local personal lists

The catalog includes a local list workspace. Visitors can create named collections, add games from their detail sheets, reorder entries, remove games, and export/import a versioned JSON backup. Data is stored in that browser's local storage; it is not uploaded, shared or synchronized between devices. Import appends lists rather than overwriting existing collections. This is deliberately not presented as community voting or public rankings: those require a separate backend and moderation.
