# Native Vita foundation

This directory contains the native VITA NEXUS application: an offline catalog browser, a local personal tracker and a three-slot profile manager. It deliberately does not scan installed titles or touch game save data.

## Toolchain baseline

- VitaSDK channel: **2026.08**
- Build system: CMake + VitaSDK's official toolchain and packaging helpers
- Rendering: libvita2d
- Input: SceCtrl digital controls
- Package: safe SELF wrapped in a VPK
- Title ID: VNXS00001

The 2026.08 channel is pinned deliberately for reproducible CI. Upgrade it as a reviewed toolchain change, not by silently switching to the moving latest tag.

## Build with the official container

From the repository root:

```sh
docker run --rm -v "$PWD:/workspace" -w /workspace vitasdk/vitasdk:2026.08 \
  sh -lc 'vdpm install libvita2d && cmake -S native -B native/build -DCMAKE_TOOLCHAIN_FILE="$VITASDK/share/vita.toolchain.cmake" && cmake --build native/build --parallel 2'
```

Expected output: native/build/vita_nexus_native.vpk.

## Controls

- D-pad up/down: select a module or move through the current list.
- START: exit.
- LIBRARY: X opens a matching catalog entry; L/R cycles All, Favorites, Backlog and Completed; Square removes an entry after confirmation.
- DISCOVER: X opens the selected game's metadata sheet; L/R cycles alphabetical initials; Triangle cycles platforms; Square clears filters.
- Game detail: L toggles Favorite, R toggles Backlog, Triangle toggles Completed, Left/Right adjusts the personal score (0–10).
- PROFILES: X activates a slot, Triangle exports its backup, Square starts a restore confirmation; X confirms and Circle cancels.
- Circle returns to the previous screen. D-pad wraparound is covered by a host-side C regression test.

## Safety boundary

The app reads controller input and writes only its own small profile, library and backup files under the `ux0:data/` namespace. It does not scan installed titles, modify game saves, inspect license material or connect to a service. The owner has confirmed the native application launches and works on a PS Vita 1000 (17 November 2026); other hardware/firmware combinations still need their own validation.


## Host-side menu test

Run without VitaSDK:

```sh
cc -std=c11 -Wall -Wextra -Werror -pedantic -Inative/src tests/native_menu_state_test.c -o /tmp/vita-nexus-menu-test
/tmp/vita-nexus-menu-test
```

The quality workflow runs the same test. This checks navigation state only; it does not emulate Vita hardware or certify rendered input.


## VPK integrity gate

Before upload, the native build workflow runs `tools/verify_vpk.py` against the generated package. It checks ZIP integrity, required `eboot.bin` and `sce_sys/param.sfo` entries, the expected nine-character Title ID, unsafe paths, duplicate entries, symlinks, encrypted entries, oversized archives and forbidden game/license payloads. It records the package SHA-256 in the Actions summary. The verifier itself has host-side regression tests in `tests/test_verify_vpk.py`.


## Native catalog and Discover screen

The native build consumes the same `web/data/catalog.json` snapshot as the web companion. Before the VitaSDK container build, `tools/catalog-builder/build_native_index.py` validates schema version `1.0.0` and emits `native/generated/catalog_index.c`. The generated file is a compact, read-only index of game ID, title, platform bitmask and release count; it is not a second hand-maintained catalog and it contains no package URLs, license material or payloads.

The generated index is compiled into the VPK, so Discover works offline. On the home screen select DISCOVER and press Cross; use Up/Down to browse and Circle to return. The list shows catalog titles, platform labels, game IDs and release counts. Rankings and Settings remain planned modules.

The generated C file is a build output and is intentionally not committed. Run the generator before configuring CMake. CI tests the generator, compiles the Vita binary, verifies the VPK and uploads a temporary artifact. This is still a build-level validation, not a physical-console certification.


## Native game detail sheet

The Discover browser opens a magazine-style metadata sheet for the selected catalog entry. Alongside platform coverage, release count, year, developer, publisher, genres and editorial summary, the sheet can show the content kind, age guidance (preferring PEGI when supplied), rating descriptors, an external score with its provider and vote count, plus feature/theme tags.

Every field is read from the same shared JSON snapshot. The native index builder bounds optional text and ignores invalid score shapes; missing ratings and scores remain explicitly unavailable. This screen does not claim emulator compatibility, launch games, inspect installed content or access save data. The personal tracker remains a separate, local-only score and status.


## Discover filters

The offline catalog browser supports two independent filters without allocating a second copy of the catalog:
- **Platform:** Triangle cycles All, PS Vita, PSP, PS1 and Homebrew. Multi-platform records remain visible in every matching platform filter.
- **Initial:** L/R cycles A–Z. Titles are matched case-insensitively against their first character.
- **Reset:** Square restores All platforms and All initials.

The result counter reports visible matches against the complete catalog. D-pad navigation and the game detail sheet operate on the filtered result set. Search is currently filter-based rather than free-text; a virtual keyboard is a separate UX milestone.


## Native personal Library (00.05)

