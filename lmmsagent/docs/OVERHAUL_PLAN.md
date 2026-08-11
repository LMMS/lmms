# LMMS Agent Overhaul Plan

**Status:** implemented (P0-P4 complete, 2026-08-10; P5 shipped as plugin-side capture)
**Date:** 2026-08-10
**Scope:** `plugins/AgentControl` (typed tool server inside LMMS), `lmmsagent/` (planner/orchestrator/discovery/playbooks), `mcp/` operator server (LLM-facing surface)

---

## 1. Executive summary

The current agent stack has 31 typed tools and 18 NL intents and covers roughly
"beat + basic track control". LMMS itself is a full DAW: arrangement, piano
roll, automation (any knob, song-global or per-track), FX mixer with sends and
routing, controllers (LFO/peak/MIDI), sound shaping (ADSR/filter/LFO/arpeggio),
VST + Zyn + SoundFont hosting, per-track stem export, MIDI/Hydrogen import, and
a microtuner. Research (8 codebase scouts + official manual + community forums)
shows **~80% of the DAW surface is already reachable from a GUI-thread
ToolPlugin through public core APIs** — the agent simply never exposed it.
Most of the rest needs only small one-line accessor patches to LMMS core
headers (~12 getters). Only genuinely hard items (MIDI note recording,
quantize-on-record) require real core feature work.

**The overhaul: expand AgentControl from 31 tools to ~90 typed tools across 14
DAW domains, with a discovery-first API (introspection tools so any LLM can
learn what it can touch), then rework the Python planner and the MCP operator
server on top of the new surface.**

---

## 2. Research basis

- 8 scout agents over the checkout (Song core, tracks/clips, plugin surface,
  automation/controllers, import/export/render, GUI editors, MIDI/transport,
  current agent stack). All source claims below carry `path:line` in the
  capability tables of those reports (`history://SongCore`, `TracksClips`,
  `PluginSurface`, `AutomationControllers`, `ImportExportRender`, `GuiEditors`,
  `MidiTransport`, `AgentStackCurrent`).
- Official user manual (1.3.x) — `docs.lmms.io/user-manual/llms.txt` and key
  pages: Your First Song, Song Editor, Piano Roll, Automation, Mixer, Controller
  Rack, Instrument Window, Exporting.
- Community: r/lmms workflows, LMMS forums (automation/VST/export pain points),
  reviews (strengths: piano roll, scale/chord marking, free, unlimited tracks;
  weaknesses: no audio recording, VST friction).
- `lmmsagent/docs/MANUAL_FEATURE_COVERAGE.md` + `MANUAL_TASK_MAP.md` (existing
  gap list, confirmed and extended).

---

## 3. Current state (verified)

### 3.1 What exists today

| Layer | Surface | Notes |
|---|---|---|
| AgentControl plugin | 31 typed tools (10 read / 17 write / 4 safety) | TCP 7777, JSON-line `{tool,args}` → envelope `{ok,result,state_delta,warnings,error_code,error_message}` |
| NL interpreter | 18 manifest intents, deterministic + heuristic + Ollama, confirmation gate, snapshots/undo/rollback/diff | `command_manifest.v3.json` (canonical; v2 kept for compatibility) |
| Python stack | planner → orchestrator → discovery + memory + ToolClient; daemon `run_goal`/`health`/`warmup` | planner emits snake_case actions; `resolve_*`/`guide_note` handled in orchestrator |
| Playbooks | 17 beginner playbooks as real v2 tool sequences | `manual_playbooks.py` |
| MCP | `mcp/` dev server (read-only) + `mcp/` operator server (39 tools, thin proxies) | operator built 2026-08-10 |

### 3.2 Verified gaps (what a DAW assistant must do but cannot)

