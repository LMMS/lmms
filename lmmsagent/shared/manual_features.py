from __future__ import annotations

from dataclasses import dataclass, field
from typing import List


@dataclass
class FeatureArea:
    id: str
    title: str
    manual_refs: List[str]
    keywords: List[str]
    automated_now: List[str] = field(default_factory=list)
    guided_now: List[str] = field(default_factory=list)
    deferred: List[str] = field(default_factory=list)
    example_prompts: List[str] = field(default_factory=list)


FEATURE_AREAS: List[FeatureArea] = [
    FeatureArea(
        id="main_window_navigation",
        title="Main Window and global navigation",
        manual_refs=[
            "LMMS Main Window",
            "Main Menu Bar",
            "Tool Bar",
            "The Side Bar",
        ],
        keywords=["main window", "toolbar", "side bar", "browser", "projects tab", "samples tab"],
        automated_now=[
            "open_tool (song editor, mixer, piano roll, automation editor, controller rack, project notes)",
            "list_tool_windows",
            "get_project_state",
        ],
        guided_now=[
            "drag-drop operations in sidebar",
            "window docking/arrangement",
        ],
        deferred=["typed API for browser tab selection and drag-drop"],
        example_prompts=["open mixer", "show song editor", "what windows are open"],
    ),
    FeatureArea(
        id="song_editor_tracks",
        title="Song Editor, track arrangement, and clips",
        manual_refs=[
            "The Song Editor window",
            "Working with tracks in Song Editor",
            "Context menus in Song Editor",
        ],
        keywords=["song editor", "track", "arrangement", "pattern track", "sample track", "automation track", "clip"],
        automated_now=[
            "create_track",
            "rename_track",
            "select_track",
            "mute_track",
            "solo_track",
            "clone_track",
            "delete_track (confirm)",
            "move_track",
            "set_track_volume",
            "set_track_pan",
            "set_track_pitch",
            "set_track_key_range",
            "set_track_base_note",
            "create_clip",
            "move_clip",
            "resize_clip",
            "split_clip",
            "clone_clip",
            "delete_clip",
            "set_clip_mute",
            "set_clip_name",
            "set_clip_color",
            "insert_bar",
            "remove_bar",
            "list_tracks",
            "get_track_details",
            "find_track_by_name",
        ],
        guided_now=[
            "right-click context menu item selection",
        ],
        deferred=["typed multi-select edits across several clips"],
        example_prompts=[
            "create instrument track",
            "rename track bass to Sub Bass",
            "solo drum track",
            "move the lead clip to bar 3",
            "split the bass clip at bar 2",
            "delete the bass track",
        ],
    ),
    FeatureArea(
        id="piano_roll_composition",
        title="Piano Roll note editing and composition",
        manual_refs=[
            "The Piano Roll Editor",
            "Note Length",
            "Note Volume",
            "Note Panning",
            "Composing in the Piano Roll Editor",
        ],
        keywords=["piano roll", "notes", "melody", "chord", "arpeggio", "quantize"],
        automated_now=[
            "add_notes",
            "add_steps",
            "edit_notes",
            "remove_notes",
            "clear_clip (confirm)",
            "quantize_clip",
            "humanize_clip",
            "reverse_clip",
            "split_clip_notes",
            "set_clip_velocity_scale",
            "add_chord",
            "add_arpeggio",
            "create_pattern",
            "open_tool (piano roll)",
        ],
        guided_now=[
            "precise note-drawing by mouse",
            "clipboard-heavy edits across bars",
        ],
        deferred=["full typed piano-roll selection/zoom APIs"],
        example_prompts=[
            "open piano roll",
            "add notes C4 E4 G4",
            "add a c major chord",
            "quantize the piano clip",
            "humanize the bass line",
            "arpeggiate a c major chord",
        ],
    ),
    FeatureArea(
        id="beat_bassline",
        title="Beat+Bassline rhythm and pattern library",
        manual_refs=[
            "The Beat+Bassline Editor",
            "Creating Beats",
            "Editing Beats",
            "The process flow for composing a rhythm",
        ],
        keywords=["beat", "bassline", "bb editor", "rhythm", "drums", "steps", "pattern"],
        automated_now=[
            "create_pattern",
            "select_pattern",
            "clone_pattern",
            "list_patterns",
            "set_steps",
            "set_step_velocity",
            "set_steps_per_bar",
            "add_rhythm (kick/snare/hihat/crash/ride presets)",
            "create_track(type=sample)",
            "load_sample",
            "add_steps",
            "open_tool (bb editor if available in your build name map)",
        ],
        guided_now=[
            "step-grid editing nuances",
            "pattern-level rhythm design decisions",
        ],
        deferred=["native typed BB timeline with per-step swing curves"],
        example_prompts=[
            "add rhythm",
            "create sample track",
            "load sample kick",
            "add four on the floor kick pattern",
            "select pattern 2",
            "accent the snare on step 12",
        ],
    ),
    FeatureArea(
        id="instrument_plugins",
        title="Instrument plugins, presets, VST, and SF2",
        manual_refs=[
            "Instrument Sound controls",
            "The ENV/LFO tab",
            "The Func tab",
            "The FX tab",
            "The MIDI tab",
            "Appendix A: Instrument Plugins",
        ],
        keywords=["instrument", "plugin", "preset", "triple oscillator", "sf2", "zynaddsubfx", "vst"],
        automated_now=[
            "list_instruments",
            "resolve_plugin(type=instrument)",
            "load_instrument",
            "load_instrument_preset (xiz/sf2/pat/xp)",
            "set_vst_program",
            "set_vst_param",
            "set_sf2_patch",
            "describe_instrument",
            "resolve_preset",
        ],
        guided_now=[
            "preset browser deep navigation",
            "VST plugin picker UI",
        ],
        deferred=["typed per-plugin parameter schema save/recall"],
        example_prompts=[
            "load instrument triple oscillator",
            "load the daft punk lead preset",
            "load a soundfont",
            "set vst program 3 on VST Track",
            "list instruments",
        ],
    ),
    FeatureArea(
        id="sound_shaping",
        title="Sound shaping (envelope, filter, LFO, arp)",
        manual_refs=[
            "Instrument Sound controls",
            "The ENV/LFO tab",
            "The Func tab",
            "The FX tab",
        ],
        keywords=["envelope", "attack", "sustain", "filter", "lfo", "arp", "wobble", "sound design"],
        automated_now=[
            "set_envelope (attack/hold/decay/sustain/release per target)",
            "set_filter (lowpass/highpass/bandpass/notch/moog/2xlowpass)",
            "set_lfo (amount/speed/wave per target)",
            "set_arp (chord/range/direction/time/gate/mode)",
            "set_note_stacking (type/range)",
            "set_param (model address param)",
        ],
        guided_now=[
            "multi-target modulation routing choices",
        ],
        deferred=["visual curve editors for envelope/LFO shapes"],
        example_prompts=[
            "give the bass a long attack",
            "add a lowpass filter to the lead",
            "make the bass wobble with an lfo",
            "enable arpeggio on the pluck",
        ],
    ),
    FeatureArea(
        id="fx_mixer_and_effects",
        title="FX Mixer, effects, and routing",
        manual_refs=[
            "The FX Mixer",
            "The channel structure",
            "The Effects Chain pane",
            "Appendix A: Effect Plugins",
            "Appendix E: Adding Special Effects",
        ],
        keywords=["mixer", "fx", "effect", "reverb", "delay", "eq", "sidechain", "send", "channel"],
        automated_now=[
            "open_tool(mixer)",
            "list_effects",
            "add_effect",
            "remove_effect",
            "set_effect_param",
            "get_effect_params",
            "move_effect",
            "set_effect_enabled",
            "set_effect_wetdry",
            "list_mixer_channels",
            "create_channel",
            "delete_channel (confirm)",
            "rename_channel",
            "move_channel",
            "set_channel_volume",
            "set_channel_mute",
            "set_channel_solo",
            "add_channel_effect",
            "remove_channel_effect",
            "create_send",
            "delete_send",
            "set_send_amount",
            "route_track_to_channel",
            "get_peak_levels",
        ],
        guided_now=[
            "A/B listening workflow",
        ],
        deferred=["visual mixer strip drag gestures"],
        example_prompts=[
            "show mixer",
            "add effect reverb",
            "increase the reverb wet dry to 30 percent",
            "create a send from the bass to the reverb bus",
            "route the lead to mixer channel 3",
            "what is clipping",
            "create a new fx channel",
        ],
    ),
    FeatureArea(
        id="automation_and_controllers",
        title="Automation Editor and Controller Rack",
        manual_refs=[
            "The Automation Editor",
            "Controller Rack",
            "LFO Controllers",
            "Peak Controller",
            "Appendix C: Editing the Automation Curve",
        ],
        keywords=["automation", "lfo", "controller", "peak controller", "curve"],
        automated_now=[
            "create_automation",
            "automate (draw curves)",
            "set_automation_node",
            "remove_automation_node",
            "set_automation_tension",
            "set_automation_progression",
            "global_automate (confirm)",
            "read_automation",
            "list_automation",
            "create_controller (lfo/midi/peak)",
            "set_lfo_controller",
            "connect_controller",
            "disconnect_controller (confirm)",
            "describe_controllers",
            "create_snapshot",
            "undo_last_action",
            "rollback_to_snapshot",
        ],
        guided_now=[
            "visual curve dragging refinement",
        ],
        deferred=["typed multi-node curve interpolation UI"],
        example_prompts=[
            "automate the filter cutoff up over 4 bars",
            "create global automation for master volume",
            "add an lfo controller and link it to the cutoff",
            "make the kick duck the bass with a peak controller",
            "list automation clips",
            "undo",
            "rollback to snapshot",
        ],
    ),
    FeatureArea(
        id="import_and_samples",
        title="Import, samples, and audio assets",
        manual_refs=[
            "Working with sample tracks",
            "Appendix D: Working with samples",
            "Import workflows in practical chapters",
        ],
        keywords=["sample", "import", "audio", "wav", "mp3", "midi", "hydrogen"],
        automated_now=[
            "import_audio",
            "import_midi",
            "import_hydrogen",
            "search_project_audio",
            "resolve_sample",
            "load_sample",
            "set_sample_loop (off/on/pingpong)",
            "set_sample_pitch",
            "set_sample_amp",
            "set_sample_range",
        ],
        guided_now=[
            "browser-only sample audition workflows",
        ],
        deferred=["typed sample slice/normalize batch API"],
        example_prompts=[
            "import midi /path/file.mid",
            "load sample 808 kick",
            "loop the kick sample",
            "pitch the sample up 12 semitones",
            "trim the sample region",
        ],
    ),
    FeatureArea(
        id="render_and_export",
        title="Render, export, and MIDI recording",
        manual_refs=[
            "Editing and Composing Songs -> Exporting the Song",
            "MIDI recording workflows",
        ],
        keywords=["render", "export", "stems", "wav", "mp3", "flac", "ogg", "record", "midi record"],
        automated_now=[
            "render_song (wav/flac/ogg/mp3)",
            "render_tracks (stems)",
            "render_preview (loop range)",
            "get_render_progress",
            "cancel_render",
            "export_midi",
            "record_arm",
            "record_disarm",
            "record_start",
            "record_stop (with quantize)",
            "save_project",
            "save_project_as",
        ],
        guided_now=[
            "codec/bitrate trade-off decisions",
        ],
        deferred=["multi-render queue and background batch export"],
        example_prompts=[
            "render song to wav",
            "export stems to a folder",
            "render a preview of the loop",
            "arm the midi recording",
            "save project as my song.mmp",
        ],
    ),
    FeatureArea(
        id="beginner_song_building",
        title="Beginner song-building playbooks",
        manual_refs=[
            "Editing and Composing Songs",
            "Composing a song from score sheet",
            "Adding Rhythm",
            "Adding Automation",
            "Exporting the Song",
        ],
        keywords=["beginner", "playbook", "compose from score", "add rhythm", "export song"],
        automated_now=[
            "planner playbooks over real v2 tools (score/rhythm/automation/multiple instruments/samples/export)",
            "four on the floor, filter sweep, sidechain ducking, render stems, load soundfont, vst setup",
            "guided interactive mode (confirm each step)",
        ],
        guided_now=[
            "manual confirmation checkpoints",
            "UI-only checks before export",
        ],
        deferred=["fully autonomous full-song generation without confirmation"],
        example_prompts=["beginner help", "compose from score sheet", "export song", "filter sweep"],
    ),
    FeatureArea(
        id="shortcuts_and_power_use",
        title="Keyboard shortcuts and power-user workflow",
        manual_refs=["Appendix B: Keyboard shortcuts"],
        keywords=["shortcut", "hotkey", "keyboard", "workflow speed"],
        automated_now=["documented guidance only"],
        guided_now=["human-operated keyboard workflow in LMMS UI"],
        deferred=["typed shortcut simulation layer (not planned for v1)"],
        example_prompts=["beginner help", "show me shortcut-friendly flow"],
    ),
]


