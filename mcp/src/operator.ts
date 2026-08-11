/**
 * lmms-operator: MCP server that lets another LLM operate a running LMMS
 * through natural language and typed DAW actions.
 *
 * Every tool is a thin proxy over one of two transports:
 *
 * - `LmmsClient` -> the AgentControl tool server compiled into LMMS
 *   (plugins/AgentControl, TCP 127.0.0.1:7777). Typed tools map 1:1 to the
 *   plugin's `dispatchTool` surface; commands go through the plugin's NL
 *   interpreter, including its confirmation gate.
 * - `AgentdClient` -> the persistent agent daemon (lmms-agentd, TCP
 *   127.0.0.1:7781) for `lmms_goal`, which plans a natural-language goal with
 *   the hybrid interpreter (deterministic + heuristic + Ollama) and executes
 *   the resulting steps inside LMMS.
 *
 * Responses carry the plugin envelope, including `state_delta`, so a client
 * LLM can observe what changed after every write.
 */
import { McpServer } from "@modelcontextprotocol/sdk/server/mcp.js";
import type { CallToolResult } from "@modelcontextprotocol/sdk/types.js";
import { z } from "zod";
import { AgentdClient, LmmsClient, type LmmsEnvelope } from "./lmms-client.js";

export interface OperatorDeps {
  client?: LmmsClient;
  agentd?: AgentdClient;
}

const READ_ONLY_ANNOTATIONS = {
  readOnlyHint: true,
  idempotentHint: true,
  destructiveHint: false,
  openWorldHint: false,
};

const DESTRUCTIVE_ANNOTATIONS = {
  readOnlyHint: false,
  idempotentHint: false,
  destructiveHint: true,
  openWorldHint: false,
};

const WRITE_ANNOTATIONS = {
  readOnlyHint: false,
  idempotentHint: false,
  destructiveHint: false,
  openWorldHint: false,
};

/** Track reference accepted everywhere the plugin resolves tracks. */
const TrackRefSchema = z
  .object({
    track_id: z
      .string()
      .regex(/^t_\d+$/)
      .optional()
      .describe("Stable track id from lmms_list_tracks, e.g. t_3."),
    track: z.string().optional().describe("Exact or fuzzy track name (fallback: selected track)."),
    track_name: z.string().optional().describe("Alias of track."),
  })
  .strict();

/** Mixer channel reference: name or index (0 = master). */
const ChannelRefSchema = z.union([
  z.string().min(1).describe("Channel name, e.g. \"Reverb Bus\"."),
  z.number().int().min(0).describe("Channel index, 0 = master."),
]);

/** Shared render options (AgentControl v2 §1.2 units). */
const RenderOptionsSchema = z
  .object({
    format: z.enum(["wav", "flac", "ogg", "mp3"]).optional().describe("Output format."),
    sample_rate: z.number().int().min(44100).max(192000).optional().describe("Sample rate in Hz."),
    bit_depth: z.union([z.literal(16), z.literal(24), z.literal(32)]).optional().describe("Bit depth (32 = float)."),
    bitrate: z.number().int().min(64).max(320).optional().describe("MP3 bitrate in kbps."),
    stereo_mode: z.enum(["mono", "stereo", "joint_stereo"]).optional().describe("Stereo mode."),
  })
  .strict();

/** Chord table shared by add_chord / add_arpeggio (AgentControl v2 §2.3). */
const ChordSchema = z
  .enum(["major", "minor", "7", "maj7", "min7", "dim", "sus2", "sus4", "power"])
  .describe("Chord quality from the built-in table.");

// ---- v2 static resources ---------------------------------------------------

const DA_STATE_SCHEMA_TEXT = `# AgentControl v2 — envelope, units, and model addresses

## Envelope

Every tool returns the plugin envelope as newline-delimited JSON:

    { "ok": true, "result": {}, "state_delta": {},
      "warnings": [], "error_code": null, "error_message": null, "hints": [] }

- ok: true on success; false with error_code/error_message on failure.
- result: tool-specific payload (see each tool's description).
- state_delta: diff of track_count/tempo/selected track plus domain fields after writes.
- warnings: non-fatal notices.
- hints: non-blocking contextual notes ("track did not exist; created 'Lead'",
  "tempo is automated; direct setValue is overridden during playback").
- error_code is stable: unknown_tool | tool_failed | bad_address | bad_args |
  not_implemented | not_available | render_busy | not_found.

Confirmation-gated tools (new/open project, delete track, delete channel,
clear clip, render overwrite, global automation, controller disconnect) return
result.message starting with "Confirmation required:" and needs_confirmation:
true; answer with lmms_confirm (approve/cancel) within LMMS_CONFIRM_WINDOW_MS.

## Units

- Time: ticks (192 per bar at 4/4). Some tools accept {bar, beat, tick}.
- Pitch: MIDI note numbers (60 = C4 concert).
- Note velocity: 1..127.
- Track volume / pan / master volume: 0..1 float.
- Automation clip values: 0..1 normalized (clip space).
- Render: sample rate 44100..192000, bit depth 16|24|32 (32 = float),
  bitrate 64..320 kbps, stereo mode mono|stereo|joint_stereo.

## Model addresses

Automatable models are addressed by string so clients never need C++ knowledge:

    song.tempo                  song.master.volume      song.master.pitch
    track:<name>.<param>        track:Lead.filter.cutoff
    fx:<channel>.<effect>.<param>   fx:2.reverb.wet
    inst:<param>                inst:cutoff             (instrument of selected/last track)

Track param suffixes: volume, pan, pitch, pitch_range, base_note, first_key,
last_key, mixer_channel; filter.enabled/type/cutoff/reso;
env.predelay/attack/hold/decay/sustain/release/amount (and env.<target>.<p>
for cutoff/resonance targets); lfo.amount/speed/wave (lfo.<target>.<p>);
arp.enabled/chord/range/direction/time/gate/mode; ns.enabled/type/range;
midi.cc.<n>. Effect params: fx:<channel>.<effectDisplayName>.<paramDisplayName>;
VST params additionally as fx:<channel>.<vst>.<index>.
`;

const DA_CAPABILITIES_TEXT = `# AgentControl v2 — tool surface

The lmms_* MCP tools proxy these plugin tools (wire names are sent as-is;
the plugin normalizes by lowercasing and stripping non-alphanumerics):

## Project & transport
set_time_signature, set_metronome, set_master_volume, set_master_pitch,
set_play_pos, set_loop, clear_loop, set_stop_behaviour, play_pattern,
play_clip, insert_bar, remove_bar, save_project_as

## Tracks & clips (arrangement)
create_clip, move_clip, resize_clip, split_clip, clone_clip, delete_clip,
set_clip_mute, set_clip_name, set_clip_color, clone_track, delete_track,
move_track, set_track_volume, set_track_pan, set_track_pitch,
set_track_key_range, set_track_base_note

## Notes
edit_notes, remove_notes, clear_clip, quantize_clip, humanize_clip,
reverse_clip, split_clip_notes, set_clip_velocity_scale, add_chord,
add_arpeggio

## Patterns (BB)
create_pattern, select_pattern, clone_pattern, set_steps,
set_step_velocity, set_steps_per_bar, add_rhythm

## Samples
set_sample_loop, set_sample_pitch, set_sample_amp, set_sample_range

## Instruments & sound
load_instrument_preset, set_vst_program, set_vst_param, set_sf2_patch,
describe_instrument, set_envelope, set_filter, set_lfo, set_arp,
set_note_stacking

## Effects
set_effect_param, get_effect_params, move_effect, set_effect_enabled,
set_effect_wetdry

## Mixer
list_mixer_channels, create_channel, delete_channel, rename_channel,
move_channel, set_channel_volume, set_channel_mute, set_channel_solo,
add_channel_effect, remove_channel_effect, create_send, delete_send,
set_send_amount, route_track_to_channel, get_peak_levels

## Automation
create_automation, automate, set_automation_node, remove_automation_node,
set_automation_tension, set_automation_progression, global_automate,
read_automation, list_automation

## Controllers
create_controller, set_lfo_controller, connect_controller,
disconnect_controller, describe_controllers

## Render / export
render_song, render_tracks, render_preview, get_render_progress,
cancel_render, export_midi

## MIDI record
record_arm, record_disarm, record_start, record_stop

## Misc
describe_song, set_project_notes, get_project_notes, set_microtuner

Plus the v1 surface (getprojectstate, listtracks, gettrackdetails,
listpatterns, listinstruments, listeffects, listtoolwindows,
getselectionstate, findtrackbyname, searchprojectaudio, createtrack,
renametrack, loadinstrument, loadsample, createpattern, addnotes, addsteps,
settempo, addeffect, removeeffect, seteffectparam, opentool, importaudio,
importmidi, importhydrogen, selecttrack, mutetrack, solotrack,
createsnapshot, undolastaction, rollbacktosnapshot, diffsincesnapshot).

Clip ids: "global:<address>" for global automation, "auto:<track>:<clip_index>"
for track automation clips. Render is async: render_song returns a render_id,
poll with get_render_progress, abort with cancel_render (one at a time).
`;

const DA_WORKFLOWS_TEXT = `# Manual LMMS workflows (AgentControl v2)

These are the step orders the operator LLM is expected to follow. The same
flows exist as prompts: lmms-make-a-beat, lmms-arrange-song,
lmms-mix-and-master, lmms-render-stems, lmms-automate-a-sweep.

## Make a beat
1. lmms_set_tempo with the target BPM.
2. Create a drum track (lmms_create_track type "instrument") and a bass track.
3. Program drums: lmms_add_rhythm {drum: kick/snare/hihat/crash/ride,
   pattern: [...]} or lmms_set_steps + lmms_set_step_velocity.
4. Write the bassline with lmms_add_notes, harmony with lmms_add_chord,
   motion with lmms_add_arpeggio.
5. Set the loop: lmms_set_loop {begin_tick: 0, end_tick: bars*192*sig}.
6. Preview with lmms_play; stop with lmms_stop.

## Arrange a song
1. lmms_state to see tracks/patterns; lmms_list_patterns for what exists.
2. Place sections with lmms_create_clip {track, tick, name}.
3. Repeat/develop with lmms_clone_clip, lmms_move_clip, lmms_split_clip
   {at_tick}, lmms_resize_clip {new_length}.
4. Structure edits: lmms_insert_bar / lmms_remove_bar {at_tick}.
5. Dynamics: lmms_set_clip_mute for drops, lmms_set_clip_name /
   lmms_set_clip_color to label sections. Audition with lmms_play.

## Mix and master
1. Survey: lmms_list_mixer_channels, lmms_get_peak_levels {channel?}.
2. Balance: lmms_set_track_volume {value 0..1}, lmms_set_track_pan.
3. Process: lmms_add_effect, then lmms_set_effect_param {param, value},
   lmms_set_effect_wetdry for parallel blend, lmms_set_effect_enabled to
   bypass, lmms_move_effect to reorder.
4. Buses: lmms_create_channel {name}, lmms_create_send {from, to, amount},
   lmms_set_send_amount; route tracks with lmms_route_track_to_channel.
5. Glue: lmms_set_master_volume; iterate against lmms_get_peak_levels.

## Render stems
1. Full mix: lmms_render_song {path, format, sample_rate, ...}.
2. Stems: lmms_render_tracks {dir, prefix, format, ...}.
3. Loop preview: lmms_render_preview {format, begin_tick, end_tick}.
4. Poll lmms_get_render_progress {render_id} until done; abort with
   lmms_cancel_render. Optionally lmms_export_midi {path} for the score.
   Note: rendering over an existing file is confirmation-gated.

## Automate a sweep
1. Find the address: lmms_describe_instrument / lmms_describe_song.
2. lmms_create_automation {address, name?} (or reuse via lmms_list_automation).
3. Draw the curve: lmms_automate {address, ticks: [...], values: [...]}.
4. Shape it: lmms_set_automation_progression {discrete|linear|cubic},
   lmms_set_automation_tension {tension -1..1} per node.
5. Micro-edits: lmms_set_automation_node / lmms_remove_automation_node.
6. Song-wide: lmms_global_automate (confirmation-gated).
`;

