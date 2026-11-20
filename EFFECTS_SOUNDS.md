# Axion: effects, sounds and chat cursor

Windows x64 Release update. Implementation is in the existing DDNet UI.
No new external GUI or window was added.

## UI

Axion -> Misc now contains Effects and Sounds buttons. Each opens its own
scrolling settings page with My and Other columns, plus a shared section
for events that lack a trustworthy owner. Back stays above the scroll area;
Escape returns to Misc. Existing volume, music and audio-pack settings stay intact.

## Cursor

Chat and HUD editing now use the same CUi::ConvertMouseMove and
CUi::OnCursorMove path as the menu. Both project the same window-pixel
cursor through AxionCursorForCanvas; CUi::UpdateCursor updates cursor state
without changing chat text focus. There is no custom sensitivity factor or
recentring when chat opens. Main and Dummy use the same UI cursor.

## Effects

- Air / double jump: `ax_effect_my_air_jump`, `ax_effect_other_air_jump`, `ax_effect_shared_air_jump`
- Hammer hit: `ax_effect_my_hammer_hit`, `ax_effect_other_hammer_hit`, `ax_effect_shared_hammer_hit`
- Hammer swing: `ax_effect_my_hammer_swing`, `ax_effect_other_hammer_swing`, `ax_effect_shared_hammer_swing`
- Gun muzzle flash: `ax_effect_my_gun_muzzle`, `ax_effect_other_gun_muzzle`, `ax_effect_shared_gun_muzzle`
- Shotgun muzzle flash: `ax_effect_my_shotgun_muzzle`, `ax_effect_other_shotgun_muzzle`, `ax_effect_shared_shotgun_muzzle`
- Grenade explosion: `ax_effect_my_explosion`, `ax_effect_other_explosion`, `ax_effect_shared_explosion`
- Ninja swing: `ax_effect_my_ninja_swing`, `ax_effect_other_ninja_swing`, `ax_effect_shared_ninja_swing`
- Ninja slash: `ax_effect_my_ninja_muzzle`, `ax_effect_other_ninja_muzzle`, `ax_effect_shared_ninja_muzzle`
- Hook chain / head: `ax_effect_my_hook`, `ax_effect_other_hook`, `ax_effect_shared_hook`
- Hook collision line: `ax_effect_my_hook_collision`, `ax_effect_other_hook_collision`, `ax_effect_shared_hook_collision`
- Spawn particles: `ax_effect_my_spawn`, `ax_effect_other_spawn`, `ax_effect_shared_spawn`
- Death particles: `ax_effect_my_death`, `ax_effect_other_death`, `ax_effect_shared_death`
- Skid trail: `ax_effect_my_skid`, `ax_effect_other_skid`, `ax_effect_shared_skid`
- Projectile smoke trail: `ax_effect_my_smoke_trail`, `ax_effect_other_smoke_trail`, `ax_effect_shared_smoke_trail`
- Smoke particles: `ax_effect_my_smoke`, `ax_effect_other_smoke`, `ax_effect_shared_smoke`
- Bullet trail: `ax_effect_my_bullet_trail`, `ax_effect_other_bullet_trail`, `ax_effect_shared_bullet_trail`
- Damage indicators: `ax_effect_my_damage`, `ax_effect_other_damage`, `ax_effect_shared_damage`
- Freeze stars: `ax_effect_my_freeze_stars`, `ax_effect_other_freeze_stars`, `ax_effect_shared_freeze_stars`
- Ninja / pickup shine: `ax_effect_my_shine`, `ax_effect_other_shine`, `ax_effect_shared_shine`
- Freezing flakes: `ax_effect_my_frozen`, `ax_effect_other_frozen`, `ax_effect_shared_frozen`
- Sparkle trail: `ax_effect_my_sparkle`, `ax_effect_other_sparkle`, `ax_effect_shared_sparkle`
- Finish / birthday confetti: `ax_effect_my_confetti`, `ax_effect_other_confetti`, `ax_effect_shared_confetti`
- Laser beam / head: `ax_effect_my_laser`, `ax_effect_other_laser`, `ax_effect_shared_laser`
- TClient tee trail: `ax_effect_my_tee_trail`, `ax_effect_other_tee_trail`, `ax_effect_shared_tee_trail`
- All particle effects: `ax_effect_my_particles`, `ax_effect_other_particles`, `ax_effect_shared_particles`