| # | Gap | Evidence |
|---|---|---|
| 1 | **No automation editing** — cannot create automation clips, bind a model, draw nodes, set tension/progression | no automation tools in `dispatchTool` (AgentControl.cpp:2434-2825); `AutomationClip` API is fully public (include/AutomationClip.h) |
| 2 | **No arrangement/clip ops** — no move/resize/split/clone clips, insert/remove bars, loop points | `Clip::movePosition/changeLength/clone`, `Track::insertBar/removeBar` exist (include/Clip.h, include/Track.h) |
| 3 | **No mixer** — no channel create/rename/delete, sends, routing, channel FX | `Mixer::createChannel/createChannelSend/channelSendModel/clearChannel` public (include/Mixer.h:165-197) |
| 4 | **No instrument/effect parameter tweaking** — `seteffectparam` is `not_implemented` stub | AgentControl.cpp:2714-2717; all knobs are `AutomatableModel` children, enumerable via `findChildren` (PluginSurface report) |
| 5 | **No sound shaping** — ADSR, filter, LFO, arpeggio, note stacking | `InstrumentSoundShaping::getFilterCutModel` etc. public but `m_soundShaping` private (include/InstrumentTrack.h) — accessor patch |
| 6 | **No export/render** — `exportsong`/`rendersong` return a text hint only | AgentControl.cpp export handling; `RenderManager::renderProject/renderTracks` fully usable (include/RenderManager.h) |
| 7 | **No save/open/new as typed tools** — NL-only | no dispatchTool entries; `saveProject` exists in plugin but unreachable as tool |
| 8 | **No controllers** — no LFO/MIDI/Peak controllers, no model→controller connection | `Song::addController`, `Controller::create`, `setControllerConnection` public |
| 9 | **No pattern/BB depth** — no pattern create/select/clone, no per-step velocity, no steps-per-bar | `PatternStore`, `MidiClip::addSteps/cloneSteps/setStep` public |
| 10 | **No transport depth** — no seek, no loop points, no stop-behaviour, no metronome | `Timeline::setLoopBegin/End/Enabled`, `Song::setPlayPos`, `Metronome::setActive` public |
| 11 | **No sample manipulation** — loop mode, pitch, loop points, amplification | `Sample::setLoopMode/setStartFrame/setAmplification` public (include/Sample.h) |
| 12 | **No recording** (hard) | `Song::record()` is a `TODO: Implement` stub (Song.cpp:522); no MIDI-note capture in core |
| 13 | **No quantization on entry** (medium) | editor-GUI-local; `Note::quantizePos` exists (include/Note.h) so plugin-side quantize is implementable |
| 14 | **No introspection** — an LLM cannot enumerate what knobs/params/models exist before acting | nothing like `describe_*` tools |

### 3.3 Why the surface was so small

