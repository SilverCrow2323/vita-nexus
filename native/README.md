# Native Vita foundation

This directory contains the first native VITA NEXUS shell. It is intentionally small: it proves the VitaSDK CMake/VPK pipeline and gives the project a real controller-driven entry point without pretending that the library scanner, catalog database, profiles or tracker already exist.

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

- D-pad up/down: select a module.
- START: exit.
- X on DISCOVER opens the selected game's metadata sheet; X on other modules remains inactive.
- Circle returns from a game sheet to results, then from results to the module selector.
- L/R cycles the alphabetical initial filter; Triangle cycles platform filters; Square clears filters.
- D-pad wraparound is implemented in a VitaSDK-independent state helper and covered by a host-side C regression test.

## Safety boundary

This shell only renders a screen and reads controller input. It does not scan storage, write to app folders, modify saves, inspect license material or connect to a service. A successful CI package is not hardware certification. Install and test on a real console before claiming compatibility.


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

The generated index is compiled into the VPK, so Discover works offline. On the home screen select DISCOVER and press Cross; use Up/Down to browse and Circle to return. The list shows catalog titles, platform labels, game IDs and release counts. The Library, Rankings and Settings modules remain explicitly incomplete.

The generated C file is a build output and is intentionally not committed. Run the generator before configuring CMake. CI tests the generator, compiles the Vita binary, verifies the VPK and uploads a temporary artifact. This is still a build-level validation, not a physical-console certification.


## Native game detail sheet

The Discover browser can open a metadata detail sheet for the selected catalog entry. It shows platform coverage, release count, year, developer, publisher, genres, stable game ID and a short editorial summary when those fields exist. Missing values are labelled as unavailable instead of guessed.

The detail fields are generated from the same shared JSON snapshot. The index builder bounds optional text fields to keep the native binary predictable. The screen is read-only; it does not launch games, inspect installed content or access save data.


## Discover filters

The offline catalog browser supports two independent filters without allocating a second copy of the catalog:
- **Platform:** Triangle cycles All, PS Vita, PSP, PS1 and Homebrew. Multi-platform records remain visible in every matching platform filter.
- **Initial:** L/R cycles A–Z. Titles are matched case-insensitively against their first character.
- **Reset:** Square restores All platforms and All initials.

The result counter reports visible matches against the complete catalog. D-pad navigation and the game detail sheet operate on the filtered result set. Search is currently filter-based rather than free-text; a virtual keyboard is a separate UX milestone.


## Native personal Library (00.05)

The native Library stores a personal collection in the app's own data namespace:
`ux0:data/VitaNexus-library-v1.dat`. It does not enumerate storage, inspect installed titles, touch game saves, read licenses or modify system files.

From a game's Discover detail sheet:
- **L trigger** toggles Favorite.
- **R trigger** toggles Backlog.
- **Triangle** toggles Completed.
- **Left / Right** adjusts a personal score from 0 to 10 (0 means unrated).
- Changes are written immediately to a temporary sibling file and committed with a rename.

The versioned `VNXLIB|1` format is bounded to 512 records, validates stable game IDs and field ranges, rejects duplicate IDs and malformed/checksum-invalid files, and loads into a temporary in-memory structure before replacing live state. Unknown catalog IDs are retained so a catalog update does not silently erase a user's data. Host tests cover round-trip persistence, toggles/removal, ratings, unknown IDs and corruption handling.

The Library screen lists locally tracked records. Selecting an entry opens its catalog detail when that game ID is present in the current catalog. This milestone is a local tracker, not an installed-title scanner, cloud sync or multi-profile system. CI builds and verifies the VPK; a real PS Vita test is still required.
