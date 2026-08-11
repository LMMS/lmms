# lmms-mcp

A Model Context Protocol server for the [LMMS](https://github.com/lmms/lmms)
codebase. It gives coding agents safe, read-only access to the checkout it
lives in: source search, symbol lookup, build-system facts, the plugin
catalog, GitHub issues/PRs, and curated resources and prompts for common
LMMS workflows.

Everything is read-only: no tool mutates the repository, the filesystem, or
GitHub. Paths are confined to the repository root (traversal and symlink
escapes are rejected).

## Features

### Tools

| Tool | Purpose |
| --- | --- |
| `lmms_search` | Regex search (ripgrep syntax) across the checkout, excluding build artifacts and `node_modules`; falls back to a bounded walker when ripgrep is absent |
| `lmms_read_file` | Read any repo file with an optional 1-based line range; caps whole-file reads at 4000 lines, refuses binary files |
| `lmms_list_directory` | List a repo directory (directories first, `.git` hidden) |
| `lmms_symbol_lookup` | Find C++ class/struct/enum/namespace declarations, `Name::` member definitions, and references |
| `lmms_build_info` | CMake minimum version, project name, and every `option(...)` switch parsed from the build files |
| `lmms_plugin_catalog` | Every plugin in `plugins/` classified as instrument/effect/tool/unknown, with sources and class declarations |
| `lmms_issue_lookup` | GitHub issues/PRs for `lmms/lmms` (or `LMMS_GITHUB_REPO`); no auth needed for public repos |

### Resources

`lmms://overview`, `lmms://build`, `lmms://architecture`,
`lmms://coding-conventions`, `lmms://plugins`, `lmms://documentation`,
`lmms://agents` — all generated from the live checkout at read time.

### Prompts

`lmms-new-plugin`, `lmms-build-setup`, `lmms-bug-investigation`,
`lmms-code-review`.

## Operator server: drive LMMS itself

`lmms-operator` is the opposite side of the same coin: an MCP server that lets
**another LLM operate a running LMMS** — the "Adobe-generative-tools but for a
DAW" pattern. It speaks the AgentControl protocol embedded in LMMS
(`plugins/AgentControl`, TCP `127.0.0.1:7777`) plus the agent daemon
(`lmmsagent/lmms-agentd`, TCP `127.0.0.1:7781`).

Everything is a thin proxy over those transports, so the LLM client gets the
plugin's typed envelope back — including `state_delta` after every write:

```json
{
  "ok": true,
  "result": { "tempo": 128 },
  "state_delta": { "tempo_before": 120, "tempo_after": 128 },
  "warnings": [],
  "error_code": null,
  "error_message": null
}
```

| Tool | What it does |
| --- | --- |
| `lmms_state`, `lmms_list_tracks`, `lmms_track_details`, `lmms_list_patterns`, `lmms_list_instruments`, `lmms_list_effects`, `lmms_list_tools`, `lmms_find_track`, `lmms_search_audio` | Observe the project before acting |
| `lmms_new_project`, `lmms_open_project`, `lmms_save_project`, `lmms_save_project_as` | Project lifecycle (new/open are confirmation-gated inside LMMS) |
| `lmms_play`, `lmms_pause`, `lmms_stop`, `lmms_play_pattern`, `lmms_play_clip` | Transport, including pattern/clip audition |
| `lmms_set_tempo`, `lmms_set_time_signature`, `lmms_set_metronome`, `lmms_set_master_volume`, `lmms_set_master_pitch`, `lmms_set_play_pos`, `lmms_set_loop`, `lmms_clear_loop`, `lmms_set_stop_behaviour`, `lmms_insert_bar`, `lmms_remove_bar` | Project & transport depth: meter, master level/pitch, playhead, loop, stop behaviour, bar edits |
| `lmms_create_track`, `lmms_rename_track`, `lmms_select_track`, `lmms_mute_track`, `lmms_solo_track`, `lmms_clone_track`, `lmms_delete_track`, `lmms_move_track` | Track control (delete is confirmation-gated) |
| `lmms_set_track_volume`, `lmms_set_track_pan`, `lmms_set_track_pitch`, `lmms_set_track_key_range`, `lmms_set_track_base_note` | Track level/pan/pitch/key-range models |
| `lmms_create_clip`, `lmms_move_clip`, `lmms_resize_clip`, `lmms_split_clip`, `lmms_clone_clip`, `lmms_delete_clip`, `lmms_set_clip_mute`, `lmms_set_clip_name`, `lmms_set_clip_color` | Arrangement: build and edit song-editor clips |
| `lmms_load_instrument`, `lmms_load_sample`, `lmms_import_audio`, `lmms_import_midi`, `lmms_import_hydrogen` | Instruments, samples, imports (fuzzy names/aliases resolve in the plugin) |
| `lmms_load_instrument_preset`, `lmms_set_vst_program`, `lmms_set_vst_param`, `lmms_set_sf2_patch`, `lmms_describe_instrument` | Presets, VST/SF2 control, instrument introspection |
| `lmms_set_envelope`, `lmms_set_filter`, `lmms_set_lfo`, `lmms_set_arp`, `lmms_set_note_stacking` | Sound shaping (volume/cutoff/resonance targets, model units) |
| `lmms_create_pattern`, `lmms_select_pattern`, `lmms_clone_pattern`, `lmms_add_notes`, `lmms_add_steps`, `lmms_set_steps`, `lmms_set_step_velocity`, `lmms_set_steps_per_bar`, `lmms_add_rhythm` | Patterns: notes as `{key,pos,length,velocity}`, steps, typed drum rhythms |
| `lmms_edit_notes`, `lmms_remove_notes`, `lmms_clear_clip`, `lmms_quantize_clip`, `lmms_humanize_clip`, `lmms_reverse_clip`, `lmms_split_clip_notes`, `lmms_set_clip_velocity_scale`, `lmms_add_chord`, `lmms_add_arpeggio` | Note editing: targeted updates, quantization, humanize, chord/arpeggio generators (clear is confirmation-gated) |
| `lmms_set_sample_loop`, `lmms_set_sample_pitch`, `lmms_set_sample_amp`, `lmms_set_sample_range` | Sample playback: loop modes, transpose, amplitude, region |
| `lmms_add_effect`, `lmms_remove_effect`, `lmms_set_effect_param`, `lmms_get_effect_params`, `lmms_move_effect`, `lmms_set_effect_enabled`, `lmms_set_effect_wetdry`, `lmms_open_tool` | Effect chains and tool windows |
| `lmms_list_mixer_channels`, `lmms_create_channel`, `lmms_delete_channel`, `lmms_rename_channel`, `lmms_move_channel`, `lmms_set_channel_volume`, `lmms_set_channel_mute`, `lmms_set_channel_solo`, `lmms_add_channel_effect`, `lmms_remove_channel_effect`, `lmms_create_send`, `lmms_delete_send`, `lmms_set_send_amount`, `lmms_route_track_to_channel`, `lmms_get_peak_levels` | Mixer: channels, sends, routing, peak metering (delete is confirmation-gated) |
| `lmms_create_automation`, `lmms_automate`, `lmms_set_automation_node`, `lmms_remove_automation_node`, `lmms_set_automation_tension`, `lmms_set_automation_progression`, `lmms_global_automate`, `lmms_read_automation`, `lmms_list_automation` | Automation: draw curves on model addresses, per-node edits, global automation (confirm-gated) |
| `lmms_create_controller`, `lmms_set_lfo_controller`, `lmms_connect_controller`, `lmms_disconnect_controller`, `lmms_describe_controllers` | Controller rack: LFO/MIDI/peak controllers and connections (disconnect is confirm-gated) |
| `lmms_render_song`, `lmms_render_tracks`, `lmms_render_preview`, `lmms_get_render_progress`, `lmms_cancel_render`, `lmms_export_midi` | Async render/export: full song, stems, loop preview, progress polling (overwrite is confirm-gated) |
| `lmms_record_arm`, `lmms_record_disarm`, `lmms_record_start`, `lmms_record_stop` | MIDI record: arm a clip, capture live input, quantize on stop |
| `lmms_describe_song`, `lmms_set_project_notes`, `lmms_get_project_notes`, `lmms_set_microtuner` | Project-wide introspection, notes, microtuner |
| `lmms_snapshot`, `lmms_rollback`, `lmms_diff`, `lmms_undo` | Safety: snapshot before a risky sequence, diff or roll back |
| `lmms_command` | Any NL command through the in-DAW interpreter ("add 808", "open slicer and import loop.wav and split into 16") |
| `lmms_confirm` | Answers the plugin's confirmation gate (approve/cancel) |
| `lmms_goal` | Delegates a full NL goal ("make a 4-bar house loop with an 808 bassline") to the hybrid planner in `lmms-agentd` (deterministic + heuristic + Ollama), which plans, snapshots, executes, and reports per-step results |

Time is in ticks (192 per bar at 4/4); pitch is MIDI note numbers (60 = C4);
note velocity 1..127; track/pan/master volumes 0..1; automation values 0..1
normalized clip space. Automatable models use stable string addresses
(`song.tempo`, `track:<name>.<param>`, `fx:<channel>.<effect>.<param>`,
`inst:<param>`) documented by `lmms://da/state-schema`.

### Operator resources

`lmms://da/state-schema` (envelope/address grammar and units), `lmms://da/capabilities`
(the full v2 tool surface), `lmms://da/workflows` (manual multi-step workflows).

### Operator prompts

`lmms-make-a-beat`, `lmms-arrange-song`, `lmms-mix-and-master`,
`lmms-render-stems`, `lmms-automate-a-sweep` — multi-step templates that
orchestrate the v2 tool names above.

### Running the operator server

```sh
cd mcp
npm ci
npm run build
npm run start:operator          # stdio, like any MCP server
```

Environment:

| Variable | Default | Purpose |
| --- | --- | --- |
| `LMMS_AGENT_HOST` / `LMMS_AGENT_PORT` | `127.0.0.1:7777` | AgentControl tool server inside LMMS |
| `LMMS_AGENTD_HOST` / `LMMS_AGENTD_PORT` | `127.0.0.1:7781` | Agent daemon used by `lmms_goal` |

Client config:

```json
{
  "mcpServers": {
    "lmms-operator": {
      "command": "node",
      "args": ["/path/to/lmms/mcp/dist/operator-index.js"]
    }
  }
}
```

Prerequisites for real operation: LMMS built with the AgentControl plugin and
running (it serves typed tools on port 7777); `lmms-agentd` running for
`lmms_goal`, plus Ollama (`LMMS_OLLAMA_URL`) for the LLM interpreter path.
Confirmation-gated commands (`new project`, `open project`, low-confidence
LLM interpretations) return `needs_confirmation: true` instead of executing;
answer with `lmms_confirm` within `LMMS_CONFIRM_WINDOW_MS` (default 9 s).

## Requirements

- Node.js >= 22
- `rg` (ripgrep) is optional but recommended for fast search; the server
  falls back to a built-in walker when it is missing.

## Install and run

```sh
cd mcp
npm ci
npm run build
```

Run the server over stdio:

```sh
node mcp/dist/index.js
```

The server locates the repository root by walking up from its own location or
the current directory. To point it at a specific checkout:

```sh
LMMS_REPO=/path/to/lmms node mcp/dist/index.js
```

## MCP client configuration

Generic clients (Claude Desktop, etc.):

```json
{
  "mcpServers": {
    "lmms": {
      "command": "node",
      "args": ["/path/to/lmms/mcp/dist/index.js"],
      "env": { "LMMS_REPO": "/path/to/lmms" }
    }
  }
}
```

Claude Code:

```sh
claude mcp add lmms -- node /path/to/lmms/mcp/dist/index.js
```

## Environment variables

| Variable | Default | Purpose |
| --- | --- | --- |
| `LMMS_REPO` | auto-detected | Path to the LMMS checkout the server operates on |
| `LMMS_GITHUB_REPO` | `lmms/lmms` | GitHub repo used by `lmms_issue_lookup` |
| `LMMS_GITHUB_TOKEN` | unset | Optional token to raise the GitHub rate limit or read private repos |

## Development

```sh
npm test        # vitest suite (fixture repo + protocol-level tests)
npm run build   # type-check and emit dist/
```

The test suite exercises the server over an in-memory MCP transport against a
small fixture checkout, plus unit tests for path safety, search fallback,
CMake parsing, plugin classification, and GitHub mapping.
