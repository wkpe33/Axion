# Axion Entities backgrounds and settings update

## Settings

Axion > Visuals now groups the race timer, expandable music player and shared
Dark/Light theme together in Timer / Player. Entities Background occupies the
right column below Aspect Ratio. It is also available in DDNet's original
Background settings area through the same implementation and configuration.
The original DDNet Background column now scrolls to keep its map controls accessible.

Solid retains cl_background_entities_color. Gradient adds a secondary color and
Vertical, Horizontal or Diagonal direction. Both modes include a live preview.
The preset list has individual thumbnails, selection highlighting and scrolling.
Click a preset to load it, edit colors and explicitly Update to overwrite it.
Save New refuses an existing name; Rename refuses a duplicate name. Delete removes
only the selected stored preset. Reset background restores the original gray solid
background without deleting any presets.

## Storage

The client lazily loads axion_entities_backgrounds.json from its DDNet writable
storage directory when background settings are first opened. No predefined presets
are installed. Example schema (the example is documentation, not a bundled preset):

```json
{"version":1,"presets":[{"name":"My background","mode":1,"primary":128,"secondary":255,"direction":2}]}
```

Colors are DDNet packed 24-bit HSL integers, not RGB hex. Mode 0 is Solid and 1 is
Gradient. Direction 0 is Vertical, 1 Horizontal and 2 Diagonal. Names must be valid
UTF-8, nonblank, at most 63 bytes and contain no control characters. Names are
compared case-insensitively. The safety limits are 4096 presets and a 2 MiB JSON
file. Writes use a temporary file followed by replacement through DDNet storage.
Failed writes do not commit the in-memory candidate. Malformed files remain intact;
valid entries may be displayed but writes are blocked until the file is repaired
and Reload is clicked. Current colors, mode, direction and selected preset name
are saved by DDNet's existing configuration system independently of preset writes.

## Rendering and scope

Normal-map rendering has no new draw. Solid retains the original clear. Gradient
adds one screen-sized untextured quad behind Entities rendering, using existing
per-vertex colors and GPU interpolation. It generates no textures or per-frame
heap allocations. Diagonal uses the midpoint color at the other two corners, so
both triangles describe the same affine gradient without a diagonal seam.
It uses the shared graphics interface rather than backend-specific code.
An explicitly selected Entities background map can cover this clear/gradient, as
it could already cover the old solid background.

## Modified files for this update

- CMakeLists.txt
- src/engine/shared/config_variables.h
- src/engine/shared/axion_background_presets.h / .cpp (new)
- src/game/client/components/axion_background.h (new)
- src/game/client/components/menus_settings_axion_background.cpp (new)
- src/game/client/components/menus.h
- src/game/client/components/menus_settings.cpp
- src/game/client/components/menus_settings_ddnet.cpp
- src/game/client/gameclient.cpp
- src/test/axion_config_test.cpp
- src/test/axion_presentation_test.cpp

## Verification scope

See RELEASE_TESTS.md for actual build and test results. Runtime screenshots,
new Vulkan/OpenGL/Rust-path visual checks, different-resolution visual checks and
comparative FPS/frametime measurements were not performed for this background
update. The client was not automatically relaunched after the user stopped UI
control. Source implementation and automated tests do not establish visual QA or
a measured performance improvement.
