# BestClient feature inventory

Reviewed the [official BestClient repository](https://github.com/BestProjectTeam/BestClient).
The newest public release is [v3.1.1](https://github.com/BestProjectTeam/BestClient/releases/tag/v3.1.1)
at `cf192e3fb8590dbe2e4f66a191aa6616beff6975`. The only advertised branch was `main`, at `81ca85f2ba5fca4977d96ad91d15e5604b905269`.
Its diff against v3.1.1 changes only `flake.nix`. The selected browser and input code came from that main checkout. The local source checkouts examined were a
v2.3 checkout and an earlier 3.1 checkout. The local Windows download ZIPs
contained binaries.

BestClient-authored code uses the MIT license. The original notice is
included in `BESTCLIENT-LICENSE.txt`. Existing DDNet, TaterClient and
third-party notices remain in their upstream locations.

## Integration status

| Feature | Axion status |
| --- | --- |
| Browser Utils short KoG/E-Gores names | Ported for display in server rows. The original server name remains available in details and filtering. |
| Chat photo/GIF media, gradient text and skins, hook combo, Jelly Tee, 3D particles, media backgrounds, Sweat Weapon, flying nameplates, motion blur, extra animations | Pending. These span rendering, media decoding, chat, or assets. |
| Music player, custom aspect ratio | Windows SMTC playback/track controls adapted; compact Axion capsule and app-volume slider. Aspect presets and custom ratio imported. Lyrics/visualizer remain outside scope. |
| Demo camera drift, dynamic FOV, demo and asset editors, camera and preview tools | Pending. Axion retains upstream demo playback and editing. |
| Best Input | Ported offset, smoothing, latency compensation, linear/cubic/smooth interpolation, and prediction for other tees. |
| Cloud | Ported offset and optional one-tick prediction for other tees when the server supplies pre-input. Uses live TClient controls with Cloud's sampling behavior. |
| Saiko and Delta | Ported each mode's independent offset and other-tee prediction option. |
| F Input, Auto Margin, Input Debounce, Snap Tap | Outside the selected import scope. |
| Optimizer, process priority, FPS Fog, Gores mode, self timeCP, Fast Actions, speedrun timer, finish prediction, Focus Mode | Pending. Axion retains its own performance presets and upstream timing, bindwheel and practice controls. |
| Additional spectate, camera, zoom, scale, bind and silent typing controls | Pending. Existing DDNet/TClient controls remain available. |
| Chat filters, reply/copy, chat translation extensions, profile changes | Pending. Existing TClient translation, profiles and chat configuration remain available. |
| Voice chat, voice binds and moderation, client indicators, admin panel | Pending. The voice and presence systems require protocol and service compatibility work. |
| Clans, collaborative multimapping, cloud account, cloud sync, updater, custom master services | Pending. These include service-specific protocols, account state, or distribution infrastructure. |
| Fast Practice and its dummy/ghost commands | Pending compatibility and deterministic behavior tests. Existing TClient dummy and practice features remain available. |
| HUD editor | Module-dragging behaviour adapted into a minimal 24-module Axion editor. Theme/FX systems and mini-games remain outside scope. |

The selector is `axion_input_mode`. It defaults to the TClient path.
The four ports use the existing TClient prediction world and dummy
routing. The Cloud adaptation uses TClient's ordinary direction
resolution. The other BestClient input modules are outside this update.

The reference inventory below covers the BestClient config header and
the dedicated component implementation directory. Entries marked outside
scope are listed for provenance.

## Declared settings (507)

- `ScriptName` — outside scope
- `ax_3d_particles` — outside scope
- `ax_3d_particles_alpha` — outside scope
- `ax_3d_particles_collide` — outside scope
- `ax_3d_particles_color` — outside scope
- `ax_3d_particles_color_mode` — outside scope
- `ax_3d_particles_density` — outside scope
- `ax_3d_particles_depth` — outside scope
- `ax_3d_particles_glow` — outside scope
- `ax_3d_particles_push_radius` — outside scope
- `ax_3d_particles_push_strength` — outside scope
- `ax_3d_particles_size_max` — outside scope
- `ax_3d_particles_speed` — outside scope
- `ax_3d_particles_type` — outside scope
- `ax_3d_particles_view_margin` — outside scope
- `ax_air_jump` — outside scope
- `ax_air_jump_hide_particles` — outside scope
- `ax_ambient_dust` — outside scope
- `ax_ambient_dust_amount` — outside scope
- `ax_ambient_dust_color` — outside scope
- `ax_animations` — outside scope
- `ax_atmosphere` — outside scope
- `ax_auto_margin` — outside scope
- `ax_auto_server_list_refresh` — outside scope
- `ax_auto_server_list_refresh_seconds` — outside scope
- `ax_auto_team_lock` — outside scope
- `ax_auto_team_lock_delay` — outside scope
- `ax_auto_update` — outside scope
- `ax_best_input_amount` — ported
- `ax_best_input_interpolation` — ported
- `ax_best_input_latency_comp` — ported
- `ax_best_input_others` — ported
- `ax_best_input_smoothing` — ported
- `ax_bestclient_settings_tabs` — outside scope
- `ax_better_preview` — outside scope
- `ax_better_spectate` — outside scope
- `ax_camera_drift` — outside scope
- `ax_camera_drift_amount` — outside scope
- `ax_camera_drift_reverse` — outside scope
- `ax_camera_drift_smoothness` — outside scope
- `ax_casino_balance` — outside scope
- `ax_casino_last_claim` — outside scope
- `ax_chat_alt_command_layout` — outside scope
- `ax_chat_animation` — outside scope
- `ax_chat_animation_ms` — outside scope
- `ax_chat_animation_type` — outside scope
- `ax_chat_bubble_animation` — outside scope
- `ax_chat_bubble_bg_color` — outside scope
- `ax_chat_bubble_custom_colors` — outside scope
- `ax_chat_bubble_fadein` — outside scope
- `ax_chat_bubble_fadeout` — outside scope
- `ax_chat_bubble_outline_color` — outside scope
- `ax_chat_bubble_rounding` — outside scope
- `ax_chat_bubble_showtime` — outside scope
- `ax_chat_bubble_size` — outside scope
- `ax_chat_bubble_text_color` — outside scope
- `ax_chat_bubbles` — outside scope
- `ax_chat_bubbles_demo` — outside scope
- `ax_chat_bubbles_self` — outside scope
- `ax_chat_media_allowed_domains` — outside scope
- `ax_chat_media_content_filter` — outside scope
- `ax_chat_media_gifs` — outside scope
- `ax_chat_media_photos` — outside scope
- `ax_chat_media_preview` — outside scope
- `ax_chat_media_preview_max_width` — outside scope
- `ax_chat_message_actions` — outside scope
- `ax_chat_only_tags_and_whispers` — outside scope
- `ax_chat_open_animation` — outside scope
- `ax_chat_open_animation_ms` — outside scope
- `ax_chat_typing_animation` — outside scope
- `ax_chat_typing_animation_ms` — outside scope
- `ax_cinematic_camera` — outside scope
- `ax_cinematic_camera_strength` — outside scope
- `ax_client_indicator_browser_url` — outside scope
- `ax_client_indicator_in_name_plate` — outside scope
- `ax_client_indicator_in_name_plate_above_self` — outside scope
- `ax_client_indicator_in_name_plate_dynamic` — outside scope
- `ax_client_indicator_in_name_plate_size` — outside scope
- `ax_client_indicator_in_scoreboard` — outside scope
- `ax_client_indicator_in_scoreboard_size` — outside scope
- `ax_client_indicator_secret_key` — outside scope
- `ax_client_indicator_server_address` — outside scope
- `ax_client_indicator_shared_token` — outside scope
- `ax_client_indicator_token_url` — outside scope
- `ax_client_indicator_versions` — outside scope
- `ax_cloud_input_amount` — ported
- `ax_cloud_input_others` — ported
- `ax_confirm_quit` — outside scope
- `ax_corner_indicator` — outside scope
- `ax_corner_indicator_color_alive` — outside scope
- `ax_corner_indicator_color_frozen` — outside scope
- `ax_corner_indicator_custom_colors` — outside scope
- `ax_corner_indicator_indicate_last_teammate` — outside scope
- `ax_corner_indicator_last_teammate_color` — outside scope
- `ax_corner_indicator_last_teammate_size` — outside scope
- `ax_corner_indicator_ninja_frozen` — outside scope
- `ax_corner_indicator_only_in_team` — outside scope
- `ax_corner_indicator_proximity` — outside scope
- `ax_corner_indicator_proximity_min_size` — outside scope
- `ax_corner_indicator_proximity_range` — outside scope
- `ax_corner_indicator_show_alive` — outside scope
- `ax_corner_indicator_show_frozen` — outside scope
- `ax_corner_indicator_size_alive` — outside scope
- `ax_corner_indicator_size_frozen` — outside scope
- `ax_cursor_trail` — outside scope
- `ax_cursor_trail_disable_movement` — outside scope
- `ax_cursor_trail_mode` — outside scope
- `ax_cursor_trail_number_of_frames` — outside scope
- `ax_cursor_trail_opacity` — outside scope
- `ax_cursor_trail_sampling_fps` — outside scope
- `ax_cursor_trail_trail_image` — outside scope
- `ax_cursor_trail_trail_size` — outside scope
- `ax_custom_aspect_ratio` — outside scope
- `ax_custom_aspect_ratio_apply_mode` — outside scope
- `ax_custom_aspect_ratio_cursor_native` — outside scope
- `ax_custom_aspect_ratio_den` — outside scope
- `ax_custom_aspect_ratio_mode` — outside scope
- `ax_custom_aspect_ratio_num` — outside scope
- `ax_custom_ui_background` — outside scope
- `ax_custom_ui_badge` — outside scope
- `ax_custom_ui_badge_accent_color` — outside scope
- `ax_custom_ui_badge_animate_speed` — outside scope
- `ax_custom_ui_badge_color` — outside scope
- `ax_custom_ui_badge_gradient` — outside scope
- `ax_custom_ui_badge_gradient_color` — outside scope
- `ax_custom_ui_badge_use_accent` — outside scope
- `ax_custom_ui_bind` — outside scope
- `ax_custom_ui_bind_accent_color` — outside scope
- `ax_custom_ui_bind_animate_speed` — outside scope
- `ax_custom_ui_bind_color` — outside scope
- `ax_custom_ui_bind_gradient` — outside scope
- `ax_custom_ui_bind_gradient_color` — outside scope
- `ax_custom_ui_bind_use_accent` — outside scope
- `ax_custom_ui_block` — outside scope
- `ax_custom_ui_block_accent_color` — outside scope
- `ax_custom_ui_block_animate_speed` — outside scope
- `ax_custom_ui_block_color` — outside scope
- `ax_custom_ui_block_gradient` — outside scope
- `ax_custom_ui_block_gradient_color` — outside scope
- `ax_custom_ui_block_opacity` — outside scope
- `ax_custom_ui_block_use_accent` — outside scope
- `ax_custom_ui_button` — outside scope
- `ax_custom_ui_button_accent_color` — outside scope
- `ax_custom_ui_button_animate_speed` — outside scope
- `ax_custom_ui_button_color` — outside scope
- `ax_custom_ui_button_gradient` — outside scope
- `ax_custom_ui_button_gradient_color` — outside scope
- `ax_custom_ui_button_use_accent` — outside scope
- `ax_custom_ui_checkbox` — outside scope
- `ax_custom_ui_checkbox_accent_color` — outside scope
- `ax_custom_ui_checkbox_animate_speed` — outside scope
- `ax_custom_ui_checkbox_color` — outside scope
- `ax_custom_ui_checkbox_gradient` — outside scope
- `ax_custom_ui_checkbox_gradient_color` — outside scope
- `ax_custom_ui_checkbox_use_accent` — outside scope
- `ax_custom_ui_dropdown` — outside scope
- `ax_custom_ui_dropdown_accent_color` — outside scope
- `ax_custom_ui_dropdown_animate_speed` — outside scope
- `ax_custom_ui_dropdown_color` — outside scope
- `ax_custom_ui_dropdown_gradient` — outside scope
- `ax_custom_ui_dropdown_gradient_color` — outside scope
- `ax_custom_ui_dropdown_use_accent` — outside scope
- `ax_custom_ui_editbox` — outside scope
- `ax_custom_ui_editbox_accent_color` — outside scope
- `ax_custom_ui_editbox_animate_speed` — outside scope
- `ax_custom_ui_editbox_color` — outside scope
- `ax_custom_ui_editbox_gradient` — outside scope
- `ax_custom_ui_editbox_gradient_color` — outside scope
- `ax_custom_ui_editbox_use_accent` — outside scope
- `ax_custom_ui_gradient_animate_speed` — outside scope
- `ax_custom_ui_main_panel` — outside scope
- `ax_custom_ui_main_panel_accent_color` — outside scope
- `ax_custom_ui_main_panel_animate_speed` — outside scope
- `ax_custom_ui_main_panel_color` — outside scope
- `ax_custom_ui_main_panel_gradient` — outside scope
- `ax_custom_ui_main_panel_gradient_color` — outside scope
- `ax_custom_ui_main_panel_opacity` — outside scope
- `ax_custom_ui_main_panel_use_accent` — outside scope
- `ax_custom_ui_nav_tab` — outside scope
- `ax_custom_ui_nav_tab_accent_color` — outside scope
- `ax_custom_ui_nav_tab_animate_speed` — outside scope
- `ax_custom_ui_nav_tab_color` — outside scope
- `ax_custom_ui_nav_tab_gradient` — outside scope
- `ax_custom_ui_nav_tab_gradient_color` — outside scope
- `ax_custom_ui_nav_tab_opacity` — outside scope
- `ax_custom_ui_nav_tab_use_accent` — outside scope
- `ax_custom_ui_scrollbar` — outside scope
- `ax_custom_ui_scrollbar_accent_color` — outside scope
- `ax_custom_ui_scrollbar_animate_speed` — outside scope
- `ax_custom_ui_scrollbar_color` — outside scope
- `ax_custom_ui_scrollbar_gradient` — outside scope
- `ax_custom_ui_scrollbar_gradient_color` — outside scope
- `ax_custom_ui_scrollbar_use_accent` — outside scope
- `ax_custom_ui_text` — outside scope
- `ax_custom_ui_text_accent_color` — outside scope
- `ax_custom_ui_text_animate_speed` — outside scope
- `ax_custom_ui_text_color` — outside scope
- `ax_custom_ui_text_gradient` — outside scope
- `ax_custom_ui_text_gradient_color` — outside scope
- `ax_custom_ui_text_use_accent` — outside scope
- `ax_delta_input_amount` — ported
- `ax_delta_input_others` — ported
- `ax_discord_normal_process_priority` — outside scope
- `ax_discord_rpc` — outside scope
- `ax_display_max_fps` — outside scope
- `ax_dynamic_fov` — outside scope
- `ax_dynamic_fov_amount` — outside scope
- `ax_dynamic_fov_smoothness` — outside scope
- `ax_extend_zoom` — outside scope
- `ax_f_input_amount` — outside scope
- `ax_f_input_others` — outside scope
- `ax_fast_actions` — outside scope
- `ax_finish_prediction` — outside scope
- `ax_finish_prediction_consider_freeze_walls` — outside scope
- `ax_finish_prediction_show_always` — outside scope
- `ax_finish_prediction_show_millis` — outside scope
- `ax_finish_prediction_show_percentage` — outside scope
- `ax_finish_prediction_show_time` — outside scope
- `ax_finish_prediction_time_mode` — outside scope
- `ax_flying_name_plates` — outside scope
- `ax_flying_name_plates_drag` — outside scope
- `ax_flying_name_plates_follow` — outside scope
- `ax_flying_name_plates_hide_line` — outside scope
- `ax_flying_name_plates_lift` — outside scope
- `ax_freeview_numpad` — outside scope
- `ax_freeview_smoothness` — outside scope
- `ax_freeview_speed` — outside scope
- `ax_freeze_lightning` — outside scope
- `ax_freeze_lightning_branches` — outside scope
- `ax_freeze_lightning_color` — outside scope
- `ax_freeze_lightning_height` — outside scope
- `ax_freeze_lightning_impact` — outside scope
- `ax_freeze_lightning_shake` — outside scope
- `ax_freeze_lightning_thickness` — outside scope
- `ax_freeze_pull` — outside scope
- `ax_freeze_pull_color` — outside scope
- `ax_freeze_pull_hook_color` — outside scope
- `ax_freeze_pull_range` — outside scope
- `ax_freeze_pull_shield` — outside scope
- `ax_freeze_pull_sparks` — outside scope
- `ax_freeze_pull_strands` — outside scope
- `ax_frozen_skin` — outside scope
- `ax_frozen_skin_darken` — outside scope
- `ax_game_media_background` — outside scope
- `ax_game_media_background_offset` — outside scope
- `ax_gathered_energy` — outside scope
- `ax_gathered_energy_amount` — outside scope
- `ax_gathered_energy_aura` — outside scope
- `ax_gathered_energy_burst` — outside scope
- `ax_gores_mode` — outside scope
- `ax_gores_mode_disable_weapons` — outside scope
- `ax_hammer_hide_particles` — outside scope
- `ax_hammer_slash` — outside scope
- `ax_high_process_priority` — outside scope
- `ax_hook_bolt_arc` — outside scope
- `ax_hook_bolt_branches` — outside scope
- `ax_hook_bolt_color` — outside scope
- `ax_hook_bolt_jitter` — outside scope
- `ax_hook_bolt_sparks` — outside scope
- `ax_hook_charge_time` — outside scope
- `ax_hook_fire_color` — outside scope
- `ax_hook_fire_embers` — outside scope
- `ax_hook_fire_intensity` — outside scope
- `ax_hook_fire_smoke` — outside scope
- `ax_hook_hole_color` — outside scope
- `ax_hook_hole_disk` — outside scope
- `ax_hook_hole_spin` — outside scope
- `ax_hook_hole_streaks` — outside scope
- `ax_hook_lasso_box` — outside scope
- `ax_hook_magic_color` — outside scope
- `ax_hook_magic_runes` — outside scope
- `ax_hook_magic_sparkles` — outside scope
- `ax_hook_magic_spin` — outside scope
- `ax_hook_rainbow_glitter` — outside scope
- `ax_hook_rainbow_rays` — outside scope
- `ax_hook_rainbow_speed` — outside scope
- `ax_hook_rainbow_width` — outside scope
- `ax_hook_rope` — outside scope
- `ax_hook_size` — outside scope
- `ax_hook_style` — outside scope
- `ax_hook_tentacle_color` — outside scope
- `ax_hook_tentacle_slime` — outside scope
- `ax_hook_tentacle_suckers` — outside scope
- `ax_hook_tentacle_thickness` — outside scope
- `ax_hook_tentacle_waves` — outside scope
- `ax_hook_tether` — outside scope
- `ax_hook_tip` — outside scope
- `ax_hook_wind_burst` — outside scope
- `ax_hook_wind_color` — outside scope
- `ax_hook_wind_density` — outside scope
- `ax_hook_wind_dust` — outside scope
- `ax_hook_wind_eye` — outside scope
- `ax_hook_wind_leaf_amount` — outside scope
- `ax_hook_wind_opacity` — outside scope
- `ax_hook_wind_spin` — outside scope
- `ax_hook_wind_thickness` — outside scope
- `ax_hud_chat_scale` — outside scope
- `ax_hud_chat_x` — outside scope
- `ax_hud_chat_y` — outside scope
- `ax_hud_music_player_layout_migrated` — outside scope
- `ax_hud_music_player_scale` — outside scope
- `ax_hud_music_player_x` — outside scope
- `ax_hud_music_player_y` — outside scope
- `ax_hud_voice_hud_scale` — outside scope
- `ax_hud_voice_hud_x` — outside scope
- `ax_hud_voice_hud_y` — outside scope
- `ax_hud_voice_mute_icons_scale` — outside scope
- `ax_hud_voice_mute_icons_x` — outside scope
- `ax_hud_voice_mute_icons_y` — outside scope
- `ax_hud_votes_scale` — outside scope
- `ax_hud_votes_x` — outside scope
- `ax_hud_votes_y` — outside scope
- `ax_input_debounce` — outside scope
- `ax_inputs` — outside scope
- `ax_jelly_tee` — outside scope
- `ax_jelly_tee_duration` — outside scope
- `ax_jelly_tee_others` — outside scope
- `ax_jelly_tee_strength` — outside scope
- `ax_keybind_map_format` — outside scope
- `ax_keystrokes_keyboard` — outside scope
- `ax_keystrokes_keyboard_preset` — outside scope
- `ax_keystrokes_mc_layout` — outside scope
- `ax_keystrokes_mc_pressed_opacity` — outside scope
- `ax_keystrokes_mc_show_lmb` — outside scope
- `ax_keystrokes_mc_show_rmb` — outside scope
- `ax_keystrokes_mc_show_space` — outside scope
- `ax_keystrokes_mouse` — outside scope
- `ax_keystrokes_mouse_preset` — outside scope
- `ax_keystrokes_style` — outside scope
- `ax_killfeed_animation` — outside scope
- `ax_killfeed_animation_ms` — outside scope
- `ax_laser_glow` — outside scope
- `ax_laser_glow_color` — outside scope
- `ax_laser_glow_mode` — outside scope
- `ax_laser_glow_power` — outside scope
- `ax_main_menu_animation` — outside scope
- `ax_main_menu_animation_speed` — outside scope
- `ax_mastersrv` — outside scope
- `ax_meme_nuke_explosion` — outside scope
- `ax_meme_nuke_scale` — outside scope
- `ax_meme_nuke_shake` — outside scope
- `ax_menu_media_background` — outside scope
- `ax_menu_media_background_path` — outside scope
- `ax_menu_ui_better_font` — outside scope
- `ax_menu_ui_new_checkbox` — outside scope
- `ax_menu_ui_new_color_picker` — outside scope
- `ax_menu_ui_new_scrollbar` — outside scope
- `ax_menu_ui_new_tab_option` — outside scope
- `ax_menu_ui_style` — outside scope
- `ax_menu_ui_tab_side` — outside scope
- `ax_module_ui_reveal_animation` — outside scope
- `ax_module_ui_reveal_animation_ms` — outside scope
- `ax_motion_blur` — outside scope
- `ax_motion_blur_strength` — outside scope
- `ax_music_player` — adapted compact Windows media HUD
- `ax_music_player_color_mode` — outside scope
- `ax_music_player_show_current_time` — outside scope
- `ax_music_player_show_lyrics` — outside scope
- `ax_music_player_static_color` — outside scope
- `ax_music_player_text_scale` — outside scope
- `ax_music_player_visualizer_columns` — outside scope
- `ax_music_player_visualizer_mode` — outside scope
- `ax_music_player_visualizer_rounding` — outside scope
- `ax_mute_others_airjump` — outside scope
- `ax_mute_others_hammer` — outside scope
- `ax_mute_others_hook` — outside scope
- `ax_nameplate_client_indicator_offset_x` — outside scope
- `ax_nameplate_client_indicator_offset_y` — outside scope
- `ax_nameplate_gradient` — outside scope
- `ax_nameplate_gradient_animate_speed` — outside scope
- `ax_nameplate_gradient_clan` — outside scope
- `ax_nameplate_gradient_color1` — outside scope
- `ax_nameplate_gradient_color2` — outside scope
- `ax_nameplate_gradient_color3` — outside scope
- `ax_nameplate_gradient_color4` — outside scope
- `ax_nameplate_gradient_color_count` — outside scope
- `ax_nameplate_gradient_everything` — outside scope
- `ax_nameplate_gradient_mode` — outside scope
- `ax_nameplate_gradient_skin` — outside scope
- `ax_nameplate_gradient_target` — outside scope
- `ax_nameplate_voice_offset_x` — outside scope
- `ax_nameplate_voice_offset_y` — outside scope
- `ax_notify_saves_on_map` — outside scope
- `ax_optimizer` — outside scope
- `ax_optimizer_disable_high_detail` — outside scope
- `ax_optimizer_disable_particles` — outside scope
- `ax_optimizer_disable_quads` — outside scope
- `ax_optimizer_fps_fog` — outside scope
- `ax_optimizer_fps_fog_cull_map_tiles` — outside scope
- `ax_optimizer_fps_fog_mode` — outside scope
- `ax_optimizer_fps_fog_radius_tiles` — outside scope
- `ax_optimizer_fps_fog_render_rect` — outside scope
- `ax_optimizer_fps_fog_zoom_percent` — outside scope
- `ax_physic_balls_skin` — outside scope
- `ax_physic_balls_spawn_count` — outside scope
- `ax_prev_inp_mousesens_45_degrees` — outside scope
- `ax_prev_inp_mousesens_small_sens` — outside scope
- `ax_prev_mouse_max_distance_45_degrees` — outside scope
- `ax_profile_assets` — outside scope
- `ax_profiler` — outside scope
- `ax_rocket_glow` — outside scope
- `ax_rocket_glow_color` — outside scope
- `ax_rocket_glow_mode` — outside scope
- `ax_rocket_glow_power` — outside scope
- `ax_saiko_input_amount` — ported
- `ax_saiko_input_others` — ported
- `ax_scoreboard_friendmark` — outside scope
- `ax_scoreboard_scale` — outside scope
- `ax_server_map_preview` — outside scope
- `ax_settings_nav_expanded` — outside scope
- `ax_shotgun_glow` — outside scope
- `ax_shotgun_glow_color` — outside scope
- `ax_shotgun_glow_mode` — outside scope
- `ax_shotgun_glow_power` — outside scope
- `ax_show_correct_checkpoint` — outside scope
- `ax_show_finished_map_on_ego` — outside scope
- `ax_show_finished_map_on_legit` — outside scope
- `ax_show_points_in_tab` — outside scope
- `ax_show_real_hitbox` — outside scope
- `ax_show_real_hitbox_color` — outside scope
- `ax_show_spectator_names` — outside scope
- `ax_show_style_picker` — outside scope
- `ax_show_tee_coords` — outside scope
- `ax_show_vote_percentage` — outside scope
- `ax_showhud_dummy_coord_indicator` — outside scope
- `ax_silent_typing` — outside scope
- `ax_snap_tap` — outside scope
- `ax_snap_tap_delay` — outside scope
- `ax_spec_moved_notify` — outside scope
- `ax_spec_moved_notify_text` — outside scope
- `ax_spec_pause_show_delay` — outside scope
- `ax_toggle_45_degrees` — outside scope
- `ax_toggle_small_sens` — outside scope
- `ax_translate_incoming_ignore_languages` — outside scope
- `ax_translate_incoming_source` — outside scope
- `ax_translate_message_button` — outside scope
- `ax_translate_outgoing_source` — outside scope
- `ax_translate_outgoing_strip_punctuation` — outside scope
- `ax_translate_outgoing_target` — outside scope
- `ax_translate_shift_click_target` — outside scope
- `ax_twitch_chat_nick` — outside scope
- `ax_ui_theme_element` — outside scope
- `ax_ui_themes` — outside scope
- `ax_use_short_kog_server_name` — ported under Axion name
- `ax_voice_chat_activation_mode` — outside scope
- `ax_voice_chat_bitrate` — outside scope
- `ax_voice_chat_enable` — outside scope
- `ax_voice_chat_enable_your_group` — outside scope
- `ax_voice_chat_headphones_muted` — outside scope
- `ax_voice_chat_ingame_only` — outside scope
- `ax_voice_chat_input_device` — outside scope
- `ax_voice_chat_mic_check` — outside scope
- `ax_voice_chat_mic_gain` — outside scope
- `ax_voice_chat_mic_muted` — outside scope
- `ax_voice_chat_muted_names` — outside scope
- `ax_voice_chat_name_volumes` — outside scope
- `ax_voice_chat_nameplate_icon` — outside scope
- `ax_voice_chat_output_device` — outside scope
- `ax_voice_chat_radius_enabled` — outside scope
- `ax_voice_chat_radius_tiles` — outside scope
- `ax_voice_chat_server_address` — outside scope
- `ax_voice_chat_use_team0` — outside scope
- `ax_voice_chat_vad_release_delay_ms` — outside scope
- `ax_voice_chat_vad_threshold` — outside scope
- `ax_voice_chat_volume` — outside scope
- `ax_voice_mod_key` — outside scope
- `ax_weather` — outside scope
- `ax_weather_amount` — outside scope
- `ax_weather_dust_size` — outside scope
- `ax_weather_dust_speed` — outside scope
- `ax_weather_firefly_color` — outside scope
- `ax_weather_firefly_size` — outside scope
- `ax_weather_firefly_speed` — outside scope
- `ax_weather_rain_color` — outside scope
- `ax_weather_rain_size` — outside scope
- `ax_weather_rain_speed` — outside scope
- `ax_weather_rain_tilt` — outside scope
- `ax_weather_snow_color` — outside scope
- `ax_weather_snow_size` — outside scope
- `ax_weather_snow_speed` — outside scope
- `ax_weather_snowflakes` — outside scope
- `ax_wheel_scale` — outside scope
- `br_filter_bestclient` — outside scope
- `cl_asset_arrow` — outside scope
- `cl_asset_cursor` — outside scope
- `dbg_client_indicator` — outside scope
- `indicator_version` — outside scope
- `p_focus_mode` — outside scope
- `p_focus_mode_auto_reply` — outside scope
- `p_focus_mode_auto_reply_message` — outside scope
- `p_focus_mode_hide_chat` — outside scope
- `p_focus_mode_hide_effects` — outside scope
- `p_focus_mode_hide_hud` — outside scope
- `p_focus_mode_hide_names` — outside scope
- `p_focus_mode_hide_scoreboard` — outside scope
- `p_focus_mode_hide_song_player` — outside scope
- `p_focus_mode_hide_ui` — outside scope
- `ri_edge_info_color_freeze` — outside scope
- `ri_edge_info_color_kill` — outside scope
- `ri_edge_info_color_safe` — outside scope
- `ri_edge_info_cords` — outside scope
- `ri_edge_info_jump` — outside scope
- `ri_edge_info_pos_x` — outside scope
- `ri_edge_info_pos_y` — outside scope
- `snd_pack` — outside scope
- `tc_ego_tiles_antilag` — outside scope
- `ui_scale` — outside scope

## Dedicated implementation units (95)

- `3d_particles.cpp` — outside scope
- `admin_panel.cpp` — outside scope
- `aspect_ratio.cpp` — outside scope
- `automargin.cpp` — outside scope
- `bestclient.cpp` — outside scope
- `better_preview.cpp` — outside scope
- `browser_utils.cpp` — short server names ported
- `camera_demos.cpp` — outside scope
- `camera_extras.cpp` — outside scope
- `changelog.cpp` — outside scope
- `chat_bubbles.cpp` — outside scope
- `chat_media.cpp` — outside scope
- `chat_message_actions.cpp` — outside scope
- `chat_qol.cpp` — Silent typing flag removal adapted
- `chat_scroll.cpp` — outside scope
- `clientindicator/browser_cache.cpp` — outside scope
- `clientindicator/client_indicator.cpp` — outside scope
- `clientindicator/presence_cache.cpp` — outside scope
- `cloud_input.cpp` — Cloud logic adapted
- `config_presets.cpp` — outside scope
- `console_scroll.cpp` — outside scope
- `corner_indicator.cpp` — outside scope
- `cursor_trail.cpp` — outside scope
- `demo_tee_editor.cpp` — outside scope
- `discord_rpc.cpp` — outside scope
- `edgehelper.cpp` — outside scope
- `ego_finished_maps.cpp` — outside scope
- `ego_tiles_prediction.cpp` — outside scope
- `fast_actions.cpp` — outside scope
- `fast_practice.cpp` — outside scope
- `finish_prediction.cpp` — outside scope
- `flying_name_plates.cpp` — outside scope
- `fx/fgf_actions.cpp` — outside scope
- `fx/fgf_glow.cpp` — outside scope
- `fx/fgf_iceward.cpp` — outside scope
- `fx/fgf_ki.cpp` — outside scope
- `fx/fgf_weather.cpp` — outside scope
- `fx/hookblackhole.cpp` — outside scope
- `fx/hookfire.cpp` — outside scope
- `fx/hookgrip.cpp` — outside scope
- `fx/hooklightning.cpp` — outside scope
- `fx/hookmagic.cpp` — outside scope
- `fx/hookrainbow.cpp` — outside scope
- `fx/hookrope.cpp` — outside scope
- `fx/hooktentacle.cpp` — outside scope
- `fx/hooktroll.cpp` — outside scope
- `fx/hookwind.cpp` — outside scope
- `fx/lightning.cpp` — outside scope
- `gif_bubbles.cpp` — outside scope
- `gradient.cpp` — outside scope
- `hud_editor.cpp` — module-dragging behaviour adapted in axion_overlay.cpp
- `hud_extras.cpp` — outside scope
- `inputs.cpp` — Best, Saiko and Delta logic adapted
- `jelly_tee.cpp` — outside scope
- `keybind_map.cpp` — outside scope
- `keystrokes.cpp` — outside scope
- `laser_glow.cpp` — outside scope
- `legit_finished_maps.cpp` — outside scope
- `media_decoder.cpp` — outside scope
- `menu_media_background.cpp` — outside scope
- `menus_assets_editor.cpp` — outside scope
- `menus_bestclient.cpp` — outside scope
- `menus_fx.cpp` — outside scope
- `menus_games.cpp` — outside scope
- `music_player.cpp` — Windows session selection/playback behaviour adapted in axion_media.cpp
- `music_player_lyrics.cpp` — outside scope
- `nuke_vfx.cpp` — outside scope
- `optimizer.cpp` — outside scope
- `physicball.cpp` — outside scope
- `process_priority.cpp` — outside scope
- `profile_assets.cpp` — outside scope
- `profiler.cpp` — outside scope
- `quick_binds.cpp` — outside scope
- `rocket_glow.cpp` — outside scope
- `rollback_demo.cpp` — outside scope
- `scroll_bar.cpp` — outside scope
- `server_map_preview.cpp` — outside scope
- `settings_search.cpp` — outside scope
- `show_points.cpp` — outside scope
- `snap_tap.cpp` — outside scope
- `spec_pause_radio.cpp` — outside scope
- `translate.cpp` — outside scope
- `twitch_chat.cpp` — outside scope
- `ui_animations.cpp` — outside scope
- `ui_theme/settings_nav.cpp` — outside scope
- `ui_theme/widgets.cpp` — outside scope
- `updater.cpp` — outside scope
- `visualizer/analyzer.cpp` — outside scope
- `visualizer/service.cpp` — outside scope
- `visualizer/smoother.cpp` — outside scope
- `visualizer/source_passive.cpp` — outside scope
- `visualizer/source_pulse.cpp` — outside scope
- `visualizer/source_wasapi.cpp` — outside scope
- `voice/voice.cpp` — outside scope
- `weapon_vfx.cpp` — outside scope

## Additional requested imports

Aspect presets/custom ratio, application scope (game/full/game without
HUD) and native cursor scaling are adapted from BestClient. The Axion UI
uses existing DDNet controls. Full-interface changes do not currently
include BestClient's timed confirmation/revert overlay.

Sound-pack discovery (Textures -> Audio) and WavPack path selection are adapted from
BestClient. Axion falls back to original game samples for files missing
from a pack. Loading a newly selected pack runs in the existing sound
loading job.

## Compact media player and HUD editing

Axion -> Visuals -> HUD contains Edit HUD. Music player settings have their
own Visuals panel (enabled, background opacity, animation duration). The jumps
indicator toggle remains in the standard HUD appearance settings. Editing
requires a connected game or demo.
Drag a labelled module with the left mouse button, reset one with right
click, or use Reset all / Done. Escape finishes editing. Positions are saved
as screen-relative offsets. `ax_hud_edit` opens the same editor.

The 24 movable groups include health/ammo, jumps, DDRace abilities, game
clock, score, movement, spectator count, dummy actions, FPS/prediction,
local time, notifications, race/finish times, records, votes, chat, killfeed,
broadcast, status bar, spectator HUD, music, debug information, frozen team,
frozen count and last-player notification. World-anchored nameplates,
crosshairs and fullscreen scoreboards keep their existing semantics.

Music follows BestClient's Windows SMTC session selection: playing apps
first, then the tracked and system-current sessions. Its capsule replaces the top game timer, shows game/race/countdown time even
without a media session, and expands on hover while chat is open. Play/pause, previous/next and
application volume are sent from a worker thread. Unsupported controls are
dimmed. The slider adjusts the selected application's CoreAudio sessions;
if the app cannot be matched, volume is unavailable rather than changing
system or game volume. Browsers can share volume across their tabs.
No local music library, artwork, lyrics or visualizer is added.

Axion -> Misc -> Chat contains Silent typing. It removes only the
network CHATTING flag and preserves the existing chat input freeze.
The jumps-only setting still respects the main Show HUD switch.

User-facing imported commands now use `ax_` instead of `bc_`.
Legacy top-level config lines are translated when reading config files;
new saves use the registered `ax_` names. No `bc_` commands are registered.

### Timer slot and texture-memory update

The capsule now uses BestClient's top-center timer slot, suppresses the
original timer when enabled, and retains game time with no music playing.
The compact height is 10 HUD units (BestClient default is about 9.6),
expanding to 16 units with separate hit targets for each music control.
Compact width follows the measured timer text plus padding, with no fixed
minimum width; expansion reserves additional space for the controls. Hover animation and track/volume controls remain.
The redundant TClient link was removed from Axion -> Game.

Audio packs moved to Textures -> Audio, with a searchable row list,
refresh and directory button. Selection still uses asynchronous audio loading.

### Player settings layout

Visuals has separate Visuals, HUD, Music player and Aspect Ratio panels.
The duplicate jumps toggle was removed from the Axion HUD panel. Game is
empty for now; its controls were removed from this page without changing
their saved values. Misc contains only Chat / Silent typing, with no volume
sliders. Audio-pack selection stays in Textures -> Audio. The music panel
provides visibility, background opacity (30-100%) and hover animation time
(0-500 ms, where zero expands instantly). The timer capsule and HUD editor
bounds follow actual measured glyph width, including hour/day formats.