The plugin was built as a voice-first demo (beat + transport + slicer). The
Python planner's action vocabulary (`set_tempo, open_tool, create_track,
load_*, add_/remove_effect, mute/solo, undo, rollback, guide_note,
resolve_*`) caps the whole stack: **the planner can only emit what the plugin
can execute**, and the plugin only implements a fraction of what LMMS exposes.

---

## 4. Target capability matrix

Reachability legend:

- **READY** — public core API, callable from AgentControl (GUI thread) as-is.
- **PATCH** — needs a small accessor/getter added to a core header (each ≤ ~5 lines, upstreamable).
- **CORE** — real core feature work (days+).
- **GUI** — only reachable via GUI internals; avoid or route through invokeMethod on MainWindow slots.

### 4.1 Project & transport

| Capability | API (source) | Reach |
|---|---|---|
| new/open/save project (typed) | `MainWindow` public slots or deferred `Song::createNewProject/loadProject/guiSaveProject` | READY (defer via QTimer::singleShot, AgentControl.cpp:2960 pattern) |
| play / play pattern / play clip / pause / stop / record | `Song::playSong/playPattern/playMidiClip/togglePause/stop/playAndRecord` (include/Song.h) | READY (play/pause/stop exist) |
| seek | `Song::setPlayPos`, `Timeline` per-mode playhead | READY |
| loop points | `Timeline::setLoopBegin/End/Enabled` | READY |
| stop behaviour / play start position | `Timeline::setStopBehaviour/setPlayStartPosition` | READY |
| tempo | `Song::tempoModel`/`setTempo` | READY (exists) |
| time signature | `TimeSig`/`MeterModel` (include/TimePos.h) | READY |
| metronome | `Metronome::setActive` | READY |
| master volume / pitch | `Song::m_masterVolumeModel/m_masterPitchModel` — private | PATCH (getters) |
| insert/remove bar | `Song::insertBar/removeBar` — private slots | PATCH (QMetaObject::invokeMethod or make public) |

### 4.2 Tracks & clips (arrangement)

| Capability | API | Reach |
|---|---|---|
| create/remove/clone/reorder tracks | `Track::create`, `TrackContainer::addTrack/removeTrack/moveTrack` | READY (create exists) |
| rename, mute, solo, color | `Track::setName/setMuted/setSolo/setColor` | READY (rename/mute/solo exist) |
| volume/pan/pitch per track | `InstrumentTrack::volumeModel/panningModel/pitchModel`; `SampleTrack` volume/pan private | READY / PATCH (SampleTrack getters) |
| base note / key range | `baseNoteModel/firstKeyModel/lastKeyModel` | READY |
| clip create/move/resize/split/clone/delete | `Track::createClip`, `Clip::movePosition/changeLength/clone`, `Track::removeClip` | READY |
| clip mute, name, color, time-offset | `Clip::toggleMute/setName/setColor/setStartTimeOffset` | READY |
| insert/remove bars across song | `Song::insertBar/removeBar` | PATCH |

### 4.3 Piano roll & notes

| Capability | API | Reach |
|---|---|---|
| add/remove/clear notes | `MidiClip::addNote/removeNote/clearNotes` (include/MidiClip.h) | READY (exists, extend) |
| note velocity/pan per note | `Note::setVolume/setPanning` | READY |
| quantize | `Note::quantizePos/quantizeLength` | READY (plugin-side helper) |
| reverse / split | `MidiClip::reverseNotes/splitNotes/splitNotesAlongLine` | READY |
| detune profile | `Note::createDetuning` + AutomationClip | READY |
| clip type (beat/melody) | `MidiClip::type`/setType (private setter) | PATCH |

### 4.4 Patterns (Beat+Bassline successor: PatternStore/PatternEditor)

| Capability | API | Reach |
|---|---|---|
| pattern create/select/list | `PatternStore::addPattern/currentPattern/numOfPatterns` (include/PatternStore.h) | READY |
| steps add/clone/remove | `MidiClip::addSteps/cloneSteps/removeSteps` | READY |
| step on/off | `MidiClip::setStep/addStepNote/noteAtStep` | READY (exists) |
| per-step velocity | step note `Note::setVolume` | READY (new — high value: the manual's dimmed-step accents) |
| pattern clip arrangement | `PatternClip`, `PatternTrack` | READY |

### 4.5 Instruments

| Capability | API | Reach |
|---|---|---|
| load instrument by name | `InstrumentTrack::loadInstrument` | READY (exists, fuzzy + aliases) |
| load preset/patch file | `Instrument::loadFile` (Zyn `.xiz`, Sf2 `.sf2`, PatMan, VesTige VST `.dll/.so`) | READY (new: `load_instrument_preset`) |
| **generic param get/set** | `findChildren<AutomatableModel>()` on Instrument; `displayName()` + `setValue()`; ranges | READY (the big unlock) |
| Sf2/Gig bank+patch | `childModel("bank"/"patch")` (Sf2Player.cpp:339-350) | READY |
| VST program/param | `VstPlugin::setParam/setProgram/allParameterLabels/allProgramNames/parameterDump` (VstBase/VstPlugin.h) | READY |
| Zyn patch knobs | `ZynAddSubFxInstrument` public FloatModels + `loadFile` | READY |
| arpeggio + note stacking config | `InstrumentFunctions` models — private | PATCH (getters) |

### 4.6 Sound shaping (the "producer" domain)

| Capability | API | Reach |
|---|---|---|
| ADSR + amount (volume/cutoff/reso targets) | `InstrumentSoundShaping::getVolume/Cutoff/ResonanceParameters` → `EnvelopeAndLfoParameters` (predelay/attack/hold/decay/sustain/release/amount) | PATCH (`m_soundShaping` getter) |
| filter type / cutoff / Q / enabled | `getFilterEnabledModel/getFilterModel/getFilterCutModel/getFilterResModel` | PATCH (same getter) |
| LFO (amount/speed/wave) | `EnvelopeAndLfoParameters` LFO getters | PATCH |
| arp (on/chord/range/direction/time/gate/mode) | `InstrumentFunctions` — private | PATCH |

### 4.7 Effects

| Capability | API | Reach |
|---|---|---|
| add/remove/reorder/enable | `EffectChain::appendEffect/removeEffect/moveUp/moveDown`; `Effect::enabled` model | READY (exists) |
| wet/dry | `Effect::m_wetDryModel` | READY |
| **generic param get/set** (implement `seteffectparam`) | `effect->controls()` → `findChildren<AutomatableModel>()`, `controlCount()` | READY |
| per-effect presets save/restore | `Effect::saveSettings/loadSettings` XML | READY (low priority) |

### 4.8 Mixer (new domain)

| Capability | API | Reach |
|---|---|---|
| channel create/delete/rename/reorder | `Mixer::createChannel/deleteChannel/moveChannelLeft/Right`, `MixerChannel::setName` | READY |
| channel volume/mute/solo | `MixerChannel::m_volumeModel/m_muteModel/m_soloModel` | READY |
| channel FX chain | `MixerChannel::m_fxChain` (EffectChain) | READY |
| **sends + amounts** | `Mixer::createChannelSend/deleteChannelSend/channelSendModel(from,to)` | READY |
| track → channel routing | `InstrumentTrack::mixerChannelModel` / `SampleTrack::mixerChannelModel` | READY |
| **peak metering readback** (clip detection) | `MixerChannel` peak values | PATCH (expose readback) |

### 4.9 Automation (the biggest gap, highest value)

| Capability | API | Reach |
|---|---|---|
| create automation track + clip | `Track::create(Automation)`, `AutomationTrack::createClip` | READY |
| bind model | `AutomationClip::addObject(model)` | READY |
| **draw nodes** | `AutomationClip::putValue/putValues(time,val)`, `removeNode/removeNodes/resetNodes` | READY |
| tension / progression | `setTension`, `progressionType` (Discrete/Linear/CubicHermite) | READY |
| quantization | `AutomationClip::setQuantization` | READY |
| song-global automation | `AutomationClip::globalAutomationClip(model)` (Song.cpp:69, AutomationClip.cpp:1009) | READY |
| read value at time | `valueAt` / `getTimeMap` | READY |
| model enumeration | `ModelVisitor` (include/ModelVisitor.h) | READY |

### 4.10 Controllers (new domain)

| Capability | API | Reach |
|---|---|---|
| create controller | `Song::addController(new LfoController(song))`, `Controller::create` | READY |
| LFO config (wave/base/speed/amp/phase/multiplier) | `LfoController` params — protected | PATCH (getters) |
| connect model → controller | `model->setControllerConnection(new ControllerConnection(lfo))` | READY |
| MIDI CC learn/map | `MidiController::setInputController/setInputChannel` — via protected `m_midiPort` | PATCH (accessor) |
| peak controller (sidechain ducking) | Peak controller effect + connection | READY (medium) |

### 4.11 Import / export / render (new domain, high value)

| Capability | API | Reach |
|---|---|---|
| import MIDI / Hydrogen | `ImportFilter::import` (MidiImport, HydrogenImport) | READY (import exists) |
| export MIDI | `Song::exportProjectMidi` | READY |
| **render song to file** | `RenderManager::renderProject` + `OutputSettings` (sampleRate, bitDepth 16/24/32f, stereo mode, compression); WAV/FLAC/OGG/MP3 | READY (async QThread; progress signal) |
| **render per-track stems** | `RenderManager::renderTracks` (mute/unmute per track) | READY |
| render ranges (loop/markers) | `Song::setRenderBetweenMarkers/setExportLoop/setLoopRenderCount` | READY |
| preview render while iterating | same render pipeline, loop range | READY |

### 4.12 Samples

| Capability | API | Reach |
|---|---|---|
| load sample | `SampleClip::setSampleFile` | READY (exists) |
| loop mode / loop points | `Sample::setLoopMode/setLoopStartFrame/setLoopEndFrame` (Off/On/PingPong) | READY |
| pitch / frequency | `Sample::setFrequency`, play ratio | READY |
| amplification / reversed | `Sample::setAmplification/setReversed` | READY |
| start/play-length | `SampleClip::setSampleStartFrame/setSamplePlayLength` | READY |
| sample asset discovery | data/samples (drumsynth TR909/808, waveforms, stringsnpads, latin loops), data/presets (Zyn banks), data/projects demos+templates | READY (extend DiscoveryIndex) |

### 4.13 Misc

| Capability | API | Reach |
|---|---|---|
| project notes | `ProjectNotes::setText` — no getter | PATCH (getter) |
| microtuner on/scale/keymap | `Microtuner::enabledModel/scaleModel/keymapModel` | READY |
| custom scale definition | `MicrotunerConfig` — GUI-only | GUI (skip) |
| MIDI out / program change | `MidiPort::processOutEvent`, `InstrumentTrack::midiPort` | READY |
| undo/redo | `MainWindow::undo/redo` slots or journal | READY (undo exists) |

### 4.14 Recording (hard, future)

| Capability | Reality |
|---|---|
| MIDI note recording into clips | `Song::record()` is a stub; no core capture path. Needs real core work (audio-thread MIDI capture → MidiClip insert), OR a plugin-side capture: subscribe to `MidiClient` events on the driver thread and queue → GUI thread note insertion. Feasible as plugin code but risky (threading). |
| quantize-on-record | no core hook; implementable plugin-side with `Note::quantizePos` once capture exists |
| audio recording | LMMS cannot record audio at all — out of scope (upstream limitation) |

---

## 5. Tool API design (v2)

### 5.1 Envelope (keep v1 wire format, extend)

```json
{ "ok": true, "result": {}, "state_delta": {}, "warnings": [],
  "error_code": null, "error_message": null,
  "hints": [] }
