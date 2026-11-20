# Axion chat selection and freeze controls — 2026-11-13

## Implemented

Axion > Visuals > Visuals now has a separate Round freeze tiles checkbox.
Disabling it retains the radius. Enabling it from radius zero selects radius 8.
The existing 0–16 slider remains. Full Entities mode is required. Corner tests
consider every non-air tile, including contact from the other game/front layer
and the diagonal cell at each corner. Contacted corners stay square; only exposed
corners are clipped. Gameplay collision and networking are unchanged.

The existing CChat history gains left-button drag selection of portions, words
(double click) or an entire message, including wrapped lines and UTF-8 text.
Selection endpoints use UTF-8 byte boundaries reconstructed from rendered
codepoints. A subtle selection background is drawn under the original cached text.
Native glyph geometry is collected only while creating a chat text container;
selection does not redo text layout every frame. Explicit newline characters are
retained, while automatic line wrapping does not add clipboard newlines.
Rendered nickname, ID and decoration text can be selected explicitly; clicking
message content does not implicitly include them. Hidden color markup is excluded.

With chat open, wheel up/down scrolls its existing history. Position is preserved
when new messages arrive while scrolled. The bounded history remains 64 records.

Right-click selected text for Copy (or Ctrl+C). Right-click a nickname for Reply.
Copy has priority when clicking an existing selection. Reply inserts the historical
nickname followed by colon/space, preserves the current channel and draft, avoids
repeating a prefix, and places the caret after it. It never sends a message.
A full draft is preserved unchanged if there is no room for the prefix.
Menus stay inside the screen, close on outside click/Escape/chat close/screen
change, and use the existing chat cursor/HUD coordinate projection. Enter with a
context menu open activates the action instead of sending the chat draft.

## Actual validation

Windows x64 Release build succeeded, exit 0:
TClient/build-next/chat-rounding-verified-build.log
396 enabled C++ tests from 74 suites passed; 3 upstream tests remain disabled:
TClient/build-next/chat-rounding-verified-tests.log
New automated tests cover Unicode reply/draft preservation, duplicate prefix
avoidance, changing the reply target, full drafts and manually edited prefixes.
The client was not launched for visual/manual verification. These unit results do
not establish that all requested GUI and clipboard checks passed. Mouse selection,
wrapped/scrolled hit detection, system clipboard, menu interactions, visual corner
contacts, different screen sizes and Vulkan/OpenGL/Rust gameplay remain unverified
in the final running client. No measured FPS/frametime claim is made.

## Limits

Selection spans one history record at a time, including its wrapped lines; it does
not span separate messages. UTF-8 codepoints are kept intact, but selection does
not treat complex emoji sequences/combining marks as indivisible grapheme clusters.
No clipboard rich formatting or drag auto-scroll is added. Full drafts cannot
receive a reply prefix until space is freed; their content is retained.
Rounded Entities still uses the optional visible-tile geometry path described in
previous release notes; disabling it restores the original buffered path.

## Modified files

- CMakeLists.txt
- src/engine/shared/config_variables.h
- src/engine/shared/axion_chat_reply.h (new)
- src/engine/textrender.h
- src/engine/client/text.cpp
- src/game/client/components/chat.h
- src/game/client/components/chat.cpp
- src/game/client/components/menus_settings.cpp
- src/game/map/render_layer.cpp
- src/game/map/render_map.h
- src/game/map/render_map.cpp
- src/test/axion_config_test.cpp
- CHAT_SELECTION.md / LATEST_UPDATE.md
