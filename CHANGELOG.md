# Changelog

## Effects, sounds and chat cursor update

- Shared the main menu cursor input/projection with in-game chat, without recentering.
- Added Effects / Sounds panels under Axion -> Misc, using the existing UI.
- Added 25 effect categories and 37 gameplay sound-set filters with My / Other
  ownership and explicit Shared settings for ownerless events.
- Preserved physics/network decisions and existing audio controls.
- See EFFECTS_SOUNDS.md for the full inventory, limitations and verification status.

## Player settings layout update

- Added a separate Music player panel under Axion -> Visuals with visibility,
  background opacity and hover animation duration settings.
- Removed the duplicate jumps toggle from Axion -> HUD.
- Cleared Axion -> Game, moved Silent typing to Misc, and removed Misc volume controls.
- Made capsule width and editor bounds follow actual timer text width.

## Timer slot, Audio assets and memory update

- Moved the media capsule to the top timer slot, matching BestClient's
  game-time behaviour and reducing compact height to 10 HUD units.
- Kept hover animation and separated music-control hit targets.
- Moved sound packs to Textures -> Audio with search, refresh and directory.
- Removed the TClient input-settings link from Axion -> Game.
- Added small, visible-only asset previews, bounded HD skins and
  memory-pressure eviction/loading guards; corrected Vulkan memory counters.

## Compact HUD and media update

- Added a compact, hover-expanding Windows media capsule with play/pause,
  previous/next track and application-volume controls; OS work is asynchronous.
- Added a minimal editor for 24 HUD groups in Axion -> Visuals -> HUD.
- Made the jumps indicator independent of Show DDRace HUD.
- Added Silent typing in Axion -> Game -> Controls.
- Renamed imported `bc_` console variables to `ax_`, with legacy config loading.

## Axion Windows x64 update

- Added **Axion** immediately below **TClient** in the standard settings
  menu. Attribution notices are shipped as files.
- Added Best Input, Cloud, Saiko, and Delta to **TClient → Input** with
  their mode-specific settings. The TClient option retains existing
  Fast Input controls.
- Imported display-only short KoG and E-Gores server names from
  BestClient 3.1.1.
- Added a Test UDP popup with presets, a custom port, individual tests,
  Test All, a 64-port search, and saved per-endpoint preferences
  in the server browser.
- Reduced startup progress rendering and added opt-in startup timing.

## Visual and sound settings update

- Added Visuals, Game and Misc panels in Axion.
- Added native, 5:4, 4:3, 3:2, 16:9, 16:10 and custom gameplay aspect ratio.
- Added sound-pack discovery and background sample loading.
- Removed the startup-rendering and diagnostics checkboxes.
- Removed performance preset selection and enforced the low-detail mode.
- Added persistent Rust Vulkan pipeline caches keyed to the GPU/driver.

## Compact settings layout

Axion now uses two columns of compact panels. Visuals places short server
names on the left and Aspect Ratio on the right. Game groups input and
controls. Misc places sound-pack selection beside volume controls.

Aspect Ratio uses labeled Preset and Apply dropdowns, a cursor checkbox,
two editable Custom size fields separated by a colon, and an Apply
button. Custom values are validated and committed together when Apply
is clicked.

## 2026-11-13: Visuals organization and Entities backgrounds

- Group timer, expandable music player and shared theme in one Visuals panel.
- Add Entities Solid/Gradient controls, three gradient directions and live preview.
- Add user preset JSON persistence, scrolling thumbnails and explicit save/update/rename/delete.
- Preserve damaged preset files read-only and retain the original solid color setting.
- Final Windows x64 Release build and 394 enabled C++ tests passed.
- Final UI screenshots and renderer/performance comparison were not performed.