```

- `hints[]`: contextual suggestions the plugin can emit ("track did not exist; created 'Lead'", "tempo was automated; setValue will be overridden during playback"). Keeps the LLM honest without polling.
- Time units: **ticks** (192/bar default) everywhere; tempo-relative where the API is (LFO speed TempoSync).
- Pitch: **MIDI note numbers** (60 = C4 per LMMS convention), velocities 1-127 (plugin normalizes to Note volume 0..1 internally? No — keep LMMS units: Note volume/panning are FloatModel 0..1 scaled; expose as 0..1 floats and document; velocity 1-127 maps via setVolume(v/127)). Decide in implementation; schema must state units.

### 5.2 Model addressing (the key design decision)

Every automatable thing gets a stable address so the LLM can name it without C++ knowledge:

```
track:<name>.volume            track:<name>.pan             track:<name>.pitch
track:<name>.filter.cutoff     track:<name>.filter.reso     track:<name>.env.attack
track:<name>.arp.enabled       track:<name>.arp.pattern
fx:<channel>.reverb.wet        fx:<channel>.eq.band1.gain   (by effect displayName + control displayName)
inst:<plugin>.<param>          (VST/Zyn via param index or name)
song.master.volume             song.tempo
```

- `describe_track <ref>` / `describe_fx <channel>` / `describe_controller` return the full model tree: displayName, type, min/max/step, unit, current value, isAutomated, controller connection. **This is the discovery-first API that makes the assistant generically capable without hard-coding every knob.**
- `set_param <address> <value>` and `automate <address> <clip> [...]` operate on addresses.

### 5.3 New tool families (delta vs today's 31)

| Family | New tools (name — purpose) |
|---|---|
| Project | `new_project`, `open_project`, `save_project`, `save_project_as`, `insert_bar`, `remove_bar`, `set_time_signature`, `set_metronome`, `set_master_volume`, `set_master_pitch` |
| Transport | `set_play_pos`, `set_loop`, `clear_loop`, `set_stop_behaviour`, `play_pattern`, `play_clip` |
| Arrangement | `create_clip`, `move_clip`, `resize_clip`, `split_clip`, `clone_clip`, `delete_clip`, `set_clip_mute`, `set_clip_name`, `set_clip_color`, `clone_track`, `delete_track`, `move_track`, `set_track_volume`, `set_track_pan`, `set_track_pitch`, `set_track_key_range`, `set_track_base_note` |
| Notes | `quantize_clip`, `reverse_clip`, `split_clip_notes`, `set_note_velocity`, `set_note_pan`, `clear_clip`, `add_chord` (root + chord type → notes), `add_arpeggio_pattern` (sequence generator) |
| Patterns | `create_pattern`, `select_pattern`, `clone_pattern`, `set_steps`, `set_step_velocity`, `set_steps_per_bar` |
| Instruments | `load_instrument_preset`, `get_param`, `set_param`, `describe_instrument`, `set_vst_program`, `set_vst_param`, `set_sf2_patch`, `set_zyn_knob` |
| Sound shaping | `set_envelope`, `set_filter`, `set_lfo`, `set_arp`, `set_note_stacking` (each takes address-targeted args) |
| Effects | `set_effect_param` (implement!), `move_effect`, `set_effect_enabled`, `set_effect_wetdry`, `describe_effect` |
| Mixer | `create_channel`, `delete_channel`, `rename_channel`, `move_channel`, `set_channel_volume`, `set_channel_mute`, `set_channel_solo`, `add_channel_effect`, `remove_channel_effect`, `create_send`, `delete_send`, `set_send_amount`, `route_track_to_channel`, `get_peak_levels` |
| Automation | `create_automation`, `automate` (address, clip, node list), `set_automation_node`, `remove_automation_node`, `set_automation_tension`, `set_automation_progression`, `global_automate`, `read_automation` |
| Controllers | `create_controller` (lfo|midi|peak), `set_lfo_controller`, `connect_controller`, `disconnect_controller`, `describe_controllers` |
| Render | `render_song`, `render_tracks`, `render_preview` (loop-range WAV for quick audition), `get_render_progress` |
| Samples | `set_sample_loop`, `set_sample_pitch`, `set_sample_amp`, `set_sample_range`, `search_samples` (extend discovery) |
| Misc | `set_project_notes`, `get_project_notes`, `set_microtuner`, `set_midi_out`, `list_patterns` (exists), `list_controllers` |

Count: ~60 new tools + 31 existing → **~90 total**. Every tool keeps the
envelope + `state_delta` + snapshots/undo integration.

### 5.4 Safety model (keep and extend)

- `describe_*` is read-only. Writes return `state_delta`.
- Risk classes per intent stay: safe / confirm / destructive. New confirms:
  new/open project (exists), delete track, delete channel, render overwrite,
  clear clip, global automation (song-global writes), controller disconnect.
- `create_snapshot` before any multi-step write sequence is enforced by the
  Python orchestrator (exists) and now also by the MCP operator tool
  descriptions.

---

## 6. Core patches (small, upstreamable)

All are getter additions; each ≤ ~5 lines in a header. **Do these first — they
unlock 3 whole domains (sound shaping, controllers, master bus).**

| Header | Addition |
|---|---|
| `include/InstrumentTrack.h` | `InstrumentSoundShaping* soundShaping()`; `InstrumentFunctionArpeggio* arpeggio()`; `InstrumentFunctionNoteStacking* noteStacking()` |
| `include/SampleTrack.h` | `FloatModel* volumeModel()`; `FloatModel* panningModel()` |
| `include/Song.h` | `FloatModel* masterVolumeModel()`; `IntModel* masterPitchModel()`; make `insertBar/removeBar` public |
| `include/LfoController.h` | getters for base/speed/amount/phase/wave/multiplier models |
| `include/MidiController.h` | `MidiPort* midiPort()` (or setInputController/setInputChannel wrappers) |
| `include/ProjectNotes.h` | `QString text()` |
| `include/Mixer.h` / `MixerChannel` | peak level readback (lastPeakL/R or similar) |
| `include/MidiClip.h` | `setType` public (or `setClipType`) |

---

## 7. NL intent taxonomy v3 (manifest)

`command_manifest.v3.json` supersedes v2 (18 intents) with ~70 intents mapped
to typed AgentControl v2 tools, keeping the existing schema (aliases, risk,
capability flag, slots). New families:

- `arrange.*` (clip ops: copy/duplicate/move/split/delete, insert bar, loop)
- `mixer.*` (channel ops, sends, routing, master)
- `automation.*` (draw curve, sweep filter, fade in/out, tempo ramp, global)
- `controller.*` (LFO wobble, sidechain/ducking, CC map)
- `sound.*` (envelope presets, filter type, arp config, detune)
- `render.*` (export wav/ogg/mp3, stems, preview)
- `note.*` (quantize, humanize, reverse, velocity edit, add chord/arpeggio)
- `pattern.*` (BB steps, accents, per-step velocity)
- `sample.*` (loop/pitch/amp/range)
- `project.*` extends (time signature, metronome, notes, microtuner)

Alias harvesting from the manual (grounded phrasing users actually type):
"four on the floor", "amen break", "make the kick duck under the bass"
(peak-controller sidechain), "filter sweep", "wobble bass" (LFO beat-sync),
"sidechain through mixer", "export stems", "render the loop", "add a lowpass
filter with a slow attack", "humanize the hi-hats", "clone this track".

Confidence/risk: reuse existing gates. Deterministic-first stays; Ollama only
for ambiguous phrasing.

---

## 8. Python stack refocus

1. **Planner vocabulary** — mirror the ~90 tools as actions; planner becomes a
   sequence composer, not a capability limiter. Unknown goal → plan with
   `describe_*` probes first (discovery-driven planning).
2. **DiscoveryIndex** — extend to mixer channels, patterns, controllers,
   automation clips, presets/patch files (`data/presets`, `data/samples`,
   `data/projects`), render formats. Warmup refreshes all.
3. **Playbooks** — rewrite the 9 playbooks from `guide_note` text into real
   tool sequences (the manual's own workflows: first song = preset → piano
   roll → copy segment; first beat = kick/hat/snare → steps → accents;
   mixing = meters → volume → compressor; automation = knob → clip → nodes).
   Add playbooks: "four on the floor", "filter sweep", "sidechain ducking",
   "render stems", "VST setup".
4. **Memory** — keep journal; add preference learning (tempo, key, default
   instrument, mixer layout) and per-goal success/failure telemetry to rank
   playbooks.
5. **Evals** — extend the golden-scenario taxonomy per domain; add
   render-preview assertions (file exists, non-empty, format correct) and
   state-delta assertions per step.

---

## 9. MCP operator server v2

- Expose the ~90 tools 1:1 (currently 39). Group descriptions by domain with
  "when to use" guidance.
- Add **resources**: `lmms://da/state-schema` (envelope + address grammar),
  `lmms://da/manual-workflows` (rendered playbooks), `lmms://da/capabilities`
  (live describe dump).