def list_feature_areas() -> List[FeatureArea]:
    return FEATURE_AREAS


def find_feature_areas(query: str, limit: int = 3) -> List[FeatureArea]:
    text = query.lower()
    ranked = []
    for area in FEATURE_AREAS:
        score = 0
        for kw in area.keywords:
            if kw in text:
                score += 3
        for ref in area.manual_refs:
            if ref.lower() in text:
                score += 2
        if area.id.replace("_", " ") in text:
            score += 2
        if score > 0:
            ranked.append((score, area))
    ranked.sort(key=lambda item: item[0], reverse=True)
    return [area for _, area in ranked[:limit]]


def render_feature_area(area: FeatureArea) -> str:
    lines = [
        f"Feature Area: {area.title}",
        f"Manual refs: {', '.join(area.manual_refs)}",
        "Automated now:",
    ]
    lines.extend([f"- {item}" for item in area.automated_now])
    lines.append("Guided/manual now:")
    lines.extend([f"- {item}" for item in area.guided_now])
    lines.append("Deferred:")
    lines.extend([f"- {item}" for item in area.deferred])
    lines.append("Try saying:")
    lines.extend([f"- {item}" for item in area.example_prompts])
    return "\n".join(lines)


def render_feature_catalog() -> str:
    lines = ["LMMS feature map from manual (0.4.12), updated for the v2 tool surface:"]
    for area in FEATURE_AREAS:
        lines.append(f"- {area.title} [{area.id}]")
    lines.append("Use commands like: manual map song editor, manual map automation, manual map mixer, manual map render")
    return "\n".join(lines)