## Gameplay sounds

- Gun fire: `ax_sound_my_gun_fire`, `ax_sound_other_gun_fire`, `ax_sound_shared_gun_fire`
- Shotgun fire: `ax_sound_my_shotgun_fire`, `ax_sound_other_shotgun_fire`, `ax_sound_shared_shotgun_fire`
- Grenade fire: `ax_sound_my_grenade_fire`, `ax_sound_other_grenade_fire`, `ax_sound_shared_grenade_fire`
- Hammer fire: `ax_sound_my_hammer_fire`, `ax_sound_other_hammer_fire`, `ax_sound_shared_hammer_fire`
- Hammer hit: `ax_sound_my_hammer_hit`, `ax_sound_other_hammer_hit`, `ax_sound_shared_hammer_hit`
- Ninja fire: `ax_sound_my_ninja_fire`, `ax_sound_other_ninja_fire`, `ax_sound_shared_ninja_fire`
- Grenade explode: `ax_sound_my_grenade_explode`, `ax_sound_other_grenade_explode`, `ax_sound_shared_grenade_explode`
- Ninja hit: `ax_sound_my_ninja_hit`, `ax_sound_other_ninja_hit`, `ax_sound_shared_ninja_hit`
- Laser fire: `ax_sound_my_laser_fire`, `ax_sound_other_laser_fire`, `ax_sound_shared_laser_fire`
- Laser bounce: `ax_sound_my_laser_bounce`, `ax_sound_other_laser_bounce`, `ax_sound_shared_laser_bounce`
- Weapon switch: `ax_sound_my_weapon_switch`, `ax_sound_other_weapon_switch`, `ax_sound_shared_weapon_switch`
- Player pain short: `ax_sound_my_player_pain_short`, `ax_sound_other_player_pain_short`, `ax_sound_shared_player_pain_short`
- Player pain long: `ax_sound_my_player_pain_long`, `ax_sound_other_player_pain_long`, `ax_sound_shared_player_pain_long`
- Body land: `ax_sound_my_body_land`, `ax_sound_other_body_land`, `ax_sound_shared_body_land`
- Player airjump: `ax_sound_my_player_airjump`, `ax_sound_other_player_airjump`, `ax_sound_shared_player_airjump`
- Player jump: `ax_sound_my_player_jump`, `ax_sound_other_player_jump`, `ax_sound_shared_player_jump`
- Player die: `ax_sound_my_player_die`, `ax_sound_other_player_die`, `ax_sound_shared_player_die`
- Player spawn: `ax_sound_my_player_spawn`, `ax_sound_other_player_spawn`, `ax_sound_shared_player_spawn`
- Player skid: `ax_sound_my_player_skid`, `ax_sound_other_player_skid`, `ax_sound_shared_player_skid`
- Tee cry: `ax_sound_my_tee_cry`, `ax_sound_other_tee_cry`, `ax_sound_shared_tee_cry`
- Hook loop: `ax_sound_my_hook_loop`, `ax_sound_other_hook_loop`, `ax_sound_shared_hook_loop`
- Hook attach ground: `ax_sound_my_hook_attach_ground`, `ax_sound_other_hook_attach_ground`, `ax_sound_shared_hook_attach_ground`
- Hook attach player: `ax_sound_my_hook_attach_player`, `ax_sound_other_hook_attach_player`, `ax_sound_shared_hook_attach_player`
- Hook noattach: `ax_sound_my_hook_noattach`, `ax_sound_other_hook_noattach`, `ax_sound_shared_hook_noattach`
- Pickup health: `ax_sound_my_pickup_health`, `ax_sound_other_pickup_health`, `ax_sound_shared_pickup_health`
- Pickup armor: `ax_sound_my_pickup_armor`, `ax_sound_other_pickup_armor`, `ax_sound_shared_pickup_armor`
- Pickup grenade: `ax_sound_my_pickup_grenade`, `ax_sound_other_pickup_grenade`, `ax_sound_shared_pickup_grenade`
- Pickup shotgun: `ax_sound_my_pickup_shotgun`, `ax_sound_other_pickup_shotgun`, `ax_sound_shared_pickup_shotgun`
- Pickup ninja: `ax_sound_my_pickup_ninja`, `ax_sound_other_pickup_ninja`, `ax_sound_shared_pickup_ninja`
- Weapon spawn: `ax_sound_my_weapon_spawn`, `ax_sound_other_weapon_spawn`, `ax_sound_shared_weapon_spawn`
- Weapon noammo: `ax_sound_my_weapon_noammo`, `ax_sound_other_weapon_noammo`, `ax_sound_shared_weapon_noammo`
- Hit: `ax_sound_my_hit`, `ax_sound_other_hit`, `ax_sound_shared_hit`
- Ctf drop: `ax_sound_my_ctf_drop`, `ax_sound_other_ctf_drop`, `ax_sound_shared_ctf_drop`
- Ctf return: `ax_sound_my_ctf_return`, `ax_sound_other_ctf_return`, `ax_sound_shared_ctf_return`
- Ctf grab pl: `ax_sound_my_ctf_grab_pl`, `ax_sound_other_ctf_grab_pl`, `ax_sound_shared_ctf_grab_pl`
- Ctf grab en: `ax_sound_my_ctf_grab_en`, `ax_sound_other_ctf_grab_en`, `ax_sound_shared_ctf_grab_en`
- Ctf capture: `ax_sound_my_ctf_capture`, `ax_sound_other_ctf_capture`, `ax_sound_shared_ctf_capture`