- Add **prompts**: `lmms-make-a-beat`, `lmms-arrange-song`, `lmms-mix-and-
  master`, `lmms-render-stems`, `lmms-automate-a-sweep` — each a multi-step
  template the client LLM fills in.
- Keep typed tools as the default; `lmms_goal` stays for fuzzy delegation.
- The confirm flow and snapshot flow stay as built.

---

## 10. Phased roadmap

| Phase | Scope | Effort (est.) | Exit criteria |
|---|---|---|---|
| **P0 — Core patches** | §6 accessors; make `seteffectparam` real via `findChildren` | 2-3 d | headers compile; `set_effect_param` works on any effect |
| **P1 — Foundation domains** | Automation (§4.9), Mixer (§4.8), Render (§4.11), transport depth (§4.1), introspection (`describe_*`, `get_param/set_param`) | 5-7 d | LLM can: draw a filter sweep, route a send, render song + stems, seek/loop |
| **P2 — Arrangement & notes** | §4.2 clips, §4.3 notes, §4.4 patterns (incl. step velocity), §4.12 samples | 4-6 d | LLM can: build a 16-bar arrangement, quantize/humanize, make a BB pattern with accents |
| **P3 — Sound design** | §4.5 instruments (presets, VST, Zyn, Sf2), §4.6 shaping, §4.7 effects params, §4.10 controllers | 4-6 d | LLM can: load a soundfont + patch, set ADSR/filter/LFO, add LFO wobble, connect CC |
| **P4 — Stack alignment** | manifest v3 (§7), planner/discovery/playbooks (§8), MCP operator v2 (§9), evals | 3-4 d | e2e: "make a 4-bar house loop with 808, filter sweep, sidechain duck, render stems" runs unguided |
| **P5 — Recording (hard)** | §4.14 MIDI capture into clips, quantize-on-record | 5-10 d (core work) | optional; highest risk, defer until P0-P4 ships |

