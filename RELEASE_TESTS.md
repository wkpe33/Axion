# Axion Windows x64 Release — 2026-11-13

Built existing TClient/Axion source using MSVC x64 Release, Vulkan enabled and
existing OpenGL fallback/Rust Vulkan path retained. No new client framework.

## This update

Visuals groups Timer / Player / Theme in one panel. Entities backgrounds support
Solid, Vertical, Horizontal and Diagonal gradients with live preview and a complete
user-created JSON preset list (save, load, update, rename, delete, reset). Full
storage format, source file list and rendering details are in BACKGROUND_PRESETS.md.
Existing primary background color and unrelated configuration are preserved.

## Automated validation

Final build: build-next/background-verified-build.log, exit 0.
Final suite: build-next/background-verified-tests.log, exit 0.
394 enabled C++ tests from 73 suites passed; 3 upstream tests remain disabled.
Background tests execute preset creation, case-insensitive duplicate rejection,
file reload, explicit update, rename, delete, invalid names/colors, malformed JSON,
invalid/duplicate entries, preservation of damaged files, solid color consistency,
three gradient corner layouts and diagonal midpoint continuity. The configuration
round-trip test saves and reloads theme, timer settings, invitation position and
the new background mode/color/direction/selected preset name.

## Earlier integrated changes retained in this delivery

Scoreboard Alt/right-click player menu; seven supported player actions with
server-authoritative team commands; thin ten-second invitations from validated
stock server chat events; chat-only mouse interaction; timer controls and preview;
shared Dark/Light theme and lightweight transitions; existing HUD editor integration;
render text caching and media snapshots only when changed. Existing input modes,
UDP testing, sound packs, presentation filters, aspect ratio and renderer paths
remain in the build.

Before this background change, a local Vulkan/Rust-path Tutorial session connected
both the main client and Dummy and shut down with client/server exit 0. This is not
a runtime test of the final background update. Full manual verification of context
menu actions, invitation accept/decline/input isolation and HUD persistence was not
completed. Stock invitation parsing is unit-tested; arbitrary server-specific chat
formats are not fabricated. Decline is local dismissal because there is no standard
server decline command. Team locks/permissions remain authoritative on the server.

## Unfinished visual/performance checks

No new client UI session was launched after the user stopped computer control.
Therefore screenshots of the completed menus/themes/backgrounds are not supplied,
and final-build visual checks at different resolutions, Vulkan/OpenGL/Rust renderer
comparisons and a gameplay FPS/frametime regression benchmark remain unperformed.
A prior single first-interactive startup sample was 1284.325 ms; it is not a before/
after comparison and does not establish a startup speedup. No new comparative
performance improvement is claimed.

Client SHA256: 71E06D1206BD1B9EE6225A59D33C256E1B67C9FEB652B5CBF937AECFC03E6E3E
