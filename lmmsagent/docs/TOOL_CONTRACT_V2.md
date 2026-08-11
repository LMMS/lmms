# AgentControl Tool Contract v2 (overhaul surface)

Canonical spec for the v2 tool surface. The C++ implementation in
`plugins/AgentControl`, the NL manifest (`command_manifest.v3.json`), the
Python planner/orchestrator, and the MCP operator server all build against this
document. Wire format is unchanged from v1 (newline-delimited JSON over TCP,
one request per connection).

## 1. Conventions

### 1.1 Envelope (v2 = v1 + `hints`)

```json
{ "ok": true, "result": {}, "state_delta": {},
  "warnings": [], "error_code": null, "error_message": null,
  "hints": [] }
```

- `hints`: non-blocking contextual notes ("track did not exist; created
  'Lead'", "tempo is automated; direct setValue is overridden during
  playback"). Client LLMs use hints to stay honest without polling.
- Write tools return `state_delta` (diff of track_count/tempo/selected
  track, plus domain-specific fields when useful).

### 1.2 Units

- Time: **ticks** (192 per bar at 4/4). Tools accept `{bar, beat, tick}` as
  an alternative to `tick` when documented.
- Pitch: **MIDI note numbers** (60 = C4 concert).
- Note velocity: **1..127** (plugin maps to `Note::setVolume(v/127.0)`).
- Track volume/pan/master volume: **0..1 float** (LMMS FloatModel space).
- Automation clip values: **0..1 normalized** (clip space) — the plugin
  converts via `inverseScaledValue/scaledValue` on the bound model.
- Render: sample rate 44100..192000, bit depth 16|24|32 (32 = float),
  bitrate 64..320 kbps, stereo mode mono|stereo|joint_stereo.

### 1.3 Model addresses

Every automatable model gets a stable address so clients never need C++
knowledge. `describe_track` / `describe_effect` / `describe_instrument` /
`describe_controllers` return the resolvable tree with ranges and units.

```
song.tempo                      song.master.volume      song.master.pitch
track:<name>.<param>            track:Lead.filter.cutoff
fx:<channel>.<effect>.<param>   fx:2.reverb.wet         (effect matched by normalized displayName)
inst:<param>                    inst:cutoff             (instrument of selected/last track)
```

Track model tree (address → AutomatableModel):

| Address suffix | Model |
|---|---|
| `volume`, `pan`, `pitch`, `pitch_range`, `base_note`, `first_key`, `last_key`, `mixer_channel` | InstrumentTrack public models |
| `filter.enabled`, `filter.type`, `filter.cutoff`, `filter.reso` | InstrumentSoundShaping |
| `env.predelay`, `env.attack`, `env.hold`, `env.decay`, `env.sustain`, `env.release`, `env.amount` | volume-target envelope; `env.<target>.<p>` for cutoff/resonance targets |
| `lfo.amount`, `lfo.speed`, `lfo.wave` | volume-target LFO; `lfo.<target>.<p>` likewise |
| `arp.enabled`, `arp.chord`, `arp.range`, `arp.direction`, `arp.time`, `arp.gate`, `arp.mode` | arpeggio |
| `ns.enabled`, `ns.type`, `ns.range` | note stacking |
| `midi.cc.<n>` | midiCCModel(n) |

Effect params: `fx:<channel>.<effectDisplayName>.<paramDisplayName>` where
params are enumerated via `findChildren<AutomatableModel>()` on
`EffectControls` (displayName + range + current value). VST-hosted params are
addressed `fx:<channel>.<vst>.<index>` additionally.

### 1.4 Errors

`error_code` is stable and machine-readable: `unknown_tool`, `tool_failed`,
`bad_address`, `bad_args`, `not_implemented`, `not_available` (e.g. peak
metering without core patch), `render_busy`, `not_found`. `error_message` is
human-readable. Connection failures happen before the envelope (TCP level).

## 2. Tool surface

### 2.0 Existing v1 tools (unchanged wire names)

`getprojectstate`, `listtracks`, `gettrackdetails`, `listpatterns`,
`listinstruments`, `listeffects`, `listtoolwindows`, `getselectionstate`,
`findtrackbyname`, `searchprojectaudio`, `createtrack`, `renametrack`,
`loadinstrument`, `loadsample`, `createpattern`, `addnotes`, `addsteps`,
`settempo`, `addeffect`, `removeeffect`, `seteffectparam`, `opentool`,
`importaudio`, `importmidi`, `importhydrogen`, `selecttrack`, `mutetrack`,
`solotrack`, `createsnapshot`, `undolastaction`, `rollbacktosnapshot`,
`diffsincesnapshot`.

`seteffectparam` is re-implemented in v2 (see 2.9).

### 2.1 Project & transport

| Tool | Args | Semantics |
|---|---|---|
| `new_project` | `{}` | Queued on main window; confirm-gated (project.new). |
| `open_project` | `{path}` | Queued; confirm-gated. |
| `save_project` | `{}` | Save current project file. |
| `save_project_as` | `{path}` | Save to path. |
| `set_time_signature` | `{numerator, denominator}` | Song meter model. |
| `set_metronome` | `{enabled}` | Metronome::setActive. |
| `set_master_volume` | `{value 0..1}` | Song master volume model. |
| `set_master_pitch` | `{value}` | Song master pitch model. |
| `set_play_pos` | `{tick}` or `{bar, beat, tick}` | Song::setPlayPos (PlayMode::Song). |
| `set_loop` | `{begin_tick, end_tick}` | Timeline loop enabled + bounds. |
| `clear_loop` | `{}` | Timeline loop disabled. |
| `set_stop_behaviour` | `{mode: back_to_zero\|back_to_start\|continue}` | Timeline. |
| `play_pattern` | `{pattern?}` | Song::playPattern. |
| `play_clip` | `{track, clip_index?}` | Song::playMidiClip. |
| `insert_bar` | `{at_tick}` | Song::insertBar (invokeMethod). |
| `remove_bar` | `{at_tick}` | Song::removeBar. |

### 2.2 Tracks & clips (arrangement)

| Tool | Args | Semantics |
|---|---|---|
| `create_clip` | `{track?, tick, name?}` | Track::createClip; returns clip_index. |
| `move_clip` | `{track, clip_index, new_tick}` | Clip::movePosition. |
| `resize_clip` | `{track, clip_index, new_length}` | changeLength + setAutoResize(false). |
| `split_clip` | `{track, clip_index, at_tick}` | clone + changeLength + movePosition. |
| `clone_clip` | `{track, clip_index, to_tick?}` | duplicate at to_tick (default: end of source). |
| `delete_clip` | `{track, clip_index}` | Track::removeClip. |
| `set_clip_mute` | `{track, clip_index, mute}` | Clip::setMuted. |
| `set_clip_name` | `{track, clip_index, name}` | Clip::setName. |
| `set_clip_color` | `{track, clip_index, color}` | Clip::setColor (hex `#rrggbb`). |
| `clone_track` | `{track, name?}` | Track::clone (XML round-trip). |
| `delete_track` | `{track}` | TrackContainer::removeTrack; confirm-gated. |
| `move_track` | `{track, index}` | TrackContainer::moveTrack. |
| `set_track_volume` | `{track, value 0..1}` | track volume model. |
| `set_track_pan` | `{track, value 0..1}` | pan model. |
| `set_track_pitch` | `{track, value}` | pitch model. |
| `set_track_key_range` | `{track, first_key, last_key}` | firstKey/lastKey models. |
| `set_track_base_note` | `{track, key}` | baseNote model. |

### 2.3 Notes

| Tool | Args | Semantics |
|---|---|---|
| `add_notes` (v1) | `{track?, clip_index?, notes:[{key,pos,length,velocity}]}` | unchanged |
| `edit_notes` | `{track, clip_index?, notes:[{key,pos,length?,velocity?,pan?}]}` | update notes matching (key,pos); absent fields unchanged |
| `remove_notes` | `{track, clip_index?, keys:[], pos?}` | remove matching notes |
| `clear_clip` | `{track, clip_index?}` | MidiClip::clearNotes |
| `quantize_clip` | `{track, clip_index?, resolution: 1..192}` | Note::quantizePos per note (1/192 = free) |
| `humanize_clip` | `{track, clip_index?, amount 0..1}` | jitter pos + velocity deterministically (seeded) |
| `reverse_clip` | `{track, clip_index?}` | MidiClip::reverseNotes |
| `split_clip_notes` | `{track, clip_index?, at_tick}` | MidiClip::splitNotesAlongLine |
| `set_clip_velocity_scale` | `{track, clip_index?, scale 0..2}` | multiply all note velocities (clamped) |
| `add_chord` | `{track?, clip_index?, root, chord, pos?, length?, velocity?}` | chord table (major/minor/7/maj7/min7/dim/sus2/sus4/power) → add_notes expansion |
| `add_arpeggio` | `{track?, clip_index?, root, chord, direction: up\|down\|updown\|random, steps, pos?, step_len?, velocity?, octaves?}` | generator over chord table |

### 2.4 Patterns (BB)

| Tool | Args | Semantics |
|---|---|---|
| `create_pattern` | `{name?}` | PatternStore::addPattern; sets current |
| `select_pattern` | `{pattern}` | PatternStore::setCurrentPattern |
| `clone_pattern` | `{pattern, name?}` | duplicate pattern + clips |
| `set_steps` | `{track, clip_index?, steps:[0..127], clear_existing?}` | superset of addsteps |
| `set_step_velocity` | `{track, clip_index, step, velocity 1..127}` | step note volume |
| `set_steps_per_bar` | `{track, steps}` | MidiClip steps scaling |
| `add_rhythm` | `{drum: kick\|snare\|hihat\|crash\|ride, pattern:[0..15]}` | typed rhythm preset (addkick family) |

### 2.5 Samples

| Tool | Args | Semantics |
|---|---|---|
| `set_sample_loop` | `{track, clip_index?, mode: off\|on\|pingpong, loop_start?, loop_end?}` | Sample loop config |
| `set_sample_pitch` | `{track, clip_index?, semitones}` | Sample frequency ratio 2^(s/12) |
| `set_sample_amp` | `{track, clip_index?, value 0..1}` | Sample::setAmplification |
| `set_sample_range` | `{track, clip_index?, start_frame, length_frames}` | SampleClip region |

### 2.6 Instruments

| Tool | Args | Semantics |
|---|---|---|
| `load_instrument` (v1) | unchanged | |
| `load_instrument_preset` | `{track?, path, plugin?}` | Instrument::loadFile (xiz/sf2/pat/xp/VST) |
| `set_vst_program` | `{track, program}` | Vestige VstPlugin::setProgram |
| `set_vst_param` | `{track, param, value}` | VstPlugin::setParam by index |
| `set_sf2_patch` | `{track, bank?, patch}` | Sf2Player childModel |
| `describe_instrument` | `{track?}` | plugin name, patch, param tree (see 2.12) |

### 2.7 Sound shaping

All take `{track?, ...}`; values use model units (ranges from describe).

| Tool | Args |
|---|---|
| `set_envelope` | `{target?: volume\|cutoff\|resonance, predelay?, attack?, hold?, decay?, sustain?, release?, amount?}` |
| `set_filter` | `{enabled?, type?: lowpass\|highpass\|bandpass\|notch\|allpass\|moog\|2xlowpass, cutoff?, resonance?}` |
| `set_lfo` | `{target?: volume\|cutoff\|resonance, amount?, speed?, wave?, delay?, attack?}` |
| `set_arp` | `{enabled?, chord?, range?, direction?, time?, gate?, mode?}` |
| `set_note_stacking` | `{enabled?, type?, range?}` |

### 2.8 Effects

| Tool | Args | Semantics |
|---|---|---|
| `set_effect_param` | `{track?, effect, param, value}` | findChildren param by displayName; setValue in model units |
| `get_effect_params` | `{track?, effect}` | param list: displayName, type, min, max, value |
| `move_effect` | `{track?, effect, direction: up\|down}` | EffectChain::moveUp/moveDown |
| `set_effect_enabled` | `{track?, effect, enabled}` | Effect enabled model |
| `set_effect_wetdry` | `{track?, effect, value 0..1}` | wetDry model |

### 2.9 Mixer

| Tool | Args | Semantics |
|---|---|---|
| `list_mixer_channels` | `{}` | channels: index, name, volume, mute, solo, effects, sends |
| `create_channel` | `{name?}` | Mixer::createChannel; returns index |
| `delete_channel` | `{channel}` | Mixer::deleteChannel; confirm-gated |
| `rename_channel` | `{channel, name}` | |
| `move_channel` | `{channel, direction: left\|right}` | |
| `set_channel_volume` | `{channel, value 0..1}` | |
| `set_channel_mute` | `{channel, mute}` | |
| `set_channel_solo` | `{channel, solo}` | |
| `add_channel_effect` | `{channel, effect}` | channel fxChain appendEffect |
| `remove_channel_effect` | `{channel, effect}` | |
| `create_send` | `{from, to, amount? 0..1}` | Mixer::createChannelSend |
| `delete_send` | `{from, to}` | |
| `set_send_amount` | `{from, to, amount}` | channelSendModel setValue |
| `route_track_to_channel` | `{track, channel}` | mixerChannelModel |
| `get_peak_levels` | `{channel?}` | per-channel peak readback (not_available if core patch absent) |

Channels are addressed by name or `n` (0 = master).

### 2.10 Automation

| Tool | Args | Semantics |
|---|---|---|
| `create_automation` | `{track?, address?, name?}` | new AutomationTrack+clip bound to model at address; returns clip id |
| `automate` | `{address, ticks:[int], values:[0..1], clip?}` | putValues on the model's clip (global or track clip) |
| `set_automation_node` | `{clip, tick, value}` | putValue single node |
| `remove_automation_node` | `{clip, tick}` | removeNode at tick |
| `set_automation_tension` | `{clip, tick?, tension -1..1}` | per-node tension |
| `set_automation_progression` | `{clip, progression: discrete\|linear\|cubic}` | |
| `global_automate` | `{address, ticks:[], values:[]}` | AutomationClip::globalAutomationClip(model) + putValues |
| `read_automation` | `{clip}` or `{address}` | nodes: [(tick, value)] + tension + progression |
| `list_automation` | `{}` | automation clips: id, track, bound model address |

Clip ids: `global:<address>` or `auto:<track>:<clip_index>`.

### 2.11 Controllers

| Tool | Args | Semantics |
|---|---|---|
| `create_controller` | `{type: lfo\|midi\|peak, name?}` | Song::addController; returns controller id |
| `set_lfo_controller` | `{controller, wave?, speed?, amount?, base?, phase?, multiplier?}` | LfoController models |
| `connect_controller` | `{controller, address}` | model->setControllerConnection |
| `disconnect_controller` | `{address}` | remove connection |
| `describe_controllers` | `{}` | controllers: id, type, params, connections |

### 2.12 Render / export

| Tool | Args | Semantics |
|---|---|---|
| `render_song` | `{path, format?: wav\|flac\|ogg\|mp3, sample_rate?, bit_depth?, bitrate?, stereo_mode?}` | RenderManager::renderProject async; `render_id` returned |
| `render_tracks` | `{dir, prefix?, format?, sample_rate?, bit_depth?, bitrate?, stereo_mode?}` | renderProjectTracks stems |
| `render_preview` | `{path?, format?, begin_tick?, end_tick?}` | loop-range render (default: loop points or whole song) |
| `get_render_progress` | `{render_id?}` | progress 0..1, done, output path |
| `cancel_render` | `{render_id?}` | abortProcessing |
| `export_midi` | `{path}` | Song::exportProjectMidi |

Render is async; the plugin stores the active RenderManager per render_id and
poll progress via `get_render_progress`. One render at a time.

### 2.13 MIDI record (plugin-side capture)

| Tool | Args | Semantics |
|---|---|---|
| `record_arm` | `{track?, clip_index?}` | arm capture: plugin subscribes its own MidiPort to readable input; target clip = selected/last instrument clip |
| `record_disarm` | `{}` | drop capture subscription |
| `record_start` | `{}` | Song::playAndRecord + begin capture |
| `record_stop` | `{quantize?: 1..192}` | stop transport; insert captured notes into armed clip; optional quantize |

Capture: plugin MidiPort receives note events on the driver thread, queues
(mutex), GUI-thread timer drains into the clip at captured tick positions
(Song::getPlayPos at event time). `hints` report dropped events if the queue
overflows.

### 2.14 Misc

| Tool | Args |
|---|---|
| `describe_song` | `{}` — full state: tempo, sig, tracks+clips, mixer, patterns, automation, controllers, project file, notes, microtuner |
| `set_project_notes` | `{text}` |
| `get_project_notes` | `{}` |
| `set_microtuner` | `{enabled?, scale?, keymap?}` |

## 3. Introspection shapes

`describe_track {track?}` result:

```json
{ "track": "Lead", "type": "instrument", "index": 2,
  "params": [ { "address": "track:Lead.volume", "display": "Volume",
                "type": "float", "min": 0, "max": 1, "step": 0.01,
                "value": 0.8, "automated": false, "controller": null } ],
  "clips": [ { "index": 0, "name": "Lead 1", "start": 0, "length": 768,
               "notes": 32, "steps": 16, "type": "melody" } ] }
```

`describe_effect {track?|channel?, effect?}` returns the effect chain with
each effect's param list. `describe_instrument` returns plugin name, patch
name, and params (VST/Zyn/Sf2 included). `describe_controllers` returns
controller params and connected addresses.

## 4. Implementation notes

- All tools run on the GUI thread (AgentControl QTcpServer pattern). Mutations
  during playback wrap in `Engine::audioEngine()->requestChangesGuard()`.
- Project transitions (new/open) stay queued via `QTimer::singleShot(0,
  mainWindow(), ...)` and serialized by the existing transition flag.
- Confirmation gate applies to: new/open project, delete track, delete
  channel, clear clip, render overwrite of existing file, global automation
  writes, controller disconnect.
- New files: `agent_mixer.cpp`, `agent_automation.cpp`, `agent_render.cpp`,
  `agent_arrangement.cpp`, `agent_sound.cpp`, `agent_record.cpp` — each
  defines the `AgentControlService::dispatch*Tool(...)` entry declared in
  `AgentControl.h`. CMakeLists.txt lists the files explicitly.