**Total ≈ 3-5 engineer-weeks** for P0-P4 (the assistant's actual value), P5 is
a separate core-feature project.

### Sequencing rationale

- P1 first because automation + mixer + render are the three capabilities that
  turn "beat helper" into "DAW assistant" — and they are all READY (no core
  patches), so P0 is only needed for sound design, not for the core value.
- P4 (LLM alignment) after the surface exists; rewriting the planner before
  the tools is wasted motion.
- P5 last because it is the only slice requiring real core surgery.

---

## 11. Risks & open questions

| Risk | Mitigation |
|---|---|
| **Audio-thread safety** — any model mutation during playback must go through `requestChangesGuard` (Song.cpp:207 pattern) | all write tools wrap mutations in `Engine::audioEngine()->requestChangesGuard()`; render swaps the audio device (RenderManager already handles store/restore) |
| **Blocking dialogs** — never call `.exec()` (ExportProjectDialog, FileDialog) | plugin implements render/import/save itself via core API; dialogs only for unsupported flows |
| **Deferred project transitions** — open/new/save must be queued on the main window (AgentControl.cpp:2960 precedent) | keep QTimer::singleShot pattern; serialize via the existing `m_projectTransitionQueued` flag |
| **VST automation** — LADSPA/VST params live behind `EffectControls`; VST host params are index-based | generic `set_param` via `findChildren` covers LADSPA/built-ins; VST via `VstPlugin::setParam` index; document limits |
| **Automation value space** — clip nodes are normalized 0..1 (inverseScaledValue/scaledValue) | plugin converts model units ↔ normalized at the boundary; `describe_*` reports both |
| **Naming drift** — checkout renamed AutomationPattern→AutomationClip, BB→PatternStore, BBEditor→PatternEditor, no BBT/ControllerRack | plan already uses current names; keep an alias table in code comments |
| **Upstream resistance to accessor patches** | all patches are pure additions; file as PRs; agent works without them minus sound-design domain |
| **LMMS can't record audio** | out of scope; document loudly in the assistant's capability description |
| **Export correctness on time-sig changes / VSTs** (community-reported bugs) | render uses the same pipeline as the GUI export; surface failures as `warnings` with retry guidance |

---

## 12. Deliverables checklist

- [x] P0: accessor patches (§6) merged into the checkout — InstrumentTrack soundShaping/arpeggio/noteStacking, SampleTrack volume/pan, Song masterVolumeModel/masterPitchModel, EnvelopeAndLfoParameters non-const envelope+LFO getters, InstrumentFunctions arp/stacking getters, LfoController model getters, MidiController midiPort, ProjectNotes text(), MidiClip setClipType+setStepsPerBar, Effect setEnabled/setWetDryLevel
- [x] AgentControl v2: ~150 typed tools, envelope v2 with `hints`, address grammar, `describe_*`, guards + snapshots on all writes — 14 domain translation units (`agent_*.cpp`), dispatch via `dispatchV2Tool`
- [x] `set_effect_param` implemented via `findChildren<AutomatableModel>()` (stub removed)
- [x] Render: `render_song` / `render_tracks` / `render_preview` with progress + completion events (RenderManager-backed)
- [x] command_manifest v3 (~80 intents, manual-harvested aliases, intent→tool mapping) — plugin defaults to v3 with v2 fallback
- [x] planner/discovery/playbooks rewritten on the new surface; 17 playbooks as real tool sequences
- [x] MCP operator v2 mirrors tools (147 total), resources (`lmms://da/*`) + prompts added
- [x] evals: 45 golden scenarios + task suite buckets per domain; render + state-delta assertions
- [x] docs: COMMANDS.md regenerated; manual coverage table updated; this plan updated
- [x] Verification: full LMMS build 1213/1213 with Qt5 toolchain; `npm test` 88/88; `validate_voice_contracts.py` passes; stdio smoke test serves 147 tools
