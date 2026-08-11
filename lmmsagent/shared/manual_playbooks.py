from __future__ import annotations

from dataclasses import dataclass, field
from typing import Any, Dict, List, Optional


@dataclass
class PlaybookStep:
    title: str
    action: str
    args: Dict[str, Any] = field(default_factory=dict)
    confidence: float = 0.9
    risk: str = "safe"
    requires_snapshot: bool = False


@dataclass
class Playbook:
    id: str
    title: str
    manual_section: str
    caution: str
    keywords: List[str]
    steps: List[PlaybookStep]


# Playbooks execute real AgentControl v2 tool sequences (TOOL_CONTRACT_V2.md
# section 2). Tool names are snake_case; AgentControl dispatch normalizes them
# by lowercasing + stripping non-alphanumerics (create_send -> createsend).
PLAYBOOKS: List[Playbook] = [
    Playbook(
        id="compose_from_score_sheet",
        title="Compose a melody from score sheet (beginner)",
        manual_section="Editing and Composing Songs -> Composing a song from score sheet",
        caution=(
            "Manual is from LMMS 0.4.12. Core workflow still applies, but UI labels and defaults may differ."
        ),
        keywords=["score", "sheet", "compose", "melody", "song from score", "beginner song"],
        steps=[
            PlaybookStep(
                title="Set working tempo",
                action="set_tempo",
                args={"tempo": 140},
                confidence=0.96,
            ),
            PlaybookStep(
                title="Open Song Editor",
                action="open_tool",
                args={"name": "song editor", "kind": "window"},
                confidence=0.9,
            ),
            PlaybookStep(
                title="Create an instrument track",
                action="create_track",
                args={"type": "instrument", "name": "Lead"},
                confidence=0.93,
            ),
            PlaybookStep(
                title="Load Triple Oscillator",
                action="load_instrument",
                args={"plugin": "tripleoscillator", "track": "Lead"},
                confidence=0.92,
                requires_snapshot=True,
            ),
            PlaybookStep(
                title="Create first pattern clip",
                action="create_pattern",
                args={"name": "Melody A"},
                confidence=0.86,
            ),
            PlaybookStep(
                title="Enter melody notes",
                action="add_notes",
                args={
                    "track": "Lead",
                    "notes": [
                        {"key": 60, "pos": 0, "length": 96, "velocity": 100},
                        {"key": 62, "pos": 96, "length": 96, "velocity": 100},
                        {"key": 64, "pos": 192, "length": 96, "velocity": 100},
                        {"key": 65, "pos": 288, "length": 96, "velocity": 100},
                        {"key": 67, "pos": 384, "length": 192, "velocity": 110},
                        {"key": 65, "pos": 576, "length": 96, "velocity": 100},
                        {"key": 64, "pos": 672, "length": 96, "velocity": 100},
                    ],
                },
                confidence=0.9,
            ),
            PlaybookStep(
                title="Quantize to 1/16 grid",
                action="quantize_clip",
                args={"track": "Lead", "resolution": 16},
                confidence=0.92,
            ),
            PlaybookStep(
                title="Place the pattern in the arrangement",
                action="create_clip",
                args={"track": "Lead", "tick": 0, "name": "Melody A"},
                confidence=0.88,
            ),
        ],
    ),
    Playbook(
        id="add_rhythm_bb",
        title="Add rhythm with Beat+Bassline workflow",
        manual_section="Beat+Bassline Editor -> process flow for composing a rhythm",
        caution="Beat+Bassline internals changed across LMMS versions; use this as guided workflow.",
        keywords=["rhythm", "drums", "beat", "bassline", "bb editor", "groove"],
        steps=[
            PlaybookStep(
                title="Create sample track for drums",
                action="create_track",
                args={"type": "sample", "name": "Drums"},
                confidence=0.9,
            ),
            PlaybookStep(
                title="Create drum pattern",
                action="create_pattern",
                args={"name": "Drums"},
                confidence=0.91,
            ),
            PlaybookStep(
                title="Add kick rhythm",
                action="add_rhythm",
                args={"drum": "kick", "pattern": [0, 4, 8, 12]},
                confidence=0.93,
            ),
            PlaybookStep(
                title="Add snare backbeat",
                action="add_rhythm",
                args={"drum": "snare", "pattern": [4, 12]},
                confidence=0.93,
            ),
            PlaybookStep(
                title="Add hi-hat eighths",
                action="add_rhythm",
                args={"drum": "hihat", "pattern": [0, 2, 4, 6, 8, 10, 12, 14]},
                confidence=0.93,
            ),
            PlaybookStep(
                title="Accent the snare backbeat",
                action="set_step_velocity",
                args={"track": "Drums", "step": 12, "velocity": 125},
                confidence=0.86,
            ),
            PlaybookStep(
                title="Place the pattern in the arrangement",
                action="create_clip",
                args={"track": "Drums", "tick": 0, "name": "Drums"},
                confidence=0.88,
            ),
        ],
    ),
    Playbook(
        id="automation_volume_contrast",
        title="Create beginner automation contrast",
        manual_section="Automation Editor + Song-global automation",
        caution="Song-global automation behavior differs by version; prefer automation track for reversible edits.",
        keywords=["automation", "volume automation", "fade", "volume"],
        steps=[
            PlaybookStep(
                title="Snapshot before automation",
                action="create_snapshot",
                args={"label": "before_automation"},
                confidence=0.97,
            ),
            PlaybookStep(
                title="Create automation clip on Lead volume",
                action="create_automation",
                args={"address": "track:Lead.volume", "name": "Volume Automation"},
                confidence=0.9,
            ),
            PlaybookStep(
                title="Draw fade-in curve over 4 bars",
                action="automate",
                args={
                    "address": "track:Lead.volume",
                    "ticks": [0, 192, 384, 576, 768],
                    "values": [0.15, 0.4, 0.6, 0.85, 1.0],
                },
                confidence=0.92,
            ),
            PlaybookStep(
                title="Verify the automation nodes",
                action="read_automation",
                args={"address": "track:Lead.volume"},
                confidence=0.9,
            ),
        ],
    ),
    Playbook(
        id="multiple_instruments_dialogue",
        title="Split melody across multiple instruments",
        manual_section="Further experimentation -> Using multiple instruments",
        caution="SF2 patch browsing is plugin-specific; keep the split workflow, adapt patch selection UI.",
        keywords=["multiple instruments", "layer", "sf2", "split melody", "orchestration"],
        steps=[
            PlaybookStep(
                title="Create secondary instrument track",
                action="create_track",
                args={"type": "instrument", "name": "Secondary Voice"},
                confidence=0.93,
            ),
            PlaybookStep(
                title="Load SF2 Player",
                action="load_instrument",
                args={"plugin": "sf2player", "track": "Secondary Voice"},
                confidence=0.86,
                requires_snapshot=True,
            ),
            PlaybookStep(
                title="Select a contrasting patch",
                action="set_sf2_patch",
                args={"track": "Secondary Voice", "patch": 1},
                confidence=0.85,
            ),
            PlaybookStep(
                title="Write the second voice part",
                action="add_notes",
                args={
                    "track": "Secondary Voice",
                    "notes": [
                        {"key": 55, "pos": 0, "length": 192, "velocity": 95},
                        {"key": 52, "pos": 192, "length": 192, "velocity": 95},
                        {"key": 57, "pos": 384, "length": 192, "velocity": 95},
                        {"key": 59, "pos": 576, "length": 192, "velocity": 95},
                    ],
                },
                confidence=0.88,
            ),
        ],
    ),
    Playbook(
        id="sample_workflow",
        title="Work with samples (melodic and percussion)",
        manual_section="Appendix D -> Working with samples",
        caution="Sample browser paths changed since 0.4.12; workflow remains valid.",
        keywords=["sample", "vocal chop", "one-shot", "percussion sample", "audio clip"],
        steps=[
            PlaybookStep(
                title="Create sample track",
                action="create_track",
                args={"type": "sample", "name": "Sample Source"},
                confidence=0.93,
            ),
            PlaybookStep(
                title="Load a bundled waveform sample",
                action="load_sample",
                args={"sample_path": "data/samples/waveforms/saw1.flac"},
                confidence=0.9,
                requires_snapshot=True,
            ),
            PlaybookStep(
                title="Enable sample looping",
                action="set_sample_loop",
                args={"track": "Sample Source", "mode": "on", "loop_start": 0, "loop_end": 44100},
                confidence=0.88,
            ),
            PlaybookStep(
                title="Set sample pitch",
                action="set_sample_pitch",
                args={"track": "Sample Source", "semitones": 0},
                confidence=0.9,
            ),
            PlaybookStep(
                title="Set sample amplification",
                action="set_sample_amp",
                args={"track": "Sample Source", "value": 0.8},
                confidence=0.89,
            ),
        ],
    ),
    Playbook(
        id="export_song",
        title="Export/render final song",
        manual_section="Editing and Composing Songs -> Exporting the Song",
        caution="Export codec options vary by build and installed codecs.",
        keywords=["export", "render", "bounce", "wav", "ogg", "mp3", "final mix"],
        steps=[
            PlaybookStep(
                title="Save project first",
                action="save_project",
                args={},
                confidence=0.98,
            ),
            PlaybookStep(
                title="Render a loop-range preview",
                action="render_preview",
                args={"begin_tick": 0, "end_tick": 768},
                confidence=0.93,
            ),
            PlaybookStep(
                title="Render full song to wav",
                action="render_song",
                args={"path": "export/song.wav", "format": "wav", "sample_rate": 44100, "bit_depth": 16},
                confidence=0.94,
            ),
            PlaybookStep(
                title="Poll render progress",
                action="get_render_progress",
                args={},
                confidence=0.92,
            ),
        ],
    ),
    Playbook(
        id="edit_existing_song",
        title="Edit an existing project safely",
        manual_section="Editing and Composing Songs -> Editing an existing song",
        caution="Manual workflow is valid, but exact panel names can vary by LMMS version/theme.",
        keywords=["edit existing song", "edit project", "open project and edit", "revise song"],
        steps=[
            PlaybookStep(
                title="Create recovery snapshot",
                action="create_snapshot",
                args={"label": "before_existing_song_edit"},
                confidence=0.98,
            ),
            PlaybookStep(
                title="Inspect current project state",
                action="get_project_state",
                args={},
                confidence=0.97,
            ),
            PlaybookStep(
                title="Describe the full song structure",
                action="describe_song",
                args={},
                confidence=0.95,
            ),
            PlaybookStep(
                title="Open Song Editor for arrangement pass",
                action="open_tool",
                args={"name": "song editor"},
                confidence=0.95,
            ),
            PlaybookStep(
                title="Open Mixer for level/effect pass",
                action="open_tool",
                args={"name": "mixer"},
                confidence=0.94,
            ),
        ],
    ),
    Playbook(
        id="samples_melodic_instrument",
        title="Use a sample as a melodic instrument",
        manual_section="Appendix D -> Sample Used as a Melody Instrument",
        caution="Sample browser and sampler controls changed since 0.4.12; keep to same concept flow.",
        keywords=["sample melody", "sample instrument", "melodic sample", "sample as instrument"],
        steps=[
            PlaybookStep(
                title="Create instrument track for sampler",
                action="create_track",
                args={"type": "instrument", "name": "Sample Instrument"},
                confidence=0.93,
            ),
            PlaybookStep(
                title="Load AudioFileProcessor",
                action="load_instrument",
                args={"plugin": "audiofileprocessor", "track": "Sample Instrument"},
                confidence=0.88,
                requires_snapshot=True,
            ),
            PlaybookStep(
                title="Load the source sample into the sampler",
                action="load_sample",
                args={"sample_path": "data/samples/waveforms/saw1.flac"},
                confidence=0.9,
            ),
            PlaybookStep(
                title="Set the base note (root key)",
                action="set_track_base_note",
                args={"track": "Sample Instrument", "key": 60},
                confidence=0.88,
            ),
            PlaybookStep(
                title="Create pattern for melodic test",
                action="create_pattern",
                args={"name": "Sample Melody Test"},
                confidence=0.85,
            ),
            PlaybookStep(
                title="Write a short phrase",
                action="add_notes",
                args={
                    "track": "Sample Instrument",
                    "notes": [
                        {"key": 60, "pos": 0, "length": 96, "velocity": 100},
                        {"key": 62, "pos": 96, "length": 96, "velocity": 100},
                        {"key": 64, "pos": 192, "length": 192, "velocity": 100},
                    ],
                },
                confidence=0.9,
            ),
        ],
    ),
    Playbook(
        id="samples_percussion_instrument",
        title="Use a sample as a percussion instrument",
        manual_section="Appendix D -> Sample Used as a Percussion Instrument",
        caution="Step-sequencer and sample-track UI differs by version; sequence remains valid.",
        keywords=["sample percussion", "drum sample", "one shot", "sample as drum"],
        steps=[
            PlaybookStep(
                title="Create drum sample track",
                action="create_track",
                args={"type": "sample", "name": "Percussion Sample"},
                confidence=0.94,
            ),
            PlaybookStep(
                title="Load a short one-shot sample",
                action="load_sample",
                args={"sample_path": "data/samples/waveforms/impulse.flac"},
                confidence=0.91,
                requires_snapshot=True,
            ),
            PlaybookStep(
                title="Program basic rhythm",
                action="add_steps",
                args={"track": "Percussion Sample", "steps": [0, 4, 8, 12]},
                confidence=0.82,
            ),
            PlaybookStep(
                title="Accent the downbeat",
                action="set_step_velocity",
                args={"track": "Percussion Sample", "step": 0, "velocity": 120},
                confidence=0.86,
            ),
        ],
    ),
    Playbook(
        id="sidechain_through_mixer",
        title="Create side-chain style pumping through mixer",
        manual_section="Appendix E -> Side-chaining through FX-Mixer",
        caution="Routing UI and plugin choices differ by build; use this as controlled guided workflow.",
        keywords=["sidechain", "ducking", "pumping", "mixer routing"],
        steps=[
            PlaybookStep(
                title="Create safety snapshot",
                action="create_snapshot",
                args={"label": "before_sidechain"},
                confidence=0.98,
            ),
            PlaybookStep(
                title="Open mixer",
                action="open_tool",
                args={"name": "mixer"},
                confidence=0.97,
            ),
            PlaybookStep(
                title="Create a sidechain bus channel",
                action="create_channel",
                args={"name": "Sidechain Bus"},
                confidence=0.92,
            ),
            PlaybookStep(
                title="Create a send from the kick to the bus",
                action="create_send",
                args={"from": "Kick", "to": "Sidechain Bus", "amount": 0.5},
                confidence=0.9,
            ),
            PlaybookStep(
                title="Trim the send amount",
                action="set_send_amount",
                args={"from": "Kick", "to": "Sidechain Bus", "amount": 0.6},
                confidence=0.88,
            ),
            PlaybookStep(
                title="Balance the bus level",
                action="set_channel_volume",
                args={"channel": "Sidechain Bus", "value": 0.8},
                confidence=0.9,
            ),
        ],
    ),
    Playbook(
        id="lfo_controller_modulation",
        title="Apply LFO controller modulation",
        manual_section="Controller Rack -> LFO Controllers",
        caution="Controller rack visuals changed since 0.4.12; concept and signal flow remain same.",
        keywords=["lfo controller", "lfo modulation", "controller rack", "modulate knob"],
        steps=[
            PlaybookStep(
                title="Open controller rack",
                action="open_tool",
                args={"name": "controller rack"},
                confidence=0.95,
            ),
            PlaybookStep(
                title="Create an LFO controller",
                action="create_controller",
                args={"type": "lfo", "name": "Filter LFO"},
                confidence=0.92,
            ),
            PlaybookStep(
                title="Tune LFO wave, speed, and amount",
                action="set_lfo_controller",
                args={"controller": "Filter LFO", "wave": "sine", "speed": 0.5, "amount": 0.3},
                confidence=0.9,
            ),
            PlaybookStep(
                title="Bind LFO to filter cutoff",
                action="connect_controller",
                args={"controller": "Filter LFO", "address": "track:Lead.filter.cutoff"},
                confidence=0.9,
            ),
            PlaybookStep(
                title="Snapshot tuned state",
                action="create_snapshot",
                args={"label": "after_lfo_modulation"},
                confidence=0.95,
            ),
        ],
    ),
    Playbook(
        id="four_on_the_floor",
        title="Program a four-on-the-floor kick groove",
        manual_section="Beat+Bassline Editor -> Creating Beats (four on the floor)",
        caution="Use a clean kick one-shot; load your own sample to replace the bundled impulse.",
        keywords=["four on the floor", "4/4 kick", "house beat", "kick groove", "dance beat"],
        steps=[
            PlaybookStep(
                title="Create a kick sample track",
                action="create_track",
                args={"type": "sample", "name": "Kick"},
                confidence=0.93,
            ),
            PlaybookStep(
                title="Load a kick one-shot",
                action="load_sample",
                args={"sample_path": "data/samples/waveforms/impulse.flac"},
                confidence=0.9,
                requires_snapshot=True,
            ),
            PlaybookStep(
                title="Create the groove pattern",
                action="create_pattern",
                args={"name": "Four on the Floor"},
                confidence=0.9,
            ),
            PlaybookStep(
                title="Place a kick on every beat",
                action="add_rhythm",
                args={"drum": "kick", "pattern": [0, 4, 8, 12]},
                confidence=0.94,
            ),
            PlaybookStep(
                title="Clone a variation pattern",
                action="clone_pattern",
                args={"pattern": "Four on the Floor", "name": "Four on the Floor Var"},
                confidence=0.88,
            ),
            PlaybookStep(
                title="Place the pattern in the arrangement",
                action="create_clip",
                args={"track": "Kick", "tick": 0, "name": "Four on the Floor"},
                confidence=0.88,
            ),
        ],
    ),
    Playbook(
        id="filter_sweep",
        title="Build a filter sweep with automation",
        manual_section="Automation Editor -> Filter sweep",
        caution="Cutoff address is track:Lead.filter.cutoff; rename the track or adjust address if needed.",
        keywords=["filter sweep", "cutoff automation", "sweep", "open the filter", "wobble"],
        steps=[
            PlaybookStep(
                title="Snapshot before the sweep",
                action="create_snapshot",
                args={"label": "before_filter_sweep"},
                confidence=0.97,
            ),
            PlaybookStep(
                title="Set an initial closed lowpass",
                action="set_filter",
                args={"track": "Lead", "type": "lowpass", "cutoff": 0.15, "resonance": 0.4},
                confidence=0.9,
            ),
            PlaybookStep(
                title="Create automation on filter cutoff",
                action="create_automation",
                args={"address": "track:Lead.filter.cutoff", "name": "Filter Sweep"},
                confidence=0.91,
            ),
            PlaybookStep(
                title="Draw the sweep curve over 4 bars",
                action="automate",
                args={
                    "address": "track:Lead.filter.cutoff",
                    "ticks": [0, 192, 384, 576, 768],
                    "values": [0.05, 0.2, 0.5, 0.8, 1.0],
                },
                confidence=0.92,
            ),
            PlaybookStep(
                title="Verify the curve",
                action="read_automation",
                args={"address": "track:Lead.filter.cutoff"},
                confidence=0.9,
            ),
        ],
    ),
    Playbook(
        id="sidechain_ducking",
        title="Sidechain ducking with a peak controller",
        manual_section="Controller Rack -> Peak Controller (sidechain ducking)",
        caution="Peak metering needs the core patch; get_peak_levels returns not_available otherwise.",
        keywords=["sidechain ducking", "duck the bass", "pump with the kick", "peak controller"],
        steps=[
            PlaybookStep(
                title="Create safety snapshot",
                action="create_snapshot",
                args={"label": "before_ducking"},
                confidence=0.98,
            ),
            PlaybookStep(
                title="Create a peak controller on the kick",
                action="create_controller",
                args={"type": "peak", "name": "Kick Peak"},
                confidence=0.92,
            ),
            PlaybookStep(
                title="Connect the peak controller to the bass volume",
                action="connect_controller",
                args={"controller": "Kick Peak", "address": "track:Bass.volume"},
                confidence=0.9,
            ),
            PlaybookStep(
                title="Confirm the peak signal is live",
                action="get_peak_levels",
                args={"channel": "Kick"},
                confidence=0.87,
            ),
        ],
    ),
    Playbook(
        id="render_stems",
        title="Render stems (per-track export)",
        manual_section="Editing and Composing Songs -> Exporting the Song (per-track)",
        caution="render_tracks exports each track to its own file; directory is created if missing.",
        keywords=["render stems", "export stems", "per track export", "stems", "separate tracks"],
        steps=[
            PlaybookStep(
                title="Save project first",
                action="save_project",
                args={},
                confidence=0.98,
            ),
            PlaybookStep(
                title="Render each track to its own stem file",
                action="render_tracks",
                args={"dir": "export/stems", "prefix": "stem", "format": "wav", "sample_rate": 44100, "bit_depth": 24},
                confidence=0.92,
            ),
            PlaybookStep(
                title="Poll render progress",
                action="get_render_progress",
                args={},
                confidence=0.91,
            ),
        ],
    ),
    Playbook(
        id="load_soundfont",
        title="Load a soundfont (SF2) instrument",
        manual_section="Instrument Plugins -> Sf2 Player",
        caution=(
            "Point load_instrument_preset at your own .sf2 (for example data/presets/sf2/gm.sf2); "
            "no soundfont ships in this repo."
        ),
        keywords=["soundfont", "sf2", "load sf2", "gm bank", "sf2 player"],
        steps=[
            PlaybookStep(
                title="Create an instrument track for the soundfont",
                action="create_track",
                args={"type": "instrument", "name": "Soundfont"},
                confidence=0.93,
            ),
            PlaybookStep(
                title="Load the Sf2 Player instrument",
                action="load_instrument",
                args={"plugin": "sf2player", "track": "Soundfont"},
                confidence=0.88,
                requires_snapshot=True,
            ),
            PlaybookStep(
                title="Load the soundfont file",
                action="load_instrument_preset",
                args={"track": "Soundfont", "path": "data/presets/sf2/gm.sf2"},
                confidence=0.85,
            ),
            PlaybookStep(
                title="Select a patch from the bank",
                action="set_sf2_patch",
                args={"track": "Soundfont", "bank": 0, "patch": 0},
                confidence=0.87,
            ),
            PlaybookStep(
                title="Describe the loaded instrument",
                action="describe_instrument",
                args={"track": "Soundfont"},
                confidence=0.9,
            ),
        ],
    ),
    Playbook(
        id="vst_setup",
        title="Set up a VST instrument track",
        manual_section="Instrument Plugins -> VST (Vestige)",
        caution="Vestige must find the VST; program and param indices depend on the specific plugin.",
        keywords=["vst", "vestige", "vst plugin", "vst setup", "plugin instrument"],
        steps=[
            PlaybookStep(
                title="Create an instrument track for the VST",
                action="create_track",
                args={"type": "instrument", "name": "VST Track"},
                confidence=0.93,
            ),
            PlaybookStep(
                title="Load the Vestige VST host",
                action="load_instrument",
                args={"plugin": "vestige", "track": "VST Track"},
                confidence=0.88,
                requires_snapshot=True,
            ),
            PlaybookStep(
                title="Select program 0",
                action="set_vst_program",
                args={"track": "VST Track", "program": 0},
                confidence=0.86,
            ),
            PlaybookStep(
                title="Set a VST parameter by index",
                action="set_vst_param",
                args={"track": "VST Track", "param": 0, "value": 0.5},
                confidence=0.85,
            ),
            PlaybookStep(
                title="Describe the VST parameter tree",
                action="describe_instrument",
                args={"track": "VST Track"},
                confidence=0.9,
            ),
        ],
    ),
]


def list_playbooks() -> List[Dict[str, str]]:
    return [
        {
            "id": pb.id,
            "title": pb.title,
            "manual_section": pb.manual_section,
            "caution": pb.caution,
        }
        for pb in PLAYBOOKS
    ]


def find_playbook_for_goal(goal: str) -> Optional[Playbook]:
    text = goal.lower()
    best: Optional[Playbook] = None
    best_score = 0
    for playbook in PLAYBOOKS:
        score = sum(1 for kw in playbook.keywords if kw in text)
        if score > best_score:
            best = playbook
            best_score = score
    return best
