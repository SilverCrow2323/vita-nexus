# 🌌 Product vision

> **VITA NEXUS is a companion to the PlayStation Vita ecosystem: part personal library, part game encyclopedia, part discovery engine.**

It is not intended to replace the PS Vita LiveArea, act as a universal launcher or distribute commercial game files.

## The promise

A player should be able to answer five questions from one place:

1. **What do I own or have installed?**
2. **What should I play next?**
3. **What is this game like, and what else feels similar?**
4. **How much have I played, and what have I enjoyed?**
5. **What hidden gems am I missing across PS Vita, PSP and PS1?**

## Two coordinated experiences

### 🎮 Native application

The native application is the console-aware product. It discovers installed content, maintains local profiles and play sessions, presents the catalog offline, reads tracker events and stores a local database and artwork cache.

### 🌐 Web companion

The web companion is the public front door. Its first responsibilities are to explain the project, expose documentation and releases, and grow into a browseable read-only catalog. Public rankings and community submissions can follow when a backend exists.

The web companion cannot inspect a PS Vita memory card or local database directly. Native/web transfer or cloud synchronization must be explicit—for example, through an exported data bundle or a future authenticated sync service.

## Data provenance and accuracy

Every imported game/release fact must retain the exact provider and dataset pair that supplied it. A provider may expose multiple datasets, but a provider/dataset pair must be unique in a snapshot. Missing metadata is omitted or marked unknown; titles, regions, release identifiers, ratings and compatibility facts must never be guessed. The native index and public catalogue must consume the same validated source-pair contract.

Automated catalogue validation proves structure and provenance references, not that an upstream provider is complete or that a title is legally redistributable. Source URLs, retrieval time, dataset identity and generated snapshot hashes remain visible to maintainers.

## Product principles

| Principle | What it means in practice |
|---|---|
| Offline-first | Core native features work without an internet connection. |
| Discovery-first | The home screen gives useful recommendations immediately. |
| Cross-platform | Similarity and discovery span PS Vita, PSP and PS1. |
| Transparent | Data sources, uncertainty and availability are visible. |
| Personal | Profiles keep preferences, ratings and play history separate. |
| Respectful | Scanning is read-only; no license circumvention or commercial file hosting. |
| Accessible | High contrast, scalable layouts, reduced motion and clear labels. |

## The home screen

The first screen should feel alive but remain useful. It combines an editorial hero, a personalized recommendation, rotating discovery prompts, recently played games and quick entry points into Generator, Library, Discover and Store.

Motion is purposeful: staggered entry, subtle artwork depth, focus transitions and responsive cards. No animation should block input or make reading difficult.

## Game pages

Every canonical game page may include cover, hero artwork, logo, screenshots, summary and long description, release variants by platform and region, genres, developer, publisher, year, game features, age ratings, user and external scores, installation state, playtime and **You may also like…** recommendations across PS Vita, PSP and PS1.

Missing facts must be omitted or marked unknown rather than guessed.

## Recommendations

The engine combines gameplay loop, mechanics, genre, combat model, progression, tone, setting, art direction, narrative themes, developer relationships and profile preferences.

Every recommendation should explain why it appeared. The engine must avoid showing the same small set of games repeatedly.

## Rankings and community lists

Rankings are split into global charts, platform-specific charts, personal rankings, custom lists and future public community rankings.

A rating is not the same as a ranking. Show vote count, rating source and confidence context. External review scores must never be blended into the community score without clear labeling.

## Out of scope for the first release

- Replacing the LiveArea or launching every game format.
- Hosting or distributing commercial game packages.
- Bypassing licenses, encryption or platform security.
- Mandatory cloud accounts.
- Real-time social feeds or chat.
- Claiming automatic playtime support for environments that have not been tested.

## Definition of success

VITA NEXUS succeeds when a player can open it, discover something worth playing, understand why it was recommended and manage their collection without needing an internet connection.
