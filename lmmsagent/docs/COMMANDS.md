# Tool Commands

Canonical listing generated from `TOOL_CONTRACT_V2.md` section 2.
Wire names normalize by lowercasing + stripping non-alphanumerics
(`create_send` → `createsend`); planner/playbook actions stay snake_case.

## Project & transport

- `new_project` (confirm)
- `open_project {path}` (confirm)
- `save_project`
- `save_project_as {path}`
- `set_time_signature {numerator, denominator}`
- `set_metronome {enabled}`
- `set_master_volume {value 0..1}`
- `set_master_pitch {value}`
- `set_play_pos {tick | bar, beat, tick}`
- `set_loop {begin_tick, end_tick}`
- `clear_loop`
- `set_stop_behaviour {mode}`
- `play_pattern {pattern?}`
- `play_clip {track, clip_index?}`
- `insert_bar {at_tick}`
- `remove_bar {at_tick}`
- `set_tempo {tempo}` (v1)

## Tracks & clips (arrangement)

- `create_clip {track?, tick, name?}`
- `move_clip {track, clip_index, new_tick}`
- `resize_clip {track, clip_index, new_length}`
- `split_clip {track, clip_index, at_tick}`
- `clone_clip {track, clip_index, to_tick?}`
- `delete_clip {track, clip_index}`
- `set_clip_mute {track, clip_index, mute}`
- `set_clip_name {track, clip_index, name}`
- `set_clip_color {track, clip_index, color}`
- `clone_track {track, name?}`
- `delete_track {track}` (confirm)
- `move_track {track, index}`
- `set_track_volume {track, value 0..1}`
- `set_track_pan {track, value 0..1}`
- `set_track_pitch {track, value}`
- `set_track_key_range {track, first_key, last_key}`
- `set_track_base_note {track, key}`

## Notes

- `add_notes {track?, clip_index?, notes}` (v1)
- `edit_notes {track, clip_index?, notes}`
- `remove_notes {track, clip_index?, keys, pos?}`
- `clear_clip {track, clip_index?}` (confirm)
- `quantize_clip {track, clip_index?, resolution 1..192}`
- `humanize_clip {track, clip_index?, amount 0..1}`
- `reverse_clip {track, clip_index?}`
- `split_clip_notes {track, clip_index?, at_tick}`
- `set_clip_velocity_scale {track, clip_index?, scale 0..2}`
- `add_chord {track?, clip_index?, root, chord, pos?, length?, velocity?}`
- `add_arpeggio {track?, clip_index?, root, chord, direction, steps, ...}`

## Patterns (BB)

- `create_pattern {name?}` (v1)
- `select_pattern {pattern}`
- `clone_pattern {pattern, name?}`
- `set_steps {track, clip_index?, steps, clear_existing?}`
- `set_step_velocity {track, clip_index, step, velocity 1..127}`
- `set_steps_per_bar {track, steps}`
- `add_rhythm {drum, pattern}`

## Samples

- `set_sample_loop {track, clip_index?, mode, loop_start?, loop_end?}`
- `set_sample_pitch {track, clip_index?, semitones}`
- `set_sample_amp {track, clip_index?, value 0..1}`
- `set_sample_range {track, clip_index?, start_frame, length_frames}`

## Instruments

- `load_instrument` (v1)
- `load_instrument_preset {track?, path, plugin?}` (xiz/sf2/pat/xp/VST)
- `set_vst_program {track, program}`
- `set_vst_param {track, param, value}`
- `set_sf2_patch {track, bank?, patch}`
- `describe_instrument {track?}`

## Sound shaping

- `set_envelope {target?, predelay?, attack?, hold?, decay?, sustain?, release?, amount?}`
- `set_filter {enabled?, type?, cutoff?, resonance?}`
- `set_lfo {target?, amount?, speed?, wave?, delay?, attack?}`
- `set_arp {enabled?, chord?, range?, direction?, time?, gate?, mode?}`
- `set_note_stacking {enabled?, type?, range?}`

## Effects

- `set_effect_param {track?, effect, param, value}` (v2)
- `get_effect_params {track?, effect}`
- `move_effect {track?, effect, direction}`
- `set_effect_enabled {track?, effect, enabled}`
- `set_effect_wetdry {track?, effect, value 0..1}`

## Mixer

- `list_mixer_channels`
- `create_channel {name?}`
- `delete_channel {channel}` (confirm)
- `rename_channel {channel, name}`
- `move_channel {channel, direction}`
- `set_channel_volume {channel, value 0..1}`
- `set_channel_mute {channel, mute}`
- `set_channel_solo {channel, solo}`
- `add_channel_effect {channel, effect}`
- `remove_channel_effect {channel, effect}`
- `create_send {from, to, amount? 0..1}`
- `delete_send {from, to}`
- `set_send_amount {from, to, amount}`
- `route_track_to_channel {track, channel}`
- `get_peak_levels {channel?}`

## Automation

- `create_automation {track?, address?, name?}`
- `automate {address, ticks, values, clip?}`
- `set_automation_node {clip, tick, value}`
- `remove_automation_node {clip, tick}`
- `set_automation_tension {clip, tick?, tension -1..1}`
- `set_automation_progression {clip, progression}`
- `global_automate {address, ticks, values}` (confirm)
- `read_automation {clip | address}`
- `list_automation`

## Controllers

- `create_controller {type: lfo|midi|peak, name?}`
- `set_lfo_controller {controller, wave?, speed?, amount?, base?, phase?, multiplier?}`
- `connect_controller {controller, address}`
- `disconnect_controller {address}` (confirm)
- `describe_controllers`

## Render / export

- `render_song {path, format?, sample_rate?, bit_depth?, bitrate?, stereo_mode?}`
- `render_tracks {dir, prefix?, format?, sample_rate?, bit_depth?, bitrate?, stereo_mode?}`
- `render_preview {path?, format?, begin_tick?, end_tick?}`
- `get_render_progress {render_id?}`
- `cancel_render {render_id?}`
- `export_midi {path}`

## MIDI record

- `record_arm {track?, clip_index?}`
- `record_disarm`
- `record_start`
- `record_stop {quantize?: 1..192}`

## Misc

- `describe_song`
- `set_project_notes {text}`
- `get_project_notes`
- `set_microtuner {enabled?, scale?, keymap?}`

## Legacy v1 surface (unchanged wire names)

- Query: `getprojectstate`, `listtracks`, `gettrackdetails`, `listpatterns`,
  `listinstruments`, `listeffects`, `listtoolwindows`, `getselectionstate`,
  `findtrackbyname`, `searchprojectaudio`
- Write: `createtrack`, `renametrack`, `loadinstrument`, `loadsample`,
  `createpattern`, `addnotes`, `addsteps`, `settempo`, `addeffect`,
  `removeeffect`, `opentool`, `importaudio`, `importmidi`, `importhydrogen`,
  `selecttrack`, `mutetrack`, `solotrack`
- Safety: `createsnapshot`, `undolastaction`, `rollbacktosnapshot`,
  `diffsincesnapshot`

## Planner Intents (text agent)

- `beginner help`
- `manual map`
- `manual map song editor`
- `manual map automation`
- `manual map mixer`
- `manual map render`
- `manual map samples`
- `list playbooks`