## Ownership and limitations

Both connected local IDs (main and Dummy) count as My, regardless of which
tee is currently controlled. Spectating another player does not make them local.
Character effects, hooks, flashes and extended/predicted projectile/laser
trails use explicit owner IDs. Predicted audio/effect events carry a separate
presentation-owner field. Entity IDs, projectile start ticks and coordinates
are never interpreted as client IDs. Physics, event identity/deduplication,
network messages and prediction decisions are unchanged.

Legacy server SoundWorld, explosion, hammer-hit, spawn, damage and celebration
events generally carry positions only. These use Shared settings. A death
visual carries the victim client ID, but a separate death sound usually does
not. Legacy projectiles/lasers without ownership metadata, map laser effects
and pickup shine are Shared. Disabling My or Other is deliberately insufficient
to mute an ownerless event: use its Shared switch. On servers without matching
predicted/extended data, fewer events can be separated by owner.

Air jump and double jump are the same existing AirJump effect, not two
invented events. There is no dedicated Hook Release sound in this source's
sound catalog. Hook Loop is a real sound asset but isn't normally dispatched
by this client. CTF, pickup, weapon-spawn, no-ammo, pain, landing and hit
sound sets are included. Chat sounds, menu music and custom map ambient
samples are not gameplay sound-set filters and retain their existing controls.

All particle effects is a master over particle creation; Smoke particles is
a master over smoke sprites. Specific effects also honor these masters.
Explosion controls its full burst, including its smoke; Smoke can separately
hide the smoke. Existing particles may finish their short lifetimes after a
toggle; newly generated effects are filtered. Existing upstream master
settings (snd_game, snd_gun, snd_long_pain, freeze stars, tee trails etc.)
remain authoritative; these new enabled-by-default switches don't turn them on.

Visual switches never suppress their associated audio. Sound filtering occurs
before sample selection at gameplay dispatch, with positioning, gain, mixing,
master volumes and demo-recording messages preserved. Queued sounds retain
ownership until playback. No per-frame heap allocations are added for filters.

## Verification status

The final Windows x64 Release build and 394 enabled C++ tests passed, including
presentation ownership, filter configuration and cursor projection tests. Three
upstream tests are disabled. Final visual/gameplay checks were not completed;
see RELEASE_TESTS.md for the exact test scope and limitations.