// ---- v2 prompt definitions ---------------------------------------------------

const makeABeatShape = {
  style: z.string().optional().describe("Style hint, e.g. house, trap, boom bap."),
  tempo: z.number().int().min(40).max(220).optional().describe("Target BPM (default 120)."),
  bars: z.number().int().min(1).max(64).optional().describe("Number of bars (default 4)."),
} satisfies z.ZodRawShape;

const arrangeSongShape = {
  bars: z.number().int().min(1).max(128).optional().describe("Song length in bars (default 32)."),
  sections: z.string().optional().describe("Section plan, e.g. \"intro 8 / drop 16 / outro 8\"."),
} satisfies z.ZodRawShape;

const mixMasterShape = {
  loudness: z.string().optional().describe("Loudness target, e.g. \"-14 LUFS\", \"club loud\"."),
} satisfies z.ZodRawShape;

const renderStemsShape = {
  dir: z.string().optional().describe("Output directory for stems."),
  format: z.enum(["wav", "flac", "ogg", "mp3"]).optional().describe("Stem format (default wav)."),
} satisfies z.ZodRawShape;

const automateSweepShape = {
  address: z.string().optional().describe("Model address, e.g. track:Lead.filter.cutoff."),
} satisfies z.ZodRawShape;

function renderMakeABeat(args: Record<string, unknown>): string {
  const tempo = typeof args.tempo === "number" ? args.tempo : 120;
  const bars = typeof args.bars === "number" ? args.bars : 4;
  const style = typeof args.style === "string" ? args.style : "generic";
  return [
    `Make a ${bars}-bar ${style} beat at ${tempo} BPM.`,
    "",
    "1. lmms_set_tempo { tempo } — the BPM above.",
    "2. lmms_create_track { type: \"instrument\", name: \"Drums\" }; load a drum instrument (lmms_load_instrument) or samples (lmms_load_sample).",
    "3. Program the beat: lmms_add_rhythm { drum: \"kick\" | \"snare\" | \"hihat\" | \"crash\" | \"ride\", pattern: [...] } or lmms_set_steps + lmms_set_step_velocity.",
    "4. lmms_create_track { type: \"instrument\", name: \"Bass\" }; load an instrument and write a bassline with lmms_add_notes (key 36..48 territory).",
    "5. Harmony: lmms_add_chord { root, chord: major|minor|7|maj7|min7|dim|sus2|sus4|power } or lmms_add_arpeggio for motion.",
    "6. lmms_set_loop { begin_tick: 0, end_tick: <bars * 192> } and audition with lmms_play.",
    "",
    "Start by calling lmms_state to confirm the project, then follow the steps in order.",
  ].join("\n");
}

function renderArrangeSong(args: Record<string, unknown>): string {
  const bars = typeof args.bars === "number" ? args.bars : 32;
  const sections = typeof args.sections === "string" ? args.sections : "intro 8 / drop 16 / outro 8";
  return [
    `Arrange the song into a ${bars}-bar structure: ${sections}.`,
    "",
    "1. lmms_state + lmms_list_patterns — inventory tracks and pattern clips.",
    "2. Lay out sections with lmms_create_clip { track, tick, name }.",
    "3. Develop: lmms_clone_clip { track, clip_index, to_tick } for repeats, lmms_move_clip for placement.",
    "4. Vary: lmms_split_clip { at_tick } then lmms_resize_clip { new_length } for edits per section.",
    "5. Structure: lmms_insert_bar { at_tick } / lmms_remove_bar { at_tick } to fix bar count.",
    "6. Dynamics: lmms_set_clip_mute for drops, lmms_set_clip_name / lmms_set_clip_color to label sections.",
    "7. Audition with lmms_play; iterate.",
    "",
    "Verify placement against lmms_state after each pass.",
  ].join("\n");
}

function renderMixMaster(args: Record<string, unknown>): string {
  const loudness = typeof args.loudness === "string" ? args.loudness : "balanced (-14 LUFS-ish)";
  return [
    `Mix and master the project toward a ${loudness} result.`,
    "",
    "1. Survey: lmms_list_mixer_channels and lmms_get_peak_levels { channel? }.",
    "2. Balance: lmms_set_track_volume { track, value 0..1 } and lmms_set_track_pan { value 0..1 } per track.",
    "3. Process: lmms_add_effect { effect } (EQ/comp/reverb), then lmms_set_effect_param { effect, param, value },",
    "   lmms_set_effect_wetdry { value 0..1 } for parallel blend, lmms_set_effect_enabled to bypass, lmms_move_effect to reorder.",
    "4. Buses: lmms_create_channel { name }, lmms_create_send { from, to, amount 0..1 }, lmms_set_send_amount;",
    "   route tracks with lmms_route_track_to_channel { track, channel }.",
    "5. Glue: lmms_set_master_volume; iterate against lmms_get_peak_levels until peaks sit under 0 dB with headroom.",
    "",
    "Do one knob move at a time and re-check lmms_get_peak_levels after each.",
  ].join("\n");
}

function renderRenderStems(args: Record<string, unknown>): string {
  const dir = typeof args.dir === "string" ? args.dir : "./stems";
  const format = typeof args.format === "string" ? args.format : "wav";
  return [
    `Render the project to audio (full mix + per-track stems in ${format}).`,
    "",
    "1. Full mix: lmms_render_song { path: \"mix." + format + "\", format } — poll lmms_get_render_progress until done.",
    "2. Stems: lmms_render_tracks { dir: \"" + dir + "\", prefix: \"stem\", format } — one file per track.",
    "3. Loop preview (optional): lmms_render_preview { format, begin_tick, end_tick }.",
    "4. Abort a stuck render with lmms_cancel_render; exporting MIDI is lmms_export_midi { path }.",
    "",
    "Note: overwriting an existing render file is confirmation-gated — expect needs_confirmation and answer with lmms_confirm.",
  ].join("\n");
}

function renderAutomateSweep(args: Record<string, unknown>): string {
  const address = typeof args.address === "string" ? args.address : "track:<track>.filter.cutoff";
  return [
    `Automate an expressive sweep on ${address}.`,
    "",
    "1. lmms_describe_instrument (or lmms_describe_song) to confirm the address and its value range.",
    "2. lmms_create_automation { address, name? } — or reuse an existing clip from lmms_list_automation.",
    "3. Draw the curve: lmms_automate { address, ticks: [...], values: [...0..1] }.",
    "4. Shape: lmms_set_automation_progression { clip, progression: discrete|linear|cubic } and",
    "   lmms_set_automation_tension { clip, tick?, tension -1..1 } per node.",
    "5. Micro-edit: lmms_set_automation_node { clip, tick, value } / lmms_remove_automation_node { clip, tick }.",
    "6. Song-wide version: lmms_global_automate { address, ticks, values } (confirmation-gated).",
    "7. Audition with lmms_play.",
    "",
    "Values are normalized 0..1 clip space; the plugin converts to the model's scale.",
  ].join("\n");
}

const CommandSchema = z
  .object({
    command: z
      .string()
      .min(1)
      .describe(
        "Natural-language command for LMMS, e.g. \"add 808\", \"play\", \"open slicer and import loop.wav and split into 16\", \"open piano roll\".",
      ),
    file: z.string().optional().describe("Filename appended to the command (Downloads-relative)."),
    path: z.string().optional().describe("Filesystem path appended to the command."),
    plugin: z.string().optional().describe("Plugin name appended to the command."),
    track: z.string().optional().describe("Track name appended as \"to <track>\"."),
  })
  .strict();

function envelopeResult(env: LmmsEnvelope, label?: string): CallToolResult {
  const text = JSON.stringify(env, null, 2);
  return {
    content: [
      {
        type: "text",
        text: label ? `${label}\n${text}` : text,
      },
    ],
    structuredContent: env as unknown as Record<string, unknown>,
    isError: !env.ok,
  };
}

/** The plugin's confirmation gate answers with a plain message string. */
function confirmationPrompt(env: LmmsEnvelope): string | null {
  const message = env.result?.message;
  if (typeof message !== "string" || !message.startsWith("Confirmation required:")) {
    return null;
  }
  return message;
}

function commandResult(env: LmmsEnvelope): CallToolResult {
  const prompt = confirmationPrompt(env);
  if (prompt !== null) {
    const structured = {
      ...(env as unknown as Record<string, unknown>),
      needs_confirmation: true,
      confirmation_hint:
        "LMMS is waiting for approval. Call lmms_confirm with approve=true to execute, or approve=false to cancel. The pending confirmation expires after LMMS_CONFIRM_WINDOW_MS (default 9000 ms).",
    };
    return {
      content: [{ type: "text", text: prompt }],
      structuredContent: structured,
      isError: false,
    };
  }
  return envelopeResult(env);
}

function guard<A>(
  fn: (args: A) => Promise<CallToolResult>,
): (args: A) => Promise<CallToolResult> {
  return async (args: A): Promise<CallToolResult> => {
    try {
      return await fn(args);
    } catch (err) {
      const message = err instanceof Error ? err.message : String(err);
      return {
        content: [{ type: "text", text: `error: ${message}` }],
        isError: true,
      };
    }
  };
}

/**
 * Build the operator MCP server. `deps` lets tests inject fake clients and
 * point at a fake AgentControl server.
 */
