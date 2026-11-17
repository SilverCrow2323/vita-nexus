# 🧱 Architecture

This document defines the initial boundaries. It is a blueprint, not a claim that every component already exists.

## System overview

~~~mermaid
flowchart TB
    subgraph Shared["Shared contracts and published data"]
        SC["Game / Release schema"]
        CAT["Catalog manifests"]
        ART["Artwork metadata"]
    end
    subgraph Native["Native PS Vita"]
        UI["Vita UI"]
        SCAN["Installation scanner"]
        REC["Recommendation engine"]
        DB[("SQLite")]
        TRACK["NEXUS Tracker plugin"]
    end
    subgraph Web["Web companion"]
        SITE["Static web application"]
        PAGES["GitHub Pages"]
    end
    subgraph Future["Optional future services"]
        API["Community API"]
        AUTH["Identity and sync"]
    end
    SC --> DB
    CAT --> DB
    ART --> DB
    SC --> SITE
    CAT --> SITE
    UI <--> DB
    SCAN --> DB
    TRACK --> DB
    REC --> DB
    SITE --> PAGES
    SITE -. optional .-> API
    API --> AUTH
~~~

## Component boundaries

| Component | Responsibility | Must not do |
|---|---|---|
| UI engine | Screens, focus, animation, input | Own catalog rules |
| Catalog engine | Canonical games, releases, metadata | Assume every release is installed |
| Installation scanner | Read local application/content metadata | Modify or relocate files |
| Release matcher | Link regional records to canonical games | Merge solely by fuzzy title |
| Artwork engine | Cache cover, hero, logo and screenshots | Claim unverified assets are official |
| Recommendation engine | Cross-platform similarity and personalization | Recommend only the current platform |
| Profile manager | Local player identity and preferences | Require an online account |
| Playtime tracker | Capture process/lifecycle events | Assume all environments expose identical events |
| Web frontend | Browse published public data | Read Vita storage or host server code |
| Community service (future) | Public votes, lists, identity and sync | Become a dependency of offline native use |

## Canonical data model

Use a canonical game entity plus one or more release records.

### Games

- **game_id** — stable internal ID;
- canonical and sort titles;
- short and long descriptions;
- genres, themes and gameplay features;
- developer, publisher and release year;
- artwork references;
- metadata sources and attribution.

### Releases

- **release_id**, **game_id**;
- platform: **vita**, **psp**, **ps1** or **homebrew**;
- region and language codes;
- title ID, content ID, disc ID and serial;
- version and package size;
- availability source and last checked time.

### Local-only entities

- **installations** — observed path, platform, identifiers and scan timestamp;
- **profiles** — local profile IDs and preferences;
- **profile_games** — status, personal score, favorites and notes;
- **sessions** — start/end timestamps, duration and source events.

### Editorial and community entities

- **age_ratings** — rating system, value, territory and descriptors;
- **external_scores** — provider, score, sample size and source URL;
- **game_ratings** — profile score and timestamps;
- **rankings** and **ranking_entries** — named lists and ordered games;
- **game_relations** — similarity links, rationale, score and provenance.

## Shared schema strategy

The native application and web frontend should consume the same versioned JSON contract. The native app may import it into SQLite; the web frontend may read the published JSON directly.

Keep data separate from presentation. Never hard-code a second independent copy of a game's metadata inside UI components.

## Data flow

1. Import catalog source files into a staging area.
2. Normalize and validate source records.
3. Match each release to a canonical game.
4. Enrich records with attributed editorial metadata and artwork references.
5. Publish a versioned static catalog snapshot.
6. Let the native app import the snapshot and reconcile it with local installations.
7. Let the web companion browse the same public snapshot.

## Tracking boundary

The tracker is a separate native plugin because playtime observation must continue while the UI is not active. It should emit small, recoverable lifecycle events. The app reconciles those events into session records.

A tracker event should include an event ID and schema version, process/application identifier when available, platform and title identifiers, event type (**start**, **suspend**, **resume**, **exit**, **recovery**) and timestamps. Identification confidence must be explicit.

Do not infer perfect session boundaries from a single process query. Validate each environment on hardware.

## Privacy and security

- Local profile data stays on the console by default.
- No credentials or personal data are published in catalog JSON.
- Web Pages contains only public project and catalog information.
- Any future cloud sync must be opt-in and authenticated.
- Community submissions require validation, rate limiting and moderation.
- Untrusted catalog strings must be escaped before rendering.


## Catalog ingestion and public web snapshot

The first source adapter imports the PS Vita, PSP and PS1 game TSV indexes published by NoPayStation. Its layout maps follow PKGj's platform-specific column positions. The importer intentionally discards package URLs, zRIF/license values and package digests.

The normalized snapshot is written to `web/data/catalog.json`, which is consumed by both the public catalog browser and, later, the native database importer. `catalog/schema/catalog.schema.json` is the versioned contract. Curated descriptions, artwork, age ratings and external scores live in `web/data/editorial-overrides.json` and must include attribution before publication.

Automatic grouping is deliberately conservative: identical normalized titles are grouped within one platform. A game appearing on multiple platforms remains separate until an explicit cross-platform relationship or curated canonical mapping is supplied.


## Homebrew source adapter

The catalog builder imports the public VitaHomebrewDB Vita and PSP indexes as a separate content kind. The canonical platform remains `homebrew`; each release carries a `targetPlatform` field so the UI can label it as homebrew for Vita or PSP. This avoids confusing homebrew tools/apps with commercial store releases.

The source adapter retains names, descriptions, author, version, category code, requirements, changelog, source project and release-page links, and source-hosted artwork references. It discards direct package URLs, payload blobs and integrity hashes. Artwork references stay on the upstream host with explicit credit and ownership caveats.


## Native shell implementation status

The initial native shell is a small C application built with VitaSDK CMake helpers. It renders a controller-driven module selector with libvita2d and the system PGF font. It is deliberately isolated from catalog and storage code: no filesystem scan, save access, package install, network request or tracker hook is performed by this shell. The next native slices should add versioned catalog loading and SQLite migration before any installed-title scanner is allowed to inspect console metadata.