The native Library stores a personal collection in the app's own data namespace. The first profile retains the original `ux0:data/VitaNexus-library-v1.dat` path so existing 00.05 users keep their collection. Profiles 2 and 3 use separate app-owned files. It does not enumerate storage, inspect installed titles, touch game saves, read licenses or modify system files.

From a game's Discover detail sheet:
- **L trigger** toggles Favorite.
- **R trigger** toggles Backlog.
- **Triangle** toggles Completed.
- **Left / Right** adjusts a personal score from 0 to 10 (0 means unrated).
- Changes are written immediately to a temporary sibling file and committed with a rename.

The versioned `VNXLIB|1` format is bounded to 512 records, validates stable game IDs and field ranges, rejects duplicate IDs and malformed/checksum-invalid files, and loads into a temporary in-memory structure before replacing live state. Unknown catalog IDs are retained so a catalog update does not silently erase a user's data. Host tests cover round-trip persistence, toggles/removal, ratings, unknown IDs and corruption handling.

The Library screen lists locally tracked records. Selecting an entry opens its catalog detail when that game ID is present in the current catalog. This milestone is a local tracker, not an installed-title scanner, cloud sync or multi-profile system. CI builds and verifies the VPK; a real PS Vita test is still required.


## Profiles, collection tools and backups (00.06)

The home screen now has a dedicated **PROFILES** module with three local slots: Player 01, Player 02 and Player 03. Each slot has an independent Library file. The active slot is remembered in `ux0:data/VitaNexus-active-profile-v1.dat`; Player 01 reuses the original 00.05 data file to preserve an existing collection during the upgrade.

The profile manager shows per-slot totals, favorites, backlog, completed and rated counts. Press **X** to activate the selected slot. Empty slots start clean; switching back restores their own data.

Each slot has a separate backup file under `ux0:data/`. Press **Triangle** to export the selected profile. Press **Square** to request a restore, then **X** to confirm or **Circle** to cancel. Import validates the complete versioned data and checksum before replacing the profile file. A corrupt or missing backup never partially replaces the profile. Restore is explicit because it overwrites that slot.

The Library screen now has four views: All, Favorites, Backlog and Completed (L/R). Press **Square** on an entry to request removal, then **X** to confirm or **Circle** to cancel. Per-profile statistics stay local and are not uploaded.

The profile selector, profile metadata, export/import and statistics helpers are covered by host-side C tests. The native package version is **00.06**. A device-level smoke test was confirmed on the owner's PS Vita 1000 on 17 November 2026; CI remains the repeatable build and integrity gate.


## Native UI typography and layout refresh (00.07)

The 960×544 native interface now uses **Barlow Condensed** instead of relying on the system PGF face for the whole UI. The bundled Medium and Bold TrueType files are loaded through libvita2d's FreeType font API; medium is used for interface copy and metadata, while bold is reserved for major headings and active navigation. The original PGF renderer remains available as a fallback for missing font assets and non-ASCII catalog text.

The font files and their SIL Open Font License 1.1 ship inside the VPK at `app0:assets/fonts/`, so rendering stays offline. Their upstream source is pinned to the Google Fonts snapshot at commit `89f5431ff0db41bd2fe3f7ba21a723a01622428b`; the license source is also included at `native/assets/fonts/OFL.txt`. The VPK integrity gate requires both font files and the license text, and checks their SFNT signatures before upload.

The layout uses a consistent 48 px content gutter and more deliberate spacing between list rows, information panels and footer controls. Font sizes are mapped to real pixel sizes, long game titles and metadata are fitted to their available column widths, and editorial summaries wrap by measured text width rather than an arbitrary character count. The Library row height has been increased slightly to use the 544 px screen more evenly.

The new VPK is **00.07**. CI verifies the font assets are packaged and the text-rendering code compiles; this version still needs a quick device check on the PS Vita 1000 before calling the visual refresh hardware-verified.


## Native transitions and micro-motion (00.08)

The native UI now uses a small, deterministic motion helper with no dynamic allocation and no external runtime dependency. Entering a different screen fades a dark overlay away over eight rendered frames; this keeps navigation feedback visible without adding expensive blur, scaling or texture effects. The first home screen also fades in on startup.

The selected row in Home, Discover, Library and Profiles receives a short animated underline. Its width follows a bounded triangular pulse over a 48-frame cycle. Discover selection rendering uses the filtered-list position consistently for both the highlight and the title color, so the active row stays visually aligned when platform or alphabetical filters are enabled.

The transition and pulse calculations live in `native/src/ui_motion.h` and are covered by a VitaSDK-independent host test. The package version is **00.08**. Effects are intentionally limited to a few rectangles and integer calculations to keep the UI responsive on real hardware.


## Editorial game cards (00.09)

The native catalog index now carries the editorial fields already defined by schema 1.0.0: content kind, features, themes, age ratings and external scores. The index builder selects PEGI first for the European audience, then falls back to the first supported classification; it never fabricates a default age rating. External scores retain their provider, source, scale and vote count. Scores without a known scale are displayed without an invented denominator.

The game sheet groups this information into compact panels with platform coverage, release metadata, age guidance, review score, developer/publisher, genres, feature tags and the editorial note. The shared offline catalog remains the only source of truth. The VPK version is **00.09**; the expanded metadata layout requires a fresh hardware visual check.