export function createOperatorServer(deps: OperatorDeps = {}): McpServer {
  const client = deps.client ?? new LmmsClient();
  const agentd = deps.agentd ?? new AgentdClient();

  const server = new McpServer(
    { name: "lmms-operator", version: "0.2.0" },
    { capabilities: { tools: {}, resources: {}, prompts: {} } },
  );

  // ---- Observe ------------------------------------------------------------

  server.registerTool(
    "lmms_state",
    {
      title: "Current LMMS project state",
      description:
        "Snapshot of the open LMMS project: project file, tempo, track count, selected track, and every track with its type, instruments, and effects. Start here before planning edits.",
      inputSchema: z.object({}).strict(),
      annotations: READ_ONLY_ANNOTATIONS,
    },
    guard(async () => envelopeResult(await client.callTool("getprojectstate"))),
  );

  server.registerTool(
    "lmms_list_tracks",
    {
      title: "List tracks",
      description: "List every track in the open project with type and index.",
      inputSchema: z.object({}).strict(),
      annotations: READ_ONLY_ANNOTATIONS,
    },
    guard(async () => envelopeResult(await client.callTool("listtracks"))),
  );

  server.registerTool(
    "lmms_track_details",
    {
      title: "Track details",
      description: "Details for one track: type, name, index, instrument or sample, effect chain.",
      inputSchema: TrackRefSchema.extend({}).strict(),
      annotations: READ_ONLY_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("gettrackdetails", args))),
  );

  server.registerTool(
    "lmms_list_patterns",
    {
      title: "List patterns",
      description: "List pattern clips across the project.",
      inputSchema: z.object({}).strict(),
      annotations: READ_ONLY_ANNOTATIONS,
    },
    guard(async () => envelopeResult(await client.callTool("listpatterns"))),
  );

  server.registerTool(
    "lmms_list_instruments",
    {
      title: "List instruments",
      description: "List instruments in use on instrument tracks.",
      inputSchema: z.object({}).strict(),
      annotations: READ_ONLY_ANNOTATIONS,
    },
    guard(async () => envelopeResult(await client.callTool("listinstruments"))),
  );

  server.registerTool(
    "lmms_list_effects",
    {
      title: "List effects",
      description: "List the effect chain of one track (default: selected track).",
      inputSchema: TrackRefSchema.extend({}).strict(),
      annotations: READ_ONLY_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("listeffects", args))),
  );

  server.registerTool(
    "lmms_list_tools",
    {
      title: "List tool windows",
      description: "List tool-plugin windows available in this LMMS build (e.g. Agent Control, Tap Tempo).",
      inputSchema: z.object({}).strict(),
      annotations: READ_ONLY_ANNOTATIONS,
    },
    guard(async () => envelopeResult(await client.callTool("listtoolwindows"))),
  );

  server.registerTool(
    "lmms_find_track",
    {
      title: "Find a track",
      description: "Resolve a track name to its id and details, with fuzzy matching.",
      inputSchema: z
        .object({
          query: z.string().min(1).describe("Track name or fragment."),
        })
        .strict(),
      annotations: READ_ONLY_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("findtrackbyname", args))),
  );

  server.registerTool(
    "lmms_search_audio",
    {
      title: "Search project audio",
      description:
        "Search samples available to the project (discovery index) by name fragment, e.g. \"kick\" or \"808\".",
      inputSchema: z
        .object({
          query: z.string().min(1).describe("Sample name fragment."),
        })
        .strict(),
      annotations: READ_ONLY_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("searchprojectaudio", args))),
  );

  // ---- Project & transport (NL commands) ----------------------------------

  server.registerTool(
    "lmms_new_project",
    {
      title: "New project",
      description:
        "Create a new empty project. Discards the current project (confirmation-gated by LMMS: expect a needs_confirmation response, then call lmms_confirm).",
      inputSchema: z.object({}).strict(),
      annotations: DESTRUCTIVE_ANNOTATIONS,
    },
    guard(async () => commandResult(await client.command("new project"))),
  );

  server.registerTool(
    "lmms_open_project",
    {
      title: "Open project",
      description:
        "Open an LMMS project file (.mmp/.mmpz). Replaces the current project (confirmation-gated).",
      inputSchema: z
        .object({
          path: z.string().min(1).describe("Absolute path to the .mmp/.mmpz file."),
        })
        .strict(),
      annotations: DESTRUCTIVE_ANNOTATIONS,
    },
    guard(async (args) =>
      commandResult(await client.command("open project", { path: args.path })),
    ),
  );

  server.registerTool(
    "lmms_save_project",
    {
      title: "Save project",
      description: "Save the current project. Pass path to save as a new file.",
      inputSchema: z
        .object({
          path: z.string().optional().describe("Absolute path to save to (save-as)."),
        })
        .strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) =>
      commandResult(
        args.path
          ? await client.command("save project as", { path: args.path })
          : await client.command("save project"),
      ),
    ),
  );

  server.registerTool(
    "lmms_play",
    {
      title: "Play",
      description: "Start playback from the playhead.",
      inputSchema: z.object({}).strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async () => commandResult(await client.command("play"))),
  );

  server.registerTool(
    "lmms_pause",
    {
      title: "Pause",
      description: "Pause playback.",
      inputSchema: z.object({}).strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async () => commandResult(await client.command("pause"))),
  );

  server.registerTool(
    "lmms_stop",
    {
      title: "Stop",
      description: "Stop playback and return the playhead to the start.",
      inputSchema: z.object({}).strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async () => commandResult(await client.command("stop"))),
  );

  // ---- Tracks --------------------------------------------------------------

  server.registerTool(
    "lmms_set_tempo",
    {
      title: "Set tempo",
      description: "Set the project tempo in BPM.",
      inputSchema: z
        .object({
          tempo: z.number().int().min(20).max(999).describe("Tempo in BPM."),
        })
        .strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("settempo", args))),
  );

  server.registerTool(
    "lmms_open_tool",
    {
      title: "Open tool window",
      description: "Open a tool-plugin window by name (see lmms_list_tools), e.g. \"Agent Control\".",
      inputSchema: z
        .object({
          name: z.string().min(1).describe("Tool window name."),
        })
        .strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("opentool", args))),
  );

  server.registerTool(
    "lmms_create_track",
    {
      title: "Create track",
      description: "Create a new track of type sample, instrument, automation, or pattern.",
      inputSchema: z
        .object({
          type: z
            .enum(["sample", "instrument", "automation", "pattern"])
            .describe("Track type."),
          name: z.string().optional().describe("Track name."),
        })
        .strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("createtrack", args))),
  );

  server.registerTool(
    "lmms_rename_track",
    {
      title: "Rename track",
      description: "Rename a track.",
      inputSchema: TrackRefSchema.extend({
        new_name: z.string().min(1).describe("New track name."),
      }).strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("renametrack", args))),
  );

  server.registerTool(
    "lmms_select_track",
    {
      title: "Select track",
      description: "Make a track the selection target for subsequent tools that omit a track.",
      inputSchema: TrackRefSchema.extend({}).strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("selecttrack", args))),
  );

  server.registerTool(
    "lmms_mute_track",
    {
      title: "Mute/unmute track",
      description: "Mute or unmute a track.",
      inputSchema: TrackRefSchema.extend({
        mute: z.boolean().describe("true to mute, false to unmute."),
      }).strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("mutetrack", args))),
  );

  server.registerTool(
    "lmms_solo_track",
    {
      title: "Solo/unsolo track",
      description: "Solo or unsolo a track.",
      inputSchema: TrackRefSchema.extend({
        solo: z.boolean().describe("true to solo, false to unsolo."),
      }).strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("solotrack", args))),
  );

  // ---- Instruments, samples, imports ---------------------------------------

  server.registerTool(
    "lmms_load_instrument",
    {
      title: "Load instrument",
      description:
        "Load an instrument plugin onto an instrument track (fuzzy names and aliases resolve inside LMMS), e.g. \"tripleoscillator\", \"kicker\", \"slicert\", \"sf2player\". Creates the track when needed.",
      inputSchema: z
        .object({
          plugin: z.string().min(1).describe("Instrument plugin name or alias."),
          name: z.string().optional().describe("Alias of plugin."),
          track: z.string().optional().describe("Target track (default: selected/last instrument track)."),
        })
        .strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("loadinstrument", args))),
  );

  server.registerTool(
    "lmms_load_sample",
    {
      title: "Load sample",
      description: "Load a sample file onto a sample track (default: selected/last track).",
      inputSchema: z
        .object({
          sample_path: z.string().min(1).describe("Absolute path to the audio file."),
          path: z.string().optional().describe("Alias of sample_path."),
          track: z.string().optional().describe("Target track name."),
        })
        .strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("loadsample", args))),
  );

  server.registerTool(
    "lmms_import_audio",
    {
      title: "Import audio",
      description: "Import an audio file into the project as a sample track.",
      inputSchema: z
        .object({
          path: z.string().min(1).describe("Absolute path to the audio file."),
        })
        .strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("importaudio", args))),
  );

  server.registerTool(
    "lmms_import_midi",
    {
      title: "Import MIDI",
      description: "Import a MIDI file into the project.",
      inputSchema: z
        .object({
          path: z.string().min(1).describe("Absolute path to the .mid file."),
        })
        .strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("importmidi", args))),
  );

  server.registerTool(
    "lmms_import_hydrogen",
    {
      title: "Import Hydrogen project",
      description: "Import a Hydrogen drum machine project (.h2song).",
      inputSchema: z
        .object({
          path: z.string().min(1).describe("Absolute path to the .h2song file."),
        })
        .strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("importhydrogen", args))),
  );

  // ---- Patterns -------------------------------------------------------------

  server.registerTool(
    "lmms_create_pattern",
    {
      title: "Create pattern clip",
      description:
        "Create a pattern clip on an instrument track (default: selected/last instrument track).",
      inputSchema: TrackRefSchema.extend({
        tick: z.number().int().min(0).optional().describe("Start tick (default 0)."),
        name: z.string().optional().describe("Clip name."),
      }).strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("createpattern", args))),
  );

  server.registerTool(
    "lmms_add_notes",
    {
      title: "Add notes to pattern",
      description:
        "Add MIDI notes to a pattern clip. Notes are {key (MIDI note number), pos (tick), length (ticks), velocity (1-200)}. Without clip_index, the last clip of the track is used.",
      inputSchema: TrackRefSchema.extend({
        clip_index: z.number().int().min(0).optional().describe("Clip index on the track."),
        notes: z
          .array(
            z
              .object({
                key: z.number().int().min(0).max(127).describe("MIDI note number (60 = middle C)."),
                pos: z.number().int().min(0).optional().describe("Start tick."),
                length: z.number().int().min(1).optional().describe("Length in ticks."),
                velocity: z.number().int().min(1).max(200).optional().describe("Velocity (default 100)."),
              })
              .strict(),
          )
          .min(1)
          .describe("Notes to add."),
      }).strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("addnotes", args))),
  );

  server.registerTool(
    "lmms_add_steps",
    {
      title: "Add steps to pattern",
      description:
        "Enable step positions in a pattern clip (16 steps per bar, 0-15). With clear_existing, the clip's notes are cleared first.",
      inputSchema: TrackRefSchema.extend({
        clip_index: z.number().int().min(0).optional().describe("Clip index on the track."),
        steps: z
          .array(z.number().int().min(0).max(127))
          .min(1)
          .describe("Step positions to enable."),
        clear_existing: z.boolean().optional().describe("Clear existing notes first."),
      }).strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("addsteps", args))),
  );

  // ---- Effects ---------------------------------------------------------------

  server.registerTool(
    "lmms_add_effect",
    {
      title: "Add effect",
      description:
        "Add an effect to a track's chain (fuzzy names and aliases resolve inside LMMS), e.g. \"reverb\", \"delay\", \"compressor\", \"eq\", \"stereoenhancer\", \"amplifier\".",
      inputSchema: z
        .object({
          effect: z.string().min(1).describe("Effect name or alias."),
          track: z.string().optional().describe("Target track (default: selected/last track)."),
        })
        .strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("addeffect", args))),
  );

  server.registerTool(
    "lmms_remove_effect",
    {
      title: "Remove effect",
      description: "Remove an effect from a track's chain.",
      inputSchema: z
        .object({
          effect: z.string().min(1).describe("Effect name."),
          track: z.string().optional().describe("Track whose chain to edit."),
        })
        .strict(),
      annotations: DESTRUCTIVE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("removeeffect", args))),
  );

  // ---- Safety ----------------------------------------------------------------

  server.registerTool(
    "lmms_snapshot",
    {
      title: "Create snapshot",
      description:
        "Snapshot the current project state before a risky sequence. Returns a snapshot_id for lmms_rollback / lmms_diff.",
      inputSchema: z
        .object({
          label: z.string().optional().describe("Snapshot label."),
        })
        .strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("createsnapshot", args))),
  );

  server.registerTool(
    "lmms_rollback",
    {
      title: "Roll back to snapshot",
      description: "Restore the project state captured in a snapshot.",
      inputSchema: z
        .object({
          snapshot_id: z.string().min(1).describe("Snapshot id from lmms_snapshot."),
        })
        .strict(),
      annotations: DESTRUCTIVE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("rollbacktosnapshot", args))),
  );

  server.registerTool(
    "lmms_diff",
    {
      title: "Diff since snapshot",
      description: "Show what changed in the project since a snapshot.",
      inputSchema: z
        .object({
          snapshot_id: z.string().min(1).describe("Snapshot id from lmms_snapshot."),
        })
        .strict(),
      annotations: READ_ONLY_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("diffsincesnapshot", args))),
  );

  server.registerTool(
    "lmms_undo",
    {
      title: "Undo last action",
      description: "Undo the last mutating tool call.",
      inputSchema: z.object({}).strict(),
      annotations: DESTRUCTIVE_ANNOTATIONS,
    },
    guard(async () => envelopeResult(await client.callTool("undolastaction"))),
  );

  // ---- NL surface --------------------------------------------------------------

  server.registerTool(
    "lmms_command",
    {
      title: "Send an NL command to LMMS",
      description:
        "Send any natural-language command to the in-DAW interpreter: \"add 808\", \"add hihat\", \"open piano roll\", \"show mixer\", \"open slicer and import loop.wav and split into 16\", \"focus instrument track\". Confirmation-gated commands (new/open project, low-confidence LLM interpretations) return needs_confirmation instead of executing; answer with lmms_confirm.",
      inputSchema: CommandSchema,
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) =>
      commandResult(
        await client.command(args.command, {
          file: args.file,
          path: args.path,
          plugin: args.plugin,
          track: args.track,
        }),
      ),
    ),
  );

  server.registerTool(
    "lmms_confirm",
    {
      title: "Answer a confirmation",
      description:
        "Approve or cancel the pending confirmation-gated command inside LMMS (e.g. new/open project). The pending confirmation expires after LMMS_CONFIRM_WINDOW_MS (default 9000 ms).",
      inputSchema: z
        .object({
          approve: z.boolean().describe("true = yes, execute; false = cancel."),
        })
        .strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) =>
      envelopeResult(await client.command(args.approve ? "yes" : "cancel")),
    ),
  );

  server.registerTool(
    "lmms_goal",
    {
      title: "Run an NL goal through the agent daemon",
      description:
        "Delegate a full natural-language goal to the hybrid planner (deterministic + heuristic + Ollama) in the lmms-agentd daemon, e.g. \"make a 4-bar house loop with an 808 bassline\". The daemon plans steps, snapshots, executes inside LMMS, and returns per-step results. Requires the daemon on port 7781; the LLM interpreter needs Ollama (LMMS_OLLAMA_URL).",
      inputSchema: z
        .object({
          goal: z.string().min(1).describe("The natural-language goal."),
          project_path: z.string().optional().describe("Project file to load before planning."),
          timeout_class: z
            .enum(["interactive", "standard", "background"])
            .optional()
            .describe("Retry/backoff class (default standard)."),
          idempotency_key: z.string().optional().describe("Replay-safe key for retried goals."),
        })
        .strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => {
      const env = await agentd.runGoal(args.goal, {
        project_path: args.project_path,
        timeout_class: args.timeout_class,
        idempotency_key: args.idempotency_key,
      });
      const text = JSON.stringify(env, null, 2);
      return {
        content: [{ type: "text", text }],
        structuredContent: env as unknown as Record<string, unknown>,
        isError: !env.ok,
      };
    }),
  );

  // =====================================================================
  // v2 — Project & transport depth
  // =====================================================================

  server.registerTool(
    "lmms_set_time_signature",
    {
      title: "Set time signature",
      description: "Set the song meter, e.g. numerator 3, denominator 4 for 3/4.",
      inputSchema: z
        .object({
          numerator: z.number().int().min(1).max(32).describe("Beats per bar."),
          denominator: z.number().int().min(1).max(32).describe("Beat unit (4 = quarter note)."),
        })
        .strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("set_time_signature", args))),
  );

  server.registerTool(
    "lmms_set_metronome",
    {
      title: "Set metronome",
      description: "Enable or disable the metronome click during playback.",
      inputSchema: z
        .object({
          enabled: z.boolean().describe("true to enable the metronome."),
        })
        .strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("set_metronome", args))),
  );

  server.registerTool(
    "lmms_set_master_volume",
    {
      title: "Set master volume",
      description: "Set the master/song volume, 0..1 float.",
      inputSchema: z
        .object({
          value: z.number().min(0).max(1).describe("Master volume, 0..1."),
        })
        .strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("set_master_volume", args))),
  );

  server.registerTool(
    "lmms_set_master_pitch",
    {
      title: "Set master pitch",
      description: "Set the master pitch (song.master.pitch model, semitones).",
      inputSchema: z
        .object({
          value: z.number().describe("Master pitch in semitones."),
        })
        .strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("set_master_pitch", args))),
  );

  server.registerTool(
    "lmms_set_play_pos",
    {
      title: "Set playhead position",
      description:
        "Move the playhead: pass absolute tick, or bar/beat/tick (tick then being the offset within the beat).",
      inputSchema: z
        .object({
          tick: z.number().int().min(0).optional().describe("Absolute tick (192 per bar at 4/4); with bar present, offset within the beat."),
          bar: z.number().int().min(1).optional().describe("Bar number (1-based)."),
          beat: z.number().int().min(1).optional().describe("Beat within the bar (1-based)."),
        })
        .strict()
        .refine((a) => a.tick !== undefined || a.bar !== undefined, {
          message: "provide tick, or bar (with optional beat)",
        }),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("set_play_pos", args))),
  );

  server.registerTool(
    "lmms_set_loop",
    {
      title: "Set loop",
      description: "Enable the timeline loop over [begin_tick, end_tick).",
      inputSchema: z
        .object({
          begin_tick: z.number().int().min(0).describe("Loop start tick."),
          end_tick: z.number().int().min(1).describe("Loop end tick (exclusive)."),
        })
        .strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("set_loop", args))),
  );

  server.registerTool(
    "lmms_clear_loop",
    {
      title: "Clear loop",
      description: "Disable the timeline loop.",
      inputSchema: z.object({}).strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async () => envelopeResult(await client.callTool("clear_loop"))),
  );

  server.registerTool(
    "lmms_set_stop_behaviour",
    {
      title: "Set stop behaviour",
      description: "Where the playhead returns on stop: back_to_zero, back_to_start, or continue.",
      inputSchema: z
        .object({
          mode: z
            .enum(["back_to_zero", "back_to_start", "continue"])
            .describe("Stop behaviour."),
        })
        .strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("set_stop_behaviour", args))),
  );

  server.registerTool(
    "lmms_play_pattern",
    {
      title: "Play pattern",
      description: "Play a single pattern (default: current pattern).",
      inputSchema: z
        .object({
          pattern: z.string().optional().describe("Pattern name."),
        })
        .strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("play_pattern", args))),
  );

  server.registerTool(
    "lmms_play_clip",
    {
      title: "Play clip",
      description: "Play a MIDI clip from the song editor.",
      inputSchema: TrackRefSchema.extend({
        clip_index: z.number().int().min(0).optional().describe("Clip index on the track."),
      }).strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("play_clip", args))),
  );

  server.registerTool(
    "lmms_insert_bar",
    {
      title: "Insert bar",
      description: "Insert one empty bar at a tick position, shifting later content.",
      inputSchema: z
        .object({
          at_tick: z.number().int().min(0).describe("Tick where the bar is inserted."),
        })
        .strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("insert_bar", args))),
  );

  server.registerTool(
    "lmms_remove_bar",
    {
      title: "Remove bar",
      description: "Remove one bar at a tick position, shifting later content earlier.",
      inputSchema: z
        .object({
          at_tick: z.number().int().min(0).describe("Tick of the bar to remove."),
        })
        .strict(),
      annotations: DESTRUCTIVE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("remove_bar", args))),
  );

  server.registerTool(
    "lmms_save_project_as",
    {
      title: "Save project as",
      description: "Save the current project to a new path (.mmp/.mmpz).",
      inputSchema: z
        .object({
          path: z.string().min(1).describe("Absolute path to save to."),
        })
        .strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("save_project_as", args))),
  );

  // =====================================================================
  // v2 — Tracks & clips (arrangement)
  // =====================================================================

  server.registerTool(
    "lmms_create_clip",
    {
      title: "Create clip",
      description: "Create a new clip on a track at a tick position; returns the clip_index.",
      inputSchema: TrackRefSchema.extend({
        tick: z.number().int().min(0).describe("Start tick of the clip."),
        name: z.string().optional().describe("Clip name."),
      }).strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("create_clip", args))),
  );

  server.registerTool(
    "lmms_move_clip",
    {
      title: "Move clip",
      description: "Move a clip to a new tick position.",
      inputSchema: TrackRefSchema.extend({
        clip_index: z.number().int().min(0).describe("Clip index on the track."),
        new_tick: z.number().int().min(0).describe("New start tick."),
      }).strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("move_clip", args))),
  );

  server.registerTool(
    "lmms_resize_clip",
    {
      title: "Resize clip",
      description: "Change a clip's length in ticks (auto-resize off).",
      inputSchema: TrackRefSchema.extend({
        clip_index: z.number().int().min(0).describe("Clip index on the track."),
        new_length: z.number().int().min(1).describe("New length in ticks."),
      }).strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("resize_clip", args))),
  );

  server.registerTool(
    "lmms_split_clip",
    {
      title: "Split clip",
      description: "Split a clip at a tick position into two clips.",
      inputSchema: TrackRefSchema.extend({
        clip_index: z.number().int().min(0).describe("Clip index on the track."),
        at_tick: z.number().int().min(0).describe("Split position in ticks."),
      }).strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("split_clip", args))),
  );

  server.registerTool(
    "lmms_clone_clip",
    {
      title: "Clone clip",
      description: "Duplicate a clip at a target tick (default: end of the source clip).",
      inputSchema: TrackRefSchema.extend({
        clip_index: z.number().int().min(0).describe("Clip index on the track."),
        to_tick: z.number().int().min(0).optional().describe("Start tick of the clone."),
      }).strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("clone_clip", args))),
  );

  server.registerTool(
    "lmms_delete_clip",
    {
      title: "Delete clip",
      description: "Remove a clip from a track.",
      inputSchema: TrackRefSchema.extend({
        clip_index: z.number().int().min(0).describe("Clip index on the track."),
      }).strict(),
      annotations: DESTRUCTIVE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("delete_clip", args))),
  );

  server.registerTool(
    "lmms_set_clip_mute",
    {
      title: "Mute/unmute clip",
      description: "Mute or unmute a clip.",
      inputSchema: TrackRefSchema.extend({
        clip_index: z.number().int().min(0).describe("Clip index on the track."),
        mute: z.boolean().describe("true to mute, false to unmute."),
      }).strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("set_clip_mute", args))),
  );

  server.registerTool(
    "lmms_set_clip_name",
    {
      title: "Set clip name",
      description: "Rename a clip.",
      inputSchema: TrackRefSchema.extend({
        clip_index: z.number().int().min(0).describe("Clip index on the track."),
        name: z.string().min(1).describe("New clip name."),
      }).strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("set_clip_name", args))),
  );

  server.registerTool(
    "lmms_set_clip_color",
    {
      title: "Set clip color",
      description: "Set a clip's color, hex #rrggbb.",
      inputSchema: TrackRefSchema.extend({
        clip_index: z.number().int().min(0).describe("Clip index on the track."),
        color: z
          .string()
          .regex(/^#[0-9a-fA-F]{6}$/, "color must be #rrggbb")
          .describe("Clip color, e.g. #ff6633."),
      }).strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("set_clip_color", args))),
  );

  server.registerTool(
    "lmms_clone_track",
    {
      title: "Clone track",
      description: "Duplicate a track (XML round-trip) with an optional new name.",
      inputSchema: TrackRefSchema.extend({
        name: z.string().optional().describe("Name for the clone (default: source name + copy)."),
      }).strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("clone_track", args))),
  );

  server.registerTool(
    "lmms_delete_track",
    {
      title: "Delete track",
      description:
        "Remove a track from the project. Confirmation-gated by LMMS: expect a needs_confirmation response, then call lmms_confirm.",
      inputSchema: TrackRefSchema.extend({}).strict(),
      annotations: DESTRUCTIVE_ANNOTATIONS,
    },
    guard(async (args) => commandResult(await client.callTool("delete_track", args))),
  );

  server.registerTool(
    "lmms_move_track",
    {
      title: "Move track",
      description: "Move a track to a new position in the track list.",
      inputSchema: TrackRefSchema.extend({
        index: z.number().int().min(0).describe("Target track index."),
      }).strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("move_track", args))),
  );

  server.registerTool(
    "lmms_set_track_volume",
    {
      title: "Set track volume",
      description: "Set a track's volume, 0..1 float.",
      inputSchema: TrackRefSchema.extend({
        value: z.number().min(0).max(1).describe("Track volume, 0..1."),
      }).strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("set_track_volume", args))),
  );

  server.registerTool(
    "lmms_set_track_pan",
    {
      title: "Set track pan",
      description: "Set a track's pan, 0..1 float (0.5 = center).",
      inputSchema: TrackRefSchema.extend({
        value: z.number().min(0).max(1).describe("Track pan, 0..1 (0.5 = center)."),
      }).strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("set_track_pan", args))),
  );

  server.registerTool(
    "lmms_set_track_pitch",
    {
      title: "Set track pitch",
      description: "Set a track's pitch (transpose) in semitones.",
      inputSchema: TrackRefSchema.extend({
        value: z.number().describe("Pitch offset in semitones."),
      }).strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("set_track_pitch", args))),
  );

  server.registerTool(
    "lmms_set_track_key_range",
    {
      title: "Set track key range",
      description: "Restrict a track to a MIDI key range (first_key..last_key).",
      inputSchema: TrackRefSchema.extend({
        first_key: z.number().int().min(0).max(127).describe("Lowest playable MIDI note."),
        last_key: z.number().int().min(0).max(127).describe("Highest playable MIDI note."),
      })
        .strict()
        .refine((a) => a.first_key <= a.last_key, {
          message: "first_key must be <= last_key",
        }),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("set_track_key_range", args))),
  );

  server.registerTool(
    "lmms_set_track_base_note",
    {
      title: "Set track base note",
      description: "Set a track's base note (root for note-stacking/sample mapping).",
      inputSchema: TrackRefSchema.extend({
        key: z.number().int().min(0).max(127).describe("Base MIDI note."),
      }).strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("set_track_base_note", args))),
  );

  // =====================================================================
  // v2 — Notes
  // =====================================================================

  server.registerTool(
    "lmms_edit_notes",
    {
      title: "Edit notes",
      description:
        "Update existing notes matching (key, pos) in a clip; absent fields are left unchanged. Notes: {key, pos, length?, velocity?, pan?}.",
      inputSchema: TrackRefSchema.extend({
        clip_index: z.number().int().min(0).optional().describe("Clip index on the track."),
        notes: z
          .array(
            z
              .object({
                key: z.number().int().min(0).max(127).describe("MIDI note number to match."),
                pos: z.number().int().min(0).describe("Start tick to match."),
                length: z.number().int().min(1).optional().describe("New length in ticks."),
                velocity: z.number().int().min(1).max(127).optional().describe("New velocity, 1..127."),
                pan: z.number().min(-1).max(1).optional().describe("New pan, -1..1."),
              })
              .strict(),
          )
          .min(1)
          .describe("Notes to update."),
      }).strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("edit_notes", args))),
  );

  server.registerTool(
    "lmms_remove_notes",
    {
      title: "Remove notes",
      description: "Remove all notes with the given keys from a clip (optionally only at a pos).",
      inputSchema: TrackRefSchema.extend({
        clip_index: z.number().int().min(0).optional().describe("Clip index on the track."),
        keys: z
          .array(z.number().int().min(0).max(127))
          .min(1)
          .describe("MIDI note numbers to remove."),
        pos: z.number().int().min(0).optional().describe("Only remove notes starting at this tick."),
      }).strict(),
      annotations: DESTRUCTIVE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("remove_notes", args))),
  );

  server.registerTool(
    "lmms_clear_clip",
    {
      title: "Clear clip",
      description:
        "Remove every note from a clip. Confirmation-gated by LMMS: expect a needs_confirmation response, then call lmms_confirm.",
      inputSchema: TrackRefSchema.extend({
        clip_index: z.number().int().min(0).optional().describe("Clip index on the track."),
      }).strict(),
      annotations: DESTRUCTIVE_ANNOTATIONS,
    },
    guard(async (args) => commandResult(await client.callTool("clear_clip", args))),
  );

  server.registerTool(
    "lmms_quantize_clip",
    {
      title: "Quantize clip",
      description: "Snap all notes in a clip to a resolution in ticks per step (1..192; 192 = one bar).",
      inputSchema: TrackRefSchema.extend({
        clip_index: z.number().int().min(0).optional().describe("Clip index on the track."),
        resolution: z
          .number()
          .int()
          .min(1)
          .max(192)
          .describe("Quantization resolution in ticks (e.g. 48 = 1/4 note at 192/bar)."),
      }).strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("quantize_clip", args))),
  );

  server.registerTool(
    "lmms_humanize_clip",
    {
      title: "Humanize clip",
      description: "Jitter note timing and velocity deterministically (seeded); amount 0..1.",
      inputSchema: TrackRefSchema.extend({
        clip_index: z.number().int().min(0).optional().describe("Clip index on the track."),
        amount: z.number().min(0).max(1).describe("Humanize strength, 0..1."),
      }).strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("humanize_clip", args))),
  );

  server.registerTool(
    "lmms_reverse_clip",
    {
      title: "Reverse clip",
      description: "Reverse the order of notes in a clip.",
      inputSchema: TrackRefSchema.extend({
        clip_index: z.number().int().min(0).optional().describe("Clip index on the track."),
      }).strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("reverse_clip", args))),
  );

  server.registerTool(
    "lmms_split_clip_notes",
    {
      title: "Split clip notes",
      description: "Split every note crossing at_tick into two notes.",
      inputSchema: TrackRefSchema.extend({
        clip_index: z.number().int().min(0).optional().describe("Clip index on the track."),
        at_tick: z.number().int().min(0).describe("Split position in ticks."),
      }).strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("split_clip_notes", args))),
  );

  server.registerTool(
    "lmms_set_clip_velocity_scale",
    {
      title: "Scale clip velocities",
      description: "Multiply all note velocities in a clip by a scale 0..2 (clamped to 1..127).",
      inputSchema: TrackRefSchema.extend({
        clip_index: z.number().int().min(0).optional().describe("Clip index on the track."),
        scale: z.number().min(0).max(2).describe("Velocity multiplier, 0..2."),
      }).strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("set_clip_velocity_scale", args))),
  );

  server.registerTool(
    "lmms_add_chord",
    {
      title: "Add chord",
      description:
        "Add a chord (major/minor/7/maj7/min7/dim/sus2/sus4/power) as notes at a position.",
      inputSchema: TrackRefSchema.extend({
        clip_index: z.number().int().min(0).optional().describe("Clip index on the track."),
        root: z.number().int().min(0).max(127).describe("Root MIDI note."),
        chord: ChordSchema,
        pos: z.number().int().min(0).optional().describe("Start tick (default 0)."),
        length: z.number().int().min(1).optional().describe("Note length in ticks."),
        velocity: z.number().int().min(1).max(127).optional().describe("Note velocity, 1..127."),
      }).strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("add_chord", args))),
  );

  server.registerTool(
    "lmms_add_arpeggio",
    {
      title: "Add arpeggio",
      description:
        "Generate an arpeggio over a chord quality: direction up/down/updown/random, steps count, step length, octaves.",
      inputSchema: TrackRefSchema.extend({
        clip_index: z.number().int().min(0).optional().describe("Clip index on the track."),
        root: z.number().int().min(0).max(127).describe("Root MIDI note."),
        chord: ChordSchema,
        direction: z.enum(["up", "down", "updown", "random"]).describe("Arpeggio direction."),
        steps: z.number().int().min(1).max(256).describe("Number of arpeggio steps."),
        pos: z.number().int().min(0).optional().describe("Start tick (default 0)."),
        step_len: z.number().int().min(1).optional().describe("Step length in ticks."),
        velocity: z.number().int().min(1).max(127).optional().describe("Step velocity, 1..127."),
        octaves: z.number().int().min(1).max(5).optional().describe("Octave span (default 1)."),
      }).strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("add_arpeggio", args))),
  );

  // =====================================================================
  // v2 — Patterns (BB)
  // =====================================================================

  server.registerTool(
    "lmms_select_pattern",
    {
      title: "Select pattern",
      description: "Make a pattern the current BB editor pattern.",
      inputSchema: z
        .object({
          pattern: z.string().min(1).describe("Pattern name."),
        })
        .strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("select_pattern", args))),
  );

  server.registerTool(
    "lmms_clone_pattern",
    {
      title: "Clone pattern",
      description: "Duplicate a pattern with its clips under a new name.",
      inputSchema: z
        .object({
          pattern: z.string().min(1).describe("Pattern to clone."),
          name: z.string().optional().describe("Name for the clone."),
        })
        .strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("clone_pattern", args))),
  );

  server.registerTool(
    "lmms_set_steps",
    {
      title: "Set steps",
      description: "Replace the enabled step positions of a clip (superset of add_steps).",
      inputSchema: TrackRefSchema.extend({
        clip_index: z.number().int().min(0).optional().describe("Clip index on the track."),
        steps: z
          .array(z.number().int().min(0).max(127))
          .min(1)
          .describe("Step positions to enable."),
        clear_existing: z.boolean().optional().describe("Clear existing notes first."),
      }).strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("set_steps", args))),
  );

  server.registerTool(
    "lmms_set_step_velocity",
    {
      title: "Set step velocity",
      description: "Set the velocity of one step in a clip, 1..127.",
      inputSchema: TrackRefSchema.extend({
        clip_index: z.number().int().min(0).describe("Clip index on the track."),
        step: z.number().int().min(0).max(127).describe("Step position."),
        velocity: z.number().int().min(1).max(127).describe("Velocity, 1..127."),
      }).strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("set_step_velocity", args))),
  );

  server.registerTool(
    "lmms_set_steps_per_bar",
    {
      title: "Set steps per bar",
      description: "Scale a clip's step resolution (steps per bar).",
      inputSchema: TrackRefSchema.extend({
        steps: z.number().int().min(1).max(64).describe("Steps per bar."),
      }).strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("set_steps_per_bar", args))),
  );

  server.registerTool(
    "lmms_add_rhythm",
    {
      title: "Add rhythm",
      description:
        "Program a typed rhythm preset (kick/snare/hihat/crash/ride) over 16 step slots.",
      inputSchema: z
        .object({
          drum: z.enum(["kick", "snare", "hihat", "crash", "ride"]).describe("Drum voice."),
          pattern: z
            .array(z.number().int().min(0).max(15))
            .min(1)
            .max(16)
            .describe("Step positions (0..15) where the voice hits."),
        })
        .strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("add_rhythm", args))),
  );

  // =====================================================================
  // v2 — Samples
  // =====================================================================

  server.registerTool(
    "lmms_set_sample_loop",
    {
      title: "Set sample loop",
      description: "Configure sample looping: off, on (loop), or pingpong, with optional loop region.",
      inputSchema: TrackRefSchema.extend({
        clip_index: z.number().int().min(0).optional().describe("Clip index on the track."),
        mode: z.enum(["off", "on", "pingpong"]).describe("Loop mode."),
        loop_start: z.number().int().min(0).optional().describe("Loop start in frames."),
        loop_end: z.number().int().min(0).optional().describe("Loop end in frames."),
      }).strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("set_sample_loop", args))),
  );

  server.registerTool(
    "lmms_set_sample_pitch",
    {
      title: "Set sample pitch",
      description: "Transpose a sample by semitones (frequency ratio 2^(s/12)).",
      inputSchema: TrackRefSchema.extend({
        clip_index: z.number().int().min(0).optional().describe("Clip index on the track."),
        semitones: z.number().min(-48).max(48).describe("Pitch shift in semitones."),
      }).strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("set_sample_pitch", args))),
  );

  server.registerTool(
    "lmms_set_sample_amp",
    {
      title: "Set sample amplitude",
      description: "Set a sample's amplification, 0..1.",
      inputSchema: TrackRefSchema.extend({
        clip_index: z.number().int().min(0).optional().describe("Clip index on the track."),
        value: z.number().min(0).max(1).describe("Amplification, 0..1."),
      }).strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("set_sample_amp", args))),
  );

  server.registerTool(
    "lmms_set_sample_range",
    {
      title: "Set sample range",
      description: "Restrict a sample clip to a region [start_frame, start_frame + length_frames).",
      inputSchema: TrackRefSchema.extend({
        clip_index: z.number().int().min(0).optional().describe("Clip index on the track."),
        start_frame: z.number().int().min(0).describe("Region start in frames."),
        length_frames: z.number().int().min(1).describe("Region length in frames."),
      }).strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("set_sample_range", args))),
  );

  // =====================================================================
  // v2 — Instruments & sound shaping
  // =====================================================================

  server.registerTool(
    "lmms_load_instrument_preset",
    {
      title: "Load instrument preset",
      description: "Load an instrument preset file (xiz/sf2/pat/xp/VST) onto a track.",
      inputSchema: TrackRefSchema.extend({
        path: z.string().min(1).describe("Absolute path to the preset file."),
        plugin: z.string().optional().describe("Plugin name to host the preset in."),
      }).strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("load_instrument_preset", args))),
  );

  server.registerTool(
    "lmms_set_vst_program",
    {
      title: "Set VST program",
      description: "Switch a VST instrument to a program by index.",
      inputSchema: TrackRefSchema.extend({
        program: z.number().int().min(0).describe("VST program index."),
      }).strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("set_vst_program", args))),
  );

  server.registerTool(
    "lmms_set_vst_param",
    {
      title: "Set VST parameter",
      description: "Set a VST parameter by index.",
      inputSchema: TrackRefSchema.extend({
        param: z.number().int().min(0).describe("VST parameter index."),
        value: z.number().describe("Parameter value."),
      }).strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("set_vst_param", args))),
  );

  server.registerTool(
    "lmms_set_sf2_patch",
    {
      title: "Set SF2 patch",
      description: "Select a SoundFont patch (optionally by bank).",
      inputSchema: TrackRefSchema.extend({
        bank: z.number().int().min(0).optional().describe("Bank number."),
        patch: z.number().int().min(0).describe("Patch number."),
      }).strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("set_sf2_patch", args))),
  );

  server.registerTool(
    "lmms_describe_instrument",
    {
      title: "Describe instrument",
      description: "Instrument details: plugin name, patch, and the automatable parameter tree with ranges.",
      inputSchema: TrackRefSchema.extend({}).strict(),
      annotations: READ_ONLY_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("describe_instrument", args))),
  );

  server.registerTool(
    "lmms_set_envelope",
    {
      title: "Set envelope",
      description:
        "Set envelope stages (target volume/cutoff/resonance). Values use model units from lmms_describe_instrument; sustain and amount are 0..1.",
      inputSchema: TrackRefSchema.extend({
        target: z.enum(["volume", "cutoff", "resonance"]).optional().describe("Envelope target (default volume)."),
        predelay: z.number().min(0).optional().describe("Predelay in model units (ms)."),
        attack: z.number().min(0).optional().describe("Attack in model units (ms)."),
        hold: z.number().min(0).optional().describe("Hold in model units (ms)."),
        decay: z.number().min(0).optional().describe("Decay in model units (ms)."),
        sustain: z.number().min(0).max(1).optional().describe("Sustain level, 0..1."),
        release: z.number().min(0).optional().describe("Release in model units (ms)."),
        amount: z.number().min(0).max(1).optional().describe("Envelope amount, 0..1."),
      }).strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("set_envelope", args))),
  );

  server.registerTool(
    "lmms_set_filter",
    {
      title: "Set filter",
      description: "Configure the track's filter: enable, type, cutoff, resonance.",
      inputSchema: TrackRefSchema.extend({
        enabled: z.boolean().optional().describe("Enable the filter."),
        type: z
          .enum(["lowpass", "highpass", "bandpass", "notch", "allpass", "moog", "2xlowpass"])
          .optional()
          .describe("Filter type."),
        cutoff: z.number().min(0).optional().describe("Cutoff frequency in model units (Hz)."),
        resonance: z.number().min(0).max(1).optional().describe("Resonance, 0..1."),
      }).strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("set_filter", args))),
  );

  server.registerTool(
    "lmms_set_lfo",
    {
      title: "Set LFO",
      description: "Configure a track's LFO (target volume/cutoff/resonance): amount, speed, wave, delay, attack.",
      inputSchema: TrackRefSchema.extend({
        target: z.enum(["volume", "cutoff", "resonance"]).optional().describe("LFO target (default volume)."),
        amount: z.number().min(0).max(1).optional().describe("LFO amount, 0..1."),
        speed: z.number().min(0).optional().describe("LFO speed in model units (Hz)."),
        wave: z.string().optional().describe("Waveform name (sine/triangle/square/saw/random)."),
        delay: z.number().min(0).optional().describe("Delay in model units (ms)."),
        attack: z.number().min(0).optional().describe("Fade-in in model units (ms)."),
      }).strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("set_lfo", args))),
  );

  server.registerTool(
    "lmms_set_arp",
    {
      title: "Set arpeggio",
      description: "Configure the track's arpeggiator: enable, chord size, range, direction, time, gate, mode.",
      inputSchema: TrackRefSchema.extend({
        enabled: z.boolean().optional().describe("Enable the arpeggiator."),
        chord: z.number().int().min(1).max(12).optional().describe("Chord size (notes per step)."),
        range: z.number().int().min(1).max(9).optional().describe("Octave range."),
        direction: z.enum(["up", "down", "updown", "random"]).optional().describe("Arp direction."),
        time: z.number().min(1).optional().describe("Step time in model units (1/16 notes)."),
        gate: z.number().min(0).max(1).optional().describe("Gate length fraction, 0..1."),
        mode: z.string().optional().describe("Arp mode (e.g. free pattern string)."),
      }).strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("set_arp", args))),
  );

  server.registerTool(
    "lmms_set_note_stacking",
    {
      title: "Set note stacking",
      description: "Configure note stacking (e.g. unison/octave layers): enable, type, range.",
      inputSchema: TrackRefSchema.extend({
        enabled: z.boolean().optional().describe("Enable note stacking."),
        type: z.string().optional().describe("Stacking type (e.g. unison, chord)."),
        range: z.number().int().min(1).optional().describe("Stack range in semitones."),
      }).strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("set_note_stacking", args))),
  );

  // =====================================================================
  // v2 — Effects
  // =====================================================================

  server.registerTool(
    "lmms_set_effect_param",
    {
      title: "Set effect parameter",
      description: "Set one effect parameter by display name; value in the parameter's model units (see lmms_get_effect_params).",
      inputSchema: z
        .object({
          track: z.string().optional().describe("Track whose chain to edit (default: selected/last track)."),
          effect: z.string().min(1).describe("Effect display name."),
          param: z.string().min(1).describe("Parameter display name."),
          value: z.number().describe("Parameter value in model units."),
        })
        .strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("set_effect_param", args))),
  );

  server.registerTool(
    "lmms_get_effect_params",
    {
      title: "Get effect parameters",
      description: "List an effect's parameters: displayName, type, min, max, current value.",
      inputSchema: z
        .object({
          track: z.string().optional().describe("Track whose chain to inspect (default: selected/last track)."),
          effect: z.string().min(1).describe("Effect display name."),
        })
        .strict(),
      annotations: READ_ONLY_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("get_effect_params", args))),
  );

  server.registerTool(
    "lmms_move_effect",
    {
      title: "Move effect",
      description: "Reorder an effect in the chain: up or down.",
      inputSchema: z
        .object({
          track: z.string().optional().describe("Track whose chain to edit (default: selected/last track)."),
          effect: z.string().min(1).describe("Effect display name."),
          direction: z.enum(["up", "down"]).describe("Move direction."),
        })
        .strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("move_effect", args))),
  );

  server.registerTool(
    "lmms_set_effect_enabled",
    {
      title: "Enable/disable effect",
      description: "Bypass or engage an effect in the chain.",
      inputSchema: z
        .object({
          track: z.string().optional().describe("Track whose chain to edit (default: selected/last track)."),
          effect: z.string().min(1).describe("Effect display name."),
          enabled: z.boolean().describe("true to engage, false to bypass."),
        })
        .strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("set_effect_enabled", args))),
  );

  server.registerTool(
    "lmms_set_effect_wetdry",
    {
      title: "Set effect wet/dry",
      description: "Set an effect's wet/dry mix, 0..1.",
      inputSchema: z
        .object({
          track: z.string().optional().describe("Track whose chain to edit (default: selected/last track)."),
          effect: z.string().min(1).describe("Effect display name."),
          value: z.number().min(0).max(1).describe("Wet/dry, 0..1."),
        })
        .strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("set_effect_wetdry", args))),
  );

  // =====================================================================
  // v2 — Mixer
  // =====================================================================

  server.registerTool(
    "lmms_list_mixer_channels",
    {
      title: "List mixer channels",
      description: "List mixer channels with index, name, volume, mute, solo, effects, and sends.",
      inputSchema: z.object({}).strict(),
      annotations: READ_ONLY_ANNOTATIONS,
    },
    guard(async () => envelopeResult(await client.callTool("list_mixer_channels"))),
  );

  server.registerTool(
    "lmms_create_channel",
    {
      title: "Create mixer channel",
      description: "Create a new mixer channel; returns its index.",
      inputSchema: z
        .object({
          name: z.string().optional().describe("Channel name."),
        })
        .strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("create_channel", args))),
  );

  server.registerTool(
    "lmms_delete_channel",
    {
      title: "Delete mixer channel",
      description:
        "Delete a mixer channel. Confirmation-gated by LMMS: expect a needs_confirmation response, then call lmms_confirm.",
      inputSchema: z
        .object({
          channel: ChannelRefSchema.describe("Channel to delete."),
        })
        .strict(),
      annotations: DESTRUCTIVE_ANNOTATIONS,
    },
    guard(async (args) => commandResult(await client.callTool("delete_channel", args))),
  );

  server.registerTool(
    "lmms_rename_channel",
    {
      title: "Rename mixer channel",
      description: "Rename a mixer channel.",
      inputSchema: z
        .object({
          channel: ChannelRefSchema.describe("Channel to rename."),
          name: z.string().min(1).describe("New channel name."),
        })
        .strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("rename_channel", args))),
  );

  server.registerTool(
    "lmms_move_channel",
    {
      title: "Move mixer channel",
      description: "Move a mixer channel left or right in the mixer.",
      inputSchema: z
        .object({
          channel: ChannelRefSchema.describe("Channel to move."),
          direction: z.enum(["left", "right"]).describe("Move direction."),
        })
        .strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("move_channel", args))),
  );

  server.registerTool(
    "lmms_set_channel_volume",
    {
      title: "Set channel volume",
      description: "Set a mixer channel's volume, 0..1.",
      inputSchema: z
        .object({
          channel: ChannelRefSchema.describe("Channel."),
          value: z.number().min(0).max(1).describe("Volume, 0..1."),
        })
        .strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("set_channel_volume", args))),
  );

  server.registerTool(
    "lmms_set_channel_mute",
    {
      title: "Mute/unmute channel",
      description: "Mute or unmute a mixer channel.",
      inputSchema: z
        .object({
          channel: ChannelRefSchema.describe("Channel."),
          mute: z.boolean().describe("true to mute, false to unmute."),
        })
        .strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("set_channel_mute", args))),
  );

  server.registerTool(
    "lmms_set_channel_solo",
    {
      title: "Solo/unsolo channel",
      description: "Solo or unsolo a mixer channel.",
      inputSchema: z
        .object({
          channel: ChannelRefSchema.describe("Channel."),
          solo: z.boolean().describe("true to solo, false to unsolo."),
        })
        .strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("set_channel_solo", args))),
  );

  server.registerTool(
    "lmms_add_channel_effect",
    {
      title: "Add channel effect",
      description: "Append an effect to a mixer channel's chain (fuzzy names resolve inside LMMS).",
      inputSchema: z
        .object({
          channel: ChannelRefSchema.describe("Channel."),
          effect: z.string().min(1).describe("Effect name or alias."),
        })
        .strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("add_channel_effect", args))),
  );

  server.registerTool(
    "lmms_remove_channel_effect",
    {
      title: "Remove channel effect",
      description: "Remove an effect from a mixer channel's chain.",
      inputSchema: z
        .object({
          channel: ChannelRefSchema.describe("Channel."),
          effect: z.string().min(1).describe("Effect name."),
        })
        .strict(),
      annotations: DESTRUCTIVE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("remove_channel_effect", args))),
  );

  server.registerTool(
    "lmms_create_send",
    {
      title: "Create send",
      description: "Create a send from one mixer channel to another with an initial amount 0..1.",
      inputSchema: z
        .object({
          from: ChannelRefSchema.describe("Source channel."),
          to: ChannelRefSchema.describe("Destination channel."),
          amount: z.number().min(0).max(1).optional().describe("Initial send amount, 0..1."),
        })
        .strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("create_send", args))),
  );

  server.registerTool(
    "lmms_delete_send",
    {
      title: "Delete send",
      description: "Remove a send between two mixer channels.",
      inputSchema: z
        .object({
          from: ChannelRefSchema.describe("Source channel."),
          to: ChannelRefSchema.describe("Destination channel."),
        })
        .strict(),
      annotations: DESTRUCTIVE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("delete_send", args))),
  );

  server.registerTool(
    "lmms_set_send_amount",
    {
      title: "Set send amount",
      description: "Set a send's amount, 0..1.",
      inputSchema: z
        .object({
          from: ChannelRefSchema.describe("Source channel."),
          to: ChannelRefSchema.describe("Destination channel."),
          amount: z.number().min(0).max(1).describe("Send amount, 0..1."),
        })
        .strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("set_send_amount", args))),
  );

  server.registerTool(
    "lmms_route_track_to_channel",
    {
      title: "Route track to channel",
      description: "Route a track's output to a mixer channel.",
      inputSchema: TrackRefSchema.extend({
        channel: ChannelRefSchema.describe("Target mixer channel."),
      }).strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("route_track_to_channel", args))),
  );

  server.registerTool(
    "lmms_get_peak_levels",
    {
      title: "Get peak levels",
      description:
        "Read per-channel peak levels (single channel optional). May return not_available if the core patch is absent.",
      inputSchema: z
        .object({
          channel: ChannelRefSchema.optional().describe("Channel to read (default: all)."),
        })
        .strict(),
      annotations: READ_ONLY_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("get_peak_levels", args))),
  );

  // =====================================================================
  // v2 — Automation
  // =====================================================================

  server.registerTool(
    "lmms_create_automation",
    {
      title: "Create automation",
      description:
        "Create an automation track + clip bound to a model address; returns the clip id (auto:<track>:<clip_index>).",
      inputSchema: TrackRefSchema.extend({
        address: z.string().optional().describe("Model address, e.g. track:Lead.filter.cutoff."),
        name: z.string().optional().describe("Automation track name."),
      }).strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("create_automation", args))),
  );

  server.registerTool(
    "lmms_automate",
    {
      title: "Automate (draw curve)",
      description:
        "Write automation values (0..1 clip space) at tick positions on a model's clip; ticks and values must be equal length.",
      inputSchema: z
        .object({
          address: z.string().min(1).describe("Model address, e.g. song.master.volume."),
          ticks: z.array(z.number().int().min(0)).min(1).describe("Tick positions."),
          values: z.array(z.number().min(0).max(1)).min(1).describe("Values, 0..1."),
          clip: z.string().optional().describe("Target clip id (default: the model's clip)."),
        })
        .strict()
        .superRefine((a, ctx) => {
          if (a.ticks.length !== a.values.length) {
            ctx.addIssue({
              code: z.ZodIssueCode.custom,
              message: "ticks and values must have the same length",
            });
          }
        }),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("automate", args))),
  );

  server.registerTool(
    "lmms_set_automation_node",
    {
      title: "Set automation node",
      description: "Write a single automation node at a tick.",
      inputSchema: z
        .object({
          clip: z.string().min(1).describe("Clip id (global:<address> or auto:<track>:<clip_index>)."),
          tick: z.number().int().min(0).describe("Tick position."),
          value: z.number().min(0).max(1).describe("Value, 0..1."),
        })
        .strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("set_automation_node", args))),
  );

  server.registerTool(
    "lmms_remove_automation_node",
    {
      title: "Remove automation node",
      description: "Remove the automation node at a tick.",
      inputSchema: z
        .object({
          clip: z.string().min(1).describe("Clip id."),
          tick: z.number().int().min(0).describe("Tick position."),
        })
        .strict(),
      annotations: DESTRUCTIVE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("remove_automation_node", args))),
  );

  server.registerTool(
    "lmms_set_automation_tension",
    {
      title: "Set automation tension",
      description: "Set the tension of a node (or the whole clip), -1..1.",
      inputSchema: z
        .object({
          clip: z.string().min(1).describe("Clip id."),
          tick: z.number().int().min(0).optional().describe("Node tick (default: all nodes)."),
          tension: z.number().min(-1).max(1).describe("Tension, -1..1."),
        })
        .strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("set_automation_tension", args))),
  );

  server.registerTool(
    "lmms_set_automation_progression",
    {
      title: "Set automation progression",
      description: "Set the interpolation mode of an automation clip: discrete, linear, or cubic.",
      inputSchema: z
        .object({
          clip: z.string().min(1).describe("Clip id."),
          progression: z.enum(["discrete", "linear", "cubic"]).describe("Interpolation mode."),
        })
        .strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("set_automation_progression", args))),
  );

  server.registerTool(
    "lmms_global_automate",
    {
      title: "Global automate",
      description:
        "Write global automation (song-wide clip) for a model address. Confirmation-gated by LMMS: expect a needs_confirmation response, then call lmms_confirm.",
      inputSchema: z
        .object({
          address: z.string().min(1).describe("Model address, e.g. song.master.volume."),
          ticks: z.array(z.number().int().min(0)).min(1).describe("Tick positions."),
          values: z.array(z.number().min(0).max(1)).min(1).describe("Values, 0..1."),
        })
        .strict()
        .superRefine((a, ctx) => {
          if (a.ticks.length !== a.values.length) {
            ctx.addIssue({
              code: z.ZodIssueCode.custom,
              message: "ticks and values must have the same length",
            });
          }
        }),
      annotations: DESTRUCTIVE_ANNOTATIONS,
    },
    guard(async (args) => commandResult(await client.callTool("global_automate", args))),
  );

  server.registerTool(
    "lmms_read_automation",
    {
      title: "Read automation",
      description: "Read automation nodes as [(tick, value)] with tension and progression, by clip id or model address.",
      inputSchema: z
        .object({
          clip: z.string().optional().describe("Clip id."),
          address: z.string().optional().describe("Model address (alternative to clip)."),
        })
        .strict()
        .refine((a) => a.clip !== undefined || a.address !== undefined, {
          message: "provide clip or address",
        }),
      annotations: READ_ONLY_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("read_automation", args))),
  );

  server.registerTool(
    "lmms_list_automation",
    {
      title: "List automation",
      description: "List automation clips with id, track, and bound model address.",
      inputSchema: z.object({}).strict(),
      annotations: READ_ONLY_ANNOTATIONS,
    },
    guard(async () => envelopeResult(await client.callTool("list_automation"))),
  );

  // =====================================================================
  // v2 — Controllers
  // =====================================================================

  server.registerTool(
    "lmms_create_controller",
    {
      title: "Create controller",
      description: "Create a controller of type lfo, midi, or peak; returns the controller id.",
      inputSchema: z
        .object({
          type: z.enum(["lfo", "midi", "peak"]).describe("Controller type."),
          name: z.string().optional().describe("Controller name."),
        })
        .strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("create_controller", args))),
  );

  server.registerTool(
    "lmms_set_lfo_controller",
    {
      title: "Set LFO controller",
      description: "Configure an LFO controller's wave, speed, amount, base, phase, multiplier.",
      inputSchema: z
        .object({
          controller: z.string().min(1).describe("Controller id."),
          wave: z.string().optional().describe("Waveform name (sine/triangle/square/saw/random)."),
          speed: z.number().min(0).optional().describe("Speed in model units (Hz)."),
          amount: z.number().min(0).max(1).optional().describe("Amount, 0..1."),
          base: z.number().min(0).max(1).optional().describe("Base level, 0..1."),
          phase: z.number().min(0).optional().describe("Phase offset in model units."),
          multiplier: z.number().optional().describe("Multiplier."),
        })
        .strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("set_lfo_controller", args))),
  );

  server.registerTool(
    "lmms_connect_controller",
    {
      title: "Connect controller",
      description: "Connect a controller to a model address (e.g. sidechain ducking).",
      inputSchema: z
        .object({
          controller: z.string().min(1).describe("Controller id."),
          address: z.string().min(1).describe("Model address, e.g. track:Bass.volume."),
        })
        .strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("connect_controller", args))),
  );

  server.registerTool(
    "lmms_disconnect_controller",
    {
      title: "Disconnect controller",
      description:
        "Remove a controller connection from a model address. Confirmation-gated by LMMS: expect a needs_confirmation response, then call lmms_confirm.",
      inputSchema: z
        .object({
          address: z.string().min(1).describe("Model address to disconnect."),
        })
        .strict(),
      annotations: DESTRUCTIVE_ANNOTATIONS,
    },
    guard(async (args) => commandResult(await client.callTool("disconnect_controller", args))),
  );

  server.registerTool(
    "lmms_describe_controllers",
    {
      title: "Describe controllers",
      description: "List controllers with id, type, params, and connected addresses.",
      inputSchema: z.object({}).strict(),
      annotations: READ_ONLY_ANNOTATIONS,
    },
    guard(async () => envelopeResult(await client.callTool("describe_controllers"))),
  );

  // =====================================================================
  // v2 — Render / export
  // =====================================================================

  server.registerTool(
    "lmms_render_song",
    {
      title: "Render song",
      description:
        "Render the whole song to an audio file asynchronously; returns a render_id to poll with lmms_get_render_progress. Overwriting an existing file is confirmation-gated.",
      inputSchema: RenderOptionsSchema.extend({
        path: z.string().min(1).describe("Absolute output path."),
      }).strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => commandResult(await client.callTool("render_song", args))),
  );

  server.registerTool(
    "lmms_render_tracks",
    {
      title: "Render tracks (stems)",
      description: "Render each track to its own audio file in a directory (stems), asynchronously.",
      inputSchema: RenderOptionsSchema.extend({
        dir: z.string().min(1).describe("Output directory."),
        prefix: z.string().optional().describe("Filename prefix."),
      }).strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("render_tracks", args))),
  );

  server.registerTool(
    "lmms_render_preview",
    {
      title: "Render preview",
      description: "Render a range (default: loop points or whole song) to a file, asynchronously.",
      inputSchema: z
        .object({
          path: z.string().optional().describe("Absolute output path."),
          format: z.enum(["wav", "flac", "ogg", "mp3"]).optional().describe("Output format."),
          begin_tick: z.number().int().min(0).optional().describe("Range start tick."),
          end_tick: z.number().int().min(1).optional().describe("Range end tick (exclusive)."),
        })
        .strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("render_preview", args))),
  );

  server.registerTool(
    "lmms_get_render_progress",
    {
      title: "Get render progress",
      description: "Poll an async render: progress 0..1, done flag, and output path.",
      inputSchema: z
        .object({
          render_id: z.string().optional().describe("Render id (default: most recent)."),
        })
        .strict(),
      annotations: READ_ONLY_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("get_render_progress", args))),
  );

  server.registerTool(
    "lmms_cancel_render",
    {
      title: "Cancel render",
      description: "Abort the active render.",
      inputSchema: z
        .object({
          render_id: z.string().optional().describe("Render id (default: most recent)."),
        })
        .strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("cancel_render", args))),
  );

  server.registerTool(
    "lmms_export_midi",
    {
      title: "Export MIDI",
      description: "Export the song as a standard MIDI file.",
      inputSchema: z
        .object({
          path: z.string().min(1).describe("Absolute path to the .mid file."),
        })
        .strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("export_midi", args))),
  );

  // =====================================================================
  // v2 — MIDI record
  // =====================================================================

  server.registerTool(
    "lmms_record_arm",
    {
      title: "Arm MIDI record",
      description: "Arm a clip for MIDI capture (target: selected/last instrument clip).",
      inputSchema: TrackRefSchema.extend({
        clip_index: z.number().int().min(0).optional().describe("Clip index on the track."),
      }).strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("record_arm", args))),
  );

  server.registerTool(
    "lmms_record_disarm",
    {
      title: "Disarm MIDI record",
      description: "Drop the MIDI capture subscription.",
      inputSchema: z.object({}).strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async () => envelopeResult(await client.callTool("record_disarm"))),
  );

  server.registerTool(
    "lmms_record_start",
    {
      title: "Start recording",
      description: "Start playback and MIDI capture into the armed clip.",
      inputSchema: z.object({}).strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async () => envelopeResult(await client.callTool("record_start"))),
  );

  server.registerTool(
    "lmms_record_stop",
    {
      title: "Stop recording",
      description: "Stop the transport and commit captured notes into the armed clip, optionally quantized.",
      inputSchema: z
        .object({
          quantize: z
            .number()
            .int()
            .min(1)
            .max(192)
            .optional()
            .describe("Quantize resolution in ticks (1/192 = free)."),
        })
        .strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("record_stop", args))),
  );

  // =====================================================================
  // v2 — Misc
  // =====================================================================

  server.registerTool(
    "lmms_describe_song",
    {
      title: "Describe song",
      description:
        "Full project snapshot: tempo, time signature, tracks + clips, mixer, patterns, automation, controllers, project file, notes, microtuner.",
      inputSchema: z.object({}).strict(),
      annotations: READ_ONLY_ANNOTATIONS,
    },
    guard(async () => envelopeResult(await client.callTool("describe_song"))),
  );

  server.registerTool(
    "lmms_set_project_notes",
    {
      title: "Set project notes",
      description: "Write the project notes text.",
      inputSchema: z
        .object({
          text: z.string().min(1).describe("Project notes content."),
        })
        .strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("set_project_notes", args))),
  );

  server.registerTool(
    "lmms_get_project_notes",
    {
      title: "Get project notes",
      description: "Read the project notes text.",
      inputSchema: z.object({}).strict(),
      annotations: READ_ONLY_ANNOTATIONS,
    },
    guard(async () => envelopeResult(await client.callTool("get_project_notes"))),
  );

  server.registerTool(
    "lmms_set_microtuner",
    {
      title: "Set microtuner",
      description: "Configure the microtuner: enable, scale, keymap.",
      inputSchema: z
        .object({
          enabled: z.boolean().optional().describe("Enable microtuning."),
          scale: z.string().optional().describe("Scale name or pattern."),
          keymap: z.string().optional().describe("Keymap name or pattern."),
        })
        .strict(),
      annotations: WRITE_ANNOTATIONS,
    },
    guard(async (args) => envelopeResult(await client.callTool("set_microtuner", args))),
  );

  // =====================================================================
  // v2 — Resources
  // =====================================================================

  server.registerResource(
    "da-state-schema",
    "lmms://da/state-schema",
    {
      description:
        "AgentControl v2 envelope and state grammar: envelope fields, state_delta, units, and the model-address language (track:..., fx:..., inst:...).",
    },
    async (uri) => ({ contents: [{ uri: uri.href, text: DA_STATE_SCHEMA_TEXT }] }),
  );

  server.registerResource(
    "da-capabilities",
    "lmms://da/capabilities",
    {
      description:
        "Full AgentControl v2 tool surface grouped by family, with the plugin wire names the lmms_* tools proxy.",
    },
    async (uri) => ({ contents: [{ uri: uri.href, text: DA_CAPABILITIES_TEXT }] }),
  );

  server.registerResource(
    "da-workflows",
    "lmms://da/workflows",
    {
      description:
        "Manual multi-step LMMS workflows: make a beat, arrange a song, mix and master, render stems, automate a sweep.",
    },
    async (uri) => ({ contents: [{ uri: uri.href, text: DA_WORKFLOWS_TEXT }] }),
  );

  // =====================================================================
  // v2 — Prompts
  // =====================================================================

  server.registerPrompt(
    "lmms-make-a-beat",
    {
      description: "Build a beat from scratch: tempo, drum pattern, bassline, chords, loop, preview.",
      argsSchema: makeABeatShape,
    },
    (args) => ({
      messages: [
        {
          role: "user" as const,
          content: {
            type: "text" as const,
            text: renderMakeABeat((args ?? {}) as Record<string, unknown>),
          },
        },
      ],
    }),
  );

  server.registerPrompt(
    "lmms-arrange-song",
    {
      description: "Arrange clips into a song structure: sections, repeats, splits, mutes.",
      argsSchema: arrangeSongShape,
    },
    (args) => ({
      messages: [
        {
          role: "user" as const,
          content: {
            type: "text" as const,
            text: renderArrangeSong((args ?? {}) as Record<string, unknown>),
          },
        },
      ],
    }),
  );

  server.registerPrompt(
    "lmms-mix-and-master",
    {
      description: "Mix and master the project: balance, effects, buses/sends, master level.",
      argsSchema: mixMasterShape,
    },
    (args) => ({
      messages: [
        {
          role: "user" as const,
          content: {
            type: "text" as const,
            text: renderMixMaster((args ?? {}) as Record<string, unknown>),
          },
        },
      ],
    }),
  );

  server.registerPrompt(
    "lmms-render-stems",
    {
      description: "Render the full mix and per-track stems to audio files.",
      argsSchema: renderStemsShape,
    },
    (args) => ({
      messages: [
        {
          role: "user" as const,
          content: {
            type: "text" as const,
            text: renderRenderStems((args ?? {}) as Record<string, unknown>),
          },
        },
      ],
    }),
  );

  server.registerPrompt(
    "lmms-automate-a-sweep",
    {
      description: "Automate a filter/volume sweep: create automation, draw the curve, shape it, audition.",
      argsSchema: automateSweepShape,
    },
    (args) => ({
      messages: [
        {
          role: "user" as const,
          content: {
            type: "text" as const,
            text: renderAutomateSweep((args ?? {}) as Record<string, unknown>),
          },
        },
      ],
    }),
  );

  return server;
}
