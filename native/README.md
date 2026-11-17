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
- X is intentionally not advertised as an action until module screens are implemented.
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
