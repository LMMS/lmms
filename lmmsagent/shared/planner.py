from __future__ import annotations

import re
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Dict, List, Optional, Tuple

from .discovery import DiscoveryIndex
from .manual_features import find_feature_areas, render_feature_area, render_feature_catalog
from .manual_playbooks import find_playbook_for_goal, list_playbooks
from .types import PlanStep, PlannerOutput, Subgoal

AMBIGUOUS_TERMS = {
    "harder",
    "texture",
    "vibe",
    "darker",
    "brighter",
    "better",
    "clean up",
    "clean",
    "that plugin",
    "some plugin",
}

CREATIVE_PHRASES = {
    "add energy to drums": "Do you want denser hits, heavier sample choice, or stronger processing?",
    "darken": "Do you want darker timbre (sample/instrument swap) or darker processing (EQ/filter)?",
    "brighten": "Do you want brighter timbre (new source) or brighter processing (EQ/exciter)?",
    "add texture": "Should texture come from ambience effects, layered samples, or a new instrument layer?",
    "hit harder": "Do you want a punchier pattern, a heavier sample, or stronger processing?",
    "muddy": "Should I clean low-mids on bass, drums, or the full mix bus first?",
}

TICKS_PER_BAR = 192

NOTE_SEMITONES = {
    "c": 0, "c#": 1, "db": 1, "d": 2, "d#": 3, "eb": 3, "e": 4, "f": 5,
    "f#": 6, "gb": 6, "g": 7, "g#": 8, "ab": 8, "a": 9, "a#": 10, "bb": 10, "b": 11,
}

CHORD_TYPES = ("major", "minor", "maj7", "min7", "7", "dim", "sus2", "sus4", "power")

DRUM_TYPES = ("kick", "snare", "hihat", "crash", "ride")

DEFAULT_RHYTHMS = {
    "kick": [0, 4, 8, 12],
    "snare": [4, 12],
    "hihat": [0, 2, 4, 6, 8, 10, 12, 14],
    "crash": [0],
    "ride": [0, 4, 8, 12],
}

RENDER_FORMATS = ("wav", "flac", "ogg", "mp3")

EXACT_INTENTS = {
    "play": ("transport.play", "play"),
    "pause": ("transport.pause", "pause"),
    "stop": ("transport.stop", "stop"),
}

# Argument-free (or fixed-argument) v2 intents from TOOL_CONTRACT_V2.md section 2,
# matched in order after the bespoke argument-extracting handlers.
@dataclass(frozen=True)
class IntentRule:
    intent: str
    action: str
    keywords: Tuple[str, ...]
    risk: str = "safe"
    requires_snapshot: bool = False
    confidence: float = 0.9
    args: Optional[Dict[str, Any]] = None


INTENT_RULES: List[IntentRule] = [
    IntentRule("project.save", "save_project", ("save project", "save my work"), confidence=0.95),
    IntentRule("project.loop.clear", "clear_loop", ("clear loop", "remove loop points"), confidence=0.93),
    IntentRule("note.reverse", "reverse_clip", ("reverse notes", "play backwards"), confidence=0.9),
    IntentRule("pattern.list", "list_patterns", ("list patterns", "show patterns"), confidence=0.91),
    IntentRule("automation.list", "list_automation", ("list automation", "show automation clips"), confidence=0.92),
    IntentRule("mixer.list", "list_mixer_channels", ("list mixer channels", "show mixer channels", "which mixer channels"), confidence=0.92),
    IntentRule("mixer.peak", "get_peak_levels", ("peak levels", "what is clipping", "meter levels", "show peaks"), confidence=0.87),
    IntentRule("controller.describe", "describe_controllers", ("describe controllers", "show controllers", "list controllers"), confidence=0.91),
    IntentRule("record.arm", "record_arm", ("arm recording", "arm midi record", "arm the midi", "arm the", "prepare to record"), confidence=0.9),
    IntentRule("record.start", "record_start", ("start recording", "record me playing", "capture notes"), confidence=0.9),
    IntentRule("record.stop", "record_stop", ("stop recording", "finish recording"), confidence=0.9),
    IntentRule("record.disarm", "record_disarm", ("disarm recording", "cancel recording"), confidence=0.9),
    IntentRule("render.progress", "get_render_progress", ("render progress", "how is the render", "render status"), confidence=0.9),
    IntentRule("render.cancel", "cancel_render", ("cancel render", "stop rendering"), confidence=0.9),
    IntentRule("misc.describe_song", "describe_song", ("describe song", "what is in the project", "project overview", "describe the project"), confidence=0.93),
    IntentRule("misc.get_notes", "get_project_notes", ("get project notes", "read project notes"), confidence=0.92),
    IntentRule("snapshot", "create_snapshot", ("take a snapshot", "create snapshot", "checkpoint", "save state"), confidence=0.95),
]

CONFIRM_GATED_ACTIONS = {
    "new_project",
    "open_project",
    "delete_track",
    "clear_clip",
    "global_automate",
    "delete_channel",
    "disconnect_controller",
}


@dataclass
class Planner:
    low_confidence_threshold: float = 0.70

    @staticmethod
    def _norm(text: str) -> str:
        return " ".join(text.lower().split())

    def _find_creative_question(self, goal: str) -> Optional[str]:
        goal_norm = self._norm(goal)
        for phrase, question in CREATIVE_PHRASES.items():
            if phrase in goal_norm:
                return question
        return None

    def _is_ambiguous(self, goal: str) -> bool:
        goal_norm = self._norm(goal)
        return any(term in goal_norm for term in AMBIGUOUS_TERMS)

    def _single_step_plan(
        self,
        goal: str,
        action: str,
        args: Dict[str, Any],
        confidence: float,
        risk: str = "safe",
        requires_snapshot: bool = False,
    ) -> PlannerOutput:
        return PlannerOutput(
            goal=goal,
            mode="plan",
            needs_clarification=False,
            subgoals=[
                Subgoal(
                    id="sg1",
                    title="Execute request",
                    steps=[
                        PlanStep(
                            action=action,
                            args=args,
                            confidence=confidence,
                            risk=risk,  # type: ignore[arg-type]
                            requires_snapshot=requires_snapshot,
                        )
                    ],
                )
            ],
        )

    def _compose_track_oriented_plan(self, goal: str, track_query: str, operation_step: PlanStep) -> PlannerOutput:
        return PlannerOutput(
            goal=goal,
            mode="plan",
            needs_clarification=False,
            subgoals=[
                Subgoal(
                    id="sg1",
                    title="Resolve target track",
                    steps=[
                        PlanStep(
                            action="resolve_track_reference",
                            args={"query": track_query},
                            confidence=0.86,
                            risk="safe",
                            requires_snapshot=False,
                        )
                    ],
                ),
                Subgoal(id="sg2", title="Apply change", steps=[operation_step]),
            ],
        )

    def _clarify(self, goal: str, question: str) -> PlannerOutput:
        return PlannerOutput(
            goal=goal,
            mode="clarify",
            needs_clarification=True,
            clarification_question=question,
            subgoals=[],
        )

    # ------------------------------------------------------------------ #
    # Argument extraction helpers
    # ------------------------------------------------------------------ #

    @staticmethod
    def _extract_int(text: str) -> Optional[int]:
        for token in re.findall(r"-?\d+", text):
            return int(token)
        return None

    @staticmethod
    def _extract_float(text: str) -> Optional[float]:
        match = re.search(r"\d+(?:\.\d+)?", text)
        return float(match.group(0)) if match else None

    @staticmethod
    def _extract_percent(text: str) -> Optional[float]:
        match = re.search(r"(\d+(?:\.\d+)?)\s*%", text)
        if match:
            return min(1.0, max(0.0, float(match.group(1)) / 100.0))
        value = Planner._extract_float(text)
        return min(1.0, max(0.0, value / 100.0)) if value is not None and value <= 100 else None

    @staticmethod
    def _extract_format(text: str) -> Optional[str]:
        for fmt in RENDER_FORMATS:
            if fmt in text:
                return fmt
        return None

    @staticmethod
    def _extract_drum(text: str) -> Optional[str]:
        for drum in DRUM_TYPES:
            if drum in text:
                return drum
        return None

    @staticmethod
    def _bar_tick(text: str, default: Optional[int] = None) -> Optional[int]:
        match = re.search(r"bar\s*(\d+)", text)
        if match:
            return (int(match.group(1)) - 1) * TICKS_PER_BAR
        return default

    @staticmethod
    def _extract_bars(text: str, default: int = 4) -> int:
        match = re.search(r"(?:over|for|across)\s+(\d+)\s+bar", text)
        return int(match.group(1)) if match else default

    def _known_track_names(self, state: Dict[str, Any]) -> List[str]:
        tracks = state.get("tracks", [])
        names: List[str] = []
        if isinstance(tracks, list):
            for item in tracks:
                if isinstance(item, dict) and item.get("name"):
                    names.append(str(item["name"]))
        return names

    def _extract_track_name(self, text: str, state: Dict[str, Any]) -> Optional[str]:
        known = sorted(self._known_track_names(state), key=len, reverse=True)
        for name in known:
            if self._norm(name) in text:
                return name
        match = re.search(r"(?:track|on|for)\s+([a-z][a-z0-9 _-]{1,30}?)(?:\s+(?:to|by|with|at|and)\b|$)", text)
        if match:
            return match.group(1).strip()
        return None

    def _extract_track_or_clarify(self, text: str, state: Dict[str, Any], goal: str) -> Optional[PlannerOutput]:
        track = self._extract_track_name(text, state)
        if not track:
            return self._clarify(goal, "Which track should I target for this operation?")
        return None

    def _automation_address(self, text: str, track: Optional[str]) -> Optional[str]:
        if "master volume" in text or ("master" in text and "volume" in text):
            return "song.master.volume"
        if "master pitch" in text:
            return "song.master.pitch"
        if "tempo" in text or "bpm" in text:
            return "song.tempo"
        if track:
            if "cutoff" in text or "sweep" in text:
                return f"track:{track}.filter.cutoff"
            if "resonance" in text or "reso" in text:
                return f"track:{track}.filter.reso"
            if "pan" in text:
                return f"track:{track}.pan"
            if "volume" in text or "fade" in text or "level" in text:
                return f"track:{track}.volume"
            if "pitch" in text:
                return f"track:{track}.pitch"
        return None

    def _automation_curve(self, text: str) -> Tuple[List[int], List[float]]:
        bars = self._extract_bars(text)
        end_tick = bars * TICKS_PER_BAR
        step = max(1, TICKS_PER_BAR // 4)
        ticks = list(range(0, end_tick + 1, step))
        count = max(2, len(ticks))
        if "fade out" in text or "ramp down" in text or "sweep down" in text or "close the filter" in text:
            values = [round(1.0 - i / (count - 1), 3) for i in range(count)]
        else:
            values = [round(i / (count - 1), 3) for i in range(count)]
        return ticks, values

    @staticmethod
    def _parse_note_tokens(text: str) -> List[str]:
        pattern = re.compile(r"\b(?:[a-g](?:#|b)?)(?:-?\d{1,2})?\b")
        tokens: List[str] = []
        for match in pattern.finditer(text):
            token = match.group(0).lower()
            stem = re.match(r"[a-g]#?|b", token)
            if not stem:
                continue
            base = stem.group(0)
            if base not in NOTE_SEMITONES:
                continue
            octave_match = re.search(r"(-?\d{1,2})$", token)
            octave = int(octave_match.group(1)) if octave_match else 4
            tokens.append(f"{base}{octave}")
        return tokens

    def _notes_from_tokens(self, tokens: List[str], start_tick: int = 0, step: int = 96, length: int = 48, velocity: int = 100) -> List[Dict[str, Any]]:
        notes: List[Dict[str, Any]] = []
        for idx, token in enumerate(tokens):
            base_match = re.match(r"([a-g]#?|b)(-?\d{1,2})$", token)
            if not base_match:
                continue
            base, octave = base_match.group(1), int(base_match.group(2))
            key = (octave + 1) * 12 + NOTE_SEMITONES[base]
            notes.append({"key": key, "pos": start_tick + idx * step, "length": length, "velocity": velocity})
        return notes

    def _parse_chord(self, text: str) -> Optional[Tuple[int, str]]:
        base_match = re.search(r"\b([a-g](?:#|b)?)(\d?)\b", text)
        if not base_match:
            return None
        base, octave_str = base_match.group(1).lower(), base_match.group(2)
        if base not in NOTE_SEMITONES:
            return None
        octave = int(octave_str) if octave_str else 4
        chord = "major"
        for candidate in CHORD_TYPES:
            if candidate in text:
                chord = candidate
                break
        root = (octave + 1) * 12 + NOTE_SEMITONES[base]
        return root, chord

    # ------------------------------------------------------------------ #
    # Family handlers (each returns Optional[PlannerOutput]; None = no match)
    # ------------------------------------------------------------------ #

    def _plan_project_transport(self, goal: str, text: str, state: Dict[str, Any]) -> Optional[PlannerOutput]:
        if "new project" in text or "start fresh" in text or "clear project" in text:
            return self._single_step_plan(
                goal, "new_project", {}, 0.94, risk="destructive", requires_snapshot=True
            )
        if "open project" in text or "load project" in text:
            path = text.split("open project", 1)[-1].strip() or text.split("load project", 1)[-1].strip()
            if not path:
                return self._clarify(goal, "Which project file should I open?")
            return self._single_step_plan(
                goal, "open_project", {"path": path}, 0.93, risk="destructive", requires_snapshot=True
            )
        if "save project as" in text or ("save" in text and "project" in text and " as " in text):
            if "save project as" in text:
                path = text.split("save project as", 1)[-1].strip()
            else:
                path = text.split(" as ", 1)[-1].strip()
            if not path:
                return self._clarify(goal, "What path should I save the project to?")
            return self._single_step_plan(goal, "save_project_as", {"path": path}, 0.93)
        if "save project" in text or "save my work" in text:
            return self._single_step_plan(goal, "save_project", {}, 0.95)
        if "tempo" in text or text.startswith("bpm"):
            tempo = self._extract_int(text)
            if tempo is None or not (20 <= tempo <= 300):
                return self._clarify(goal, "What BPM should I set (20-300)?")
            return self._single_step_plan(goal, "set_tempo", {"tempo": tempo}, 0.97)
        if "time signature" in text or "meter" in text or "make it" in text and "/" in text:
            sig_match = re.search(r"(\d+)\s*/\s*(\d+)", text)
            if sig_match:
                numerator, denominator = int(sig_match.group(1)), int(sig_match.group(2))
                return self._single_step_plan(
                    goal, "set_time_signature", {"numerator": numerator, "denominator": denominator}, 0.92
                )
            if "time signature" in text or "meter" in text or ("make it" in text and self._extract_int(text) is not None):
                numerator = self._extract_int(text)
                return self._single_step_plan(
                    goal,
                    "set_time_signature",
                    {"numerator": numerator if numerator else 3, "denominator": 4},
                    0.85,
                )
        if "metronome" in text or "click track" in text:
            enabled = not any(word in text for word in ("off", "disable", "turn off", "stop"))
            return self._single_step_plan(goal, "set_metronome", {"enabled": enabled}, 0.93)
        if ("master volume" in text or ("master" in text and ("quieter" in text or "louder" in text))) and not any(
            word in text for word in ("automate", "automation", "global")
        ):
            value = self._extract_percent(text)
            if value is None:
                value = 0.5 if any(word in text for word in ("down", "quieter", "lower")) else 0.8
            return self._single_step_plan(goal, "set_master_volume", {"value": value}, 0.9)
        if "master pitch" in text:
            value = self._extract_float(text)
            if value is None:
                return self._clarify(goal, "What master pitch value should I set?")
            return self._single_step_plan(goal, "set_master_pitch", {"value": value}, 0.9)
        if "loop from bar" in text or "loop from" in text or "set loop" in text or "loop this section" in text or "loop points" in text:
            begin = self._bar_tick(text, default=0)
            match_end = re.search(r"to bar\s*(\d+)", text)
            end_tick = int(match_end.group(1)) * TICKS_PER_BAR if match_end else begin + 4 * TICKS_PER_BAR
            return self._single_step_plan(
                goal, "set_loop", {"begin_tick": begin, "end_tick": end_tick}, 0.91
            )
        if "clear loop" in text or "remove loop" in text:
            return self._single_step_plan(goal, "clear_loop", {}, 0.94)
        if "go to bar" in text or "seek to" in text or "move playhead" in text or "jump to position" in text or "start at bar" in text:
            tick = self._bar_tick(text)
            if tick is None:
                return self._clarify(goal, "To which bar should I move the playhead?")
            return self._single_step_plan(goal, "set_play_pos", {"bar": tick // TICKS_PER_BAR + 1}, 0.93)
        if "play pattern" in text:
            pattern = text.split("play pattern", 1)[-1].strip() or None
            args: Dict[str, Any] = {}
            if pattern:
                args["pattern"] = pattern
            return self._single_step_plan(goal, "play_pattern", args, 0.9)
        if "play clip" in text:
            track = self._extract_track_name(text, state)
            args: Dict[str, Any] = {}
            if track:
                args["track"] = track
            return self._single_step_plan(goal, "play_clip", args, 0.88)
        if "insert bar" in text or "add a bar" in text:
            at_tick = self._bar_tick(text, default=0)
            return self._single_step_plan(goal, "insert_bar", {"at_tick": at_tick}, 0.9)
        if "remove bar" in text or "delete bar" in text:
            at_tick = self._bar_tick(text, default=0)
            return self._single_step_plan(goal, "remove_bar", {"at_tick": at_tick}, 0.9)
        if "stop behaviour" in text or "stop behavior" in text:
            mode = "back_to_zero"
            for candidate in ("continue", "back to start", "back to zero"):
                if candidate in text:
                    mode = candidate
                    break
            return self._single_step_plan(goal, "set_stop_behaviour", {"mode": mode}, 0.88)
        return None

    def _plan_arrangement(self, goal: str, text: str, state: Dict[str, Any]) -> Optional[PlannerOutput]:
        track = self._extract_track_name(text, state)

        if "create clip" in text or "add a clip" in text or "add segment" in text or "place pattern at bar" in text:
            args: Dict[str, Any] = {}
            if track:
                args["track"] = track
            tick = self._bar_tick(text)
            if tick is not None:
                args["tick"] = tick
            name_match = re.search(r"name[di]?\s+([a-z0-9 _-]+)$", text)
            if name_match:
                args["name"] = name_match.group(1).strip()
            return self._single_step_plan(goal, "create_clip", args, 0.9)
        if "move clip" in text or "move segment" in text or "shift pattern" in text or ("move" in text and "clip" in text):
            if not track:
                return self._clarify(goal, "Which clip (track) should I move?")
            new_tick = self._bar_tick(text)
            if new_tick is None:
                return self._clarify(goal, "To which bar should I move the clip?")
            return self._single_step_plan(
                goal, "move_clip", {"track": track, "clip_index": 0, "new_tick": new_tick}, 0.88
            )
        if "resize clip" in text or "make the clip longer" in text or "make the clip shorter" in text:
            if not track:
                return self._clarify(goal, "Which clip (track) should I resize?")
            bars = self._extract_bars(text, default=4)
            return self._single_step_plan(
                goal, "resize_clip", {"track": track, "clip_index": 0, "new_length": bars * TICKS_PER_BAR}, 0.87
            )
        if "split clip" in text or "cut clip" in text or "divide segment" in text or ("split" in text and "clip" in text):
            if not track:
                return self._clarify(goal, "Which clip (track) should I split?")
            at_tick = self._bar_tick(text, default=2 * TICKS_PER_BAR)
            return self._single_step_plan(
                goal, "split_clip", {"track": track, "clip_index": 0, "at_tick": at_tick}, 0.88
            )
        if (
            "clone clip" in text
            or "copy clip" in text
            or "duplicate segment" in text
            or "repeat the pattern" in text
            or ("clone" in text and "clip" in text)
        ):
            if not track:
                return self._clarify(goal, "Which clip (track) should I clone?")
            return self._single_step_plan(goal, "clone_clip", {"track": track, "clip_index": 0}, 0.9)
        if "delete clip" in text or "remove segment" in text or (("delete" in text or "remove" in text) and "clip" in text):
            if not track:
                return self._clarify(goal, "Which clip (track) should I delete?")
            return self._single_step_plan(goal, "delete_clip", {"track": track, "clip_index": 0}, 0.89)
        if "mute clip" in text or "unmute clip" in text or (("mute" in text or "unmute" in text) and "clip" in text):
            if not track:
                return self._clarify(goal, "Which clip (track) should I mute?")
            mute = "mute" in text and "unmute" not in text
            return self._single_step_plan(goal, "set_clip_mute", {"track": track, "clip_index": 0, "mute": mute}, 0.9)
        if "name clip" in text or "rename clip" in text:
            if not track:
                return self._clarify(goal, "Which clip (track) should I rename?")
            name = text.split(" to ", 1)[-1].strip() or text.split("name clip", 1)[-1].strip()
            if not name:
                return self._clarify(goal, "What should I name the clip?")
            return self._single_step_plan(goal, "set_clip_name", {"track": track, "clip_index": 0, "name": name}, 0.89)
        if "color clip" in text or "colour clip" in text:
            if not track:
                return self._clarify(goal, "Which clip (track) should I color?")
            color_match = re.search(r"(#[0-9a-fA-F]{6})", text)
            if not color_match:
                return self._clarify(goal, "Which hex color (like #ff0000) should I use?")
            return self._single_step_plan(
                goal, "set_clip_color", {"track": track, "clip_index": 0, "color": color_match.group(1).lower()}, 0.87
            )
        if "clone track" in text or "duplicate track" in text or "copy this track" in text:
            target = self._extract_track_name(text, state)
            args: Dict[str, Any] = {}
            if target:
                args["track"] = target
            return self._single_step_plan(goal, "clone_track", args, 0.9)
        if "delete track" in text or "remove track" in text or (("delete" in text or "remove" in text) and "track" in text):
            target = self._extract_track_name(text, state)
            if not target:
                return self._clarify(goal, "Which track should I delete?")
            return self._single_step_plan(
                goal, "delete_track", {"track": target}, 0.9, risk="destructive", requires_snapshot=True
            )
        if "move track" in text:
            target = self._extract_track_name(text, state)
            index = self._extract_int(text)
            if not target or index is None:
                return self._clarify(goal, "Which track, and to which position (index), should I move?")
            return self._single_step_plan(goal, "move_track", {"track": target, "index": index}, 0.88)
        if "track volume" in text or "turn down the" in text or "turn up the" in text or "lower volume" in text or "raise volume" in text or "make the lead louder" in text or "make the bass quieter" in text:
            target = self._extract_track_name(text, state)
            if not target:
                return self._clarify(goal, "Which track's volume should I change?")
            value = self._extract_percent(text)
            if value is None:
                value = 0.5 if any(word in text for word in ("down", "lower", "quieter")) else 0.8
            return self._single_step_plan(goal, "set_track_volume", {"track": target, "value": value}, 0.9)
        if "pan left" in text or "pan right" in text or "center the track" in text or "pan the" in text or "spread" in text:
            target = self._extract_track_name(text, state)
            if not target:
                return self._clarify(goal, "Which track should I pan?")
            if "center" in text:
                value = 0.5
            elif "left" in text:
                value = 0.0
            else:
                value = 1.0
            return self._single_step_plan(goal, "set_track_pan", {"track": target, "value": value}, 0.9)
        if "pitch up" in text or "pitch down" in text or "transpose track" in text or "detune" in text:
            target = self._extract_track_name(text, state)
            if not target:
                return self._clarify(goal, "Which track should I transpose?")
            semitones = self._extract_int(text)
            if semitones is None:
                semitones = 12 if "up" in text else -12
            return self._single_step_plan(goal, "set_track_pitch", {"track": target, "value": semitones}, 0.89)
        if "key range" in text:
            target = self._extract_track_name(text, state)
            if not target:
                return self._clarify(goal, "Which track should I set the key range on?")
            keys = re.findall(r"\b(\d{1,3})\b", text)
            if len(keys) < 2:
                return self._clarify(goal, "Give me first_key and last_key MIDI numbers for the range.")
            return self._single_step_plan(
                goal, "set_track_key_range", {"track": target, "first_key": int(keys[0]), "last_key": int(keys[1])}, 0.87
            )
        if "base note" in text:
            target = self._extract_track_name(text, state)
            key = self._extract_int(text)
            if not target or key is None:
                return self._clarify(goal, "Which track and base note (MIDI key) should I set?")
            return self._single_step_plan(goal, "set_track_base_note", {"track": target, "key": key}, 0.88)
        return None

    def _plan_notes(self, goal: str, text: str, state: Dict[str, Any]) -> Optional[PlannerOutput]:
        if (
            "clear clip" in text
            or "delete all notes" in text
            or "empty the pattern" in text
            or "clear the pattern" in text
            or ("clear" in text and "clip" in text)
        ):
            track = self._extract_track_name(text, state)
            args: Dict[str, Any] = {}
            if track:
                args["track"] = track
            return self._single_step_plan(
                goal, "clear_clip", args, 0.92, risk="destructive", requires_snapshot=True
            )
        if "quantize" in text or "snap notes" in text or "fix timing" in text:
            resolution = self._extract_int(text)
            if resolution is None or not (1 <= resolution <= 192):
                resolution = 16
            track = self._extract_track_name(text, state)
            args: Dict[str, Any] = {"resolution": resolution}
            if track:
                args["track"] = track
            return self._single_step_plan(goal, "quantize_clip", args, 0.91)
        if "humanize" in text or "less robotic" in text or "swing feel" in text or "loosen timing" in text:
            amount = self._extract_percent(text)
            if amount is None:
                amount = 0.5
            track = self._extract_track_name(text, state)
            args: Dict[str, Any] = {"amount": amount}
            if track:
                args["track"] = track
            return self._single_step_plan(goal, "humanize_clip", args, 0.9)
        if "split notes" in text:
            track = self._extract_track_name(text, state)
            at_tick = self._bar_tick(text, default=2 * TICKS_PER_BAR)
            args: Dict[str, Any] = {"at_tick": at_tick}
            if track:
                args["track"] = track
            return self._single_step_plan(goal, "split_clip_notes", args, 0.87)
        if "velocity scale" in text or "make the notes louder" in text or "softer" in text and "notes" in text:
            scale = self._extract_float(text)
            if scale is None:
                scale = 1.2 if "louder" in text else 0.8
            track = self._extract_track_name(text, state)
            args: Dict[str, Any] = {"scale": min(2.0, max(0.0, scale))}
            if track:
                args["track"] = track
            return self._single_step_plan(goal, "set_clip_velocity_scale", args, 0.88)
        if "edit notes" in text:
            return self._clarify(goal, "Describe the note edits (key, position, and new values) I should apply.")
        if "remove notes" in text or "delete notes" in text:
            return self._clarify(goal, "Which notes (keys and positions) should I remove?")
        if "add chord" in text or "chord" in text and any(
            word in text for word in ("major", "minor", "maj7", "min7", "dim", "sus2", "sus4", "power", "seventh")
        ):
            parsed = self._parse_chord(text)
            if parsed is None:
                return self._clarify(goal, "Which chord (for example C major or A minor) should I add?")
            root, chord = parsed
            track = self._extract_track_name(text, state)
            args: Dict[str, Any] = {"root": root, "chord": chord}
            if track:
                args["track"] = track
            return self._single_step_plan(goal, "add_chord", args, 0.91)
        if "arpeggio" in text or "arpeggiate" in text or "broken chord" in text:
            parsed = self._parse_chord(text)
            if parsed is None:
                return self._clarify(goal, "Which chord should the arpeggio be built from (for example C major)?")
            root, chord = parsed
            direction = "up"
            for candidate in ("down", "updown", "random"):
                if candidate in text:
                    direction = candidate
                    break
            steps = self._extract_int(text)
            if steps is None:
                steps = 8
            track = self._extract_track_name(text, state)
            args: Dict[str, Any] = {"root": root, "chord": chord, "direction": direction, "steps": steps}
            if track:
                args["track"] = track
            return self._single_step_plan(goal, "add_arpeggio", args, 0.9)
        if "add notes" in text or "add melody" in text or "write a bassline" in text or "add a riff" in text or "play these notes" in text:
            tokens = self._parse_note_tokens(text)
            if not tokens:
                return self._clarify(goal, "Which notes should I add (for example C4 E4 G4)?")
            notes = self._notes_from_tokens(tokens)
            track = self._extract_track_name(text, state)
            args: Dict[str, Any] = {"notes": notes}
            if track:
                args["track"] = track
            return self._single_step_plan(goal, "add_notes", args, 0.9)
        return None

    def _plan_patterns(self, goal: str, text: str, state: Dict[str, Any]) -> Optional[PlannerOutput]:
        if "add rhythm" in text or "drum pattern" in text or "four on the floor" in text or "add 808" in text:
            drum = self._extract_drum(text)
            if drum is None:
                return self._clarify(goal, "Which drum should the rhythm use (kick, snare, hihat, crash, or ride)?")
            rhythm = DEFAULT_RHYTHMS[drum]
            if "four on the floor" in text:
                rhythm = DEFAULT_RHYTHMS["kick"]
            return self._single_step_plan(goal, "add_rhythm", {"drum": drum, "pattern": rhythm}, 0.92)
        if any(drum in text for drum in DRUM_TYPES) and any(
            word in text for word in ("add", "put", "program", "make")
        ) and "duck" not in text and "controller" not in text:
            drum = self._extract_drum(text)
            return self._single_step_plan(
                goal, "add_rhythm", {"drum": drum, "pattern": DEFAULT_RHYTHMS[drum]}, 0.9
            )
        if (
            "new pattern" in text
            or "add pattern" in text
            or "create pattern" in text
            or ("pattern" in text and any(verb in text for verb in ("create", "add", "new")))
        ):
            name_match = re.search(r"(?:create|add|new)\s+(?:a\s+)?([a-z0-9 _-]+?)\s+pattern", text)
            if not name_match:
                name_match = re.search(r"(?:pattern|new pattern|add pattern|create pattern)\s+([a-z0-9 _-]+)$", text)
            args: Dict[str, Any] = {}
            if name_match and name_match.group(1).strip() not in ("named", "pattern"):
                args["name"] = name_match.group(1).strip()
            return self._single_step_plan(goal, "create_pattern", args, 0.91)
        if "select pattern" in text or "switch to pattern" in text or "edit pattern" in text:
            pattern = text.split("pattern", 1)[-1].strip() or None
            if not pattern:
                return self._clarify(goal, "Which pattern should I select?")
            return self._single_step_plan(goal, "select_pattern", {"pattern": pattern}, 0.9)
        if "clone pattern" in text:
            pattern = text.split("clone pattern", 1)[-1].strip().split(" to ", 1)[0].strip() or None
            if not pattern:
                return self._clarify(goal, "Which pattern should I clone?")
            name_match = re.search(r" to\s+([a-z0-9 _-]+)$", text)
            args: Dict[str, Any] = {"pattern": pattern}
            if name_match:
                args["name"] = name_match.group(1).strip()
            return self._single_step_plan(goal, "clone_pattern", args, 0.88)
        if "set steps" in text or "program steps" in text or "step sequence" in text:
            steps = [int(tok) for tok in re.findall(r"\b\d{1,2}\b", text)]
            if not steps:
                return self._clarify(goal, "Which step numbers (0-15) should the pattern play?")
            return self._single_step_plan(goal, "set_steps", {"steps": steps}, 0.89)
        if "step velocity" in text or "accent the" in text:
            step = self._extract_int(text)
            if step is None:
                return self._clarify(goal, "Which step should I accent, and how loud (1-127)?")
            velocity = next((int(tok) for tok in re.findall(r"\b\d{1,3}\b", text) if 1 <= int(tok) <= 127), 120)
            return self._single_step_plan(goal, "set_step_velocity", {"step": step, "velocity": velocity}, 0.88)
        if "steps per bar" in text:
            steps = self._extract_int(text)
            if steps is None:
                return self._clarify(goal, "How many steps per bar should the pattern use?")
            return self._single_step_plan(goal, "set_steps_per_bar", {"steps": steps}, 0.87)
        return None

    def _plan_samples(self, goal: str, text: str, state: Dict[str, Any]) -> Optional[PlannerOutput]:
        track = self._extract_track_name(text, state)
        if (
            "loop the sample" in text
            or "ping pong" in text
            or "sample loop" in text
            or "stop sample looping" in text
            or ("loop" in text and "sample" in text)
        ):
            if "pingpong" in text or "ping pong" in text:
                mode = "pingpong"
            elif "off" in text or "stop" in text:
                mode = "off"
            else:
                mode = "on"
            args: Dict[str, Any] = {"mode": mode}
            if track:
                args["track"] = track
            return self._single_step_plan(goal, "set_sample_loop", args, 0.9)
        if "pitch the sample" in text or "sample pitch" in text or "make sample higher" in text or "make sample lower" in text:
            semitones = self._extract_int(text)
            if semitones is None:
                semitones = 12 if "higher" in text else -12
            args: Dict[str, Any] = {"semitones": semitones}
            if track:
                args["track"] = track
            return self._single_step_plan(goal, "set_sample_pitch", args, 0.9)
        if "sample amp" in text or "sample volume" in text or "amplification" in text:
            value = self._extract_percent(text)
            if value is None:
                value = 0.8
            args: Dict[str, Any] = {"value": value}
            if track:
                args["track"] = track
            return self._single_step_plan(goal, "set_sample_amp", args, 0.88)
        if "sample range" in text or "trim sample" in text or "sample region" in text:
            frames = [int(tok) for tok in re.findall(r"\b\d{3,}\b", text)]
            if len(frames) < 2:
                return self._clarify(goal, "Give me start_frame and length_frames for the sample region.")
            args: Dict[str, Any] = {"start_frame": frames[0], "length_frames": frames[1]}
            if track:
                args["track"] = track
            return self._single_step_plan(goal, "set_sample_range", args, 0.87)
        return None

    def _plan_instruments(self, goal: str, text: str, discovery: DiscoveryIndex, state: Dict[str, Any]) -> Optional[PlannerOutput]:
        if (
            "load preset" in text
            or "use preset" in text
            or "preset file" in text
            or ".xiz" in text
            or ".sf2" in text
            or ("preset" in text and any(verb in text for verb in ("load", "use", "apply")))
        ):
            preset_query = text
            for prefix in ("load preset", "use preset", "apply preset", "load the", "use the", "apply the", "load ", "use ", "apply "):
                if preset_query.startswith(prefix):
                    preset_query = preset_query[len(prefix):]
                    break
            preset_query = preset_query.replace("preset", "").strip()
            resolved = discovery.resolve_preset(preset_query)
            if not resolved:
                return self._clarify(
                    goal,
                    f"I could not resolve preset '{preset_query}'. Which preset file (xiz/sf2/pat/xp) should I load?",
                )
            return PlannerOutput(
                goal=goal,
                mode="plan",
                needs_clarification=False,
                subgoals=[
                    Subgoal(
                        id="sg1",
                        title="Resolve preset asset",
                        steps=[
                            PlanStep(
                                action="resolve_preset",
                                args={"query": preset_query},
                                confidence=0.84,
                                risk="safe",
                                requires_snapshot=False,
                            )
                        ],
                    ),
                    Subgoal(
                        id="sg2",
                        title="Load instrument preset",
                        steps=[
                            PlanStep(
                                action="load_instrument_preset",
                                args={"path": resolved.get("path", preset_query)},
                                confidence=0.9,
                                risk="safe",
                                requires_snapshot=True,
                            )
                        ],
                    ),
                ],
            )
        if "vst program" in text:
            program = self._extract_int(text)
            track = self._extract_track_name(text, state)
            if program is None or not track:
                return self._clarify(goal, "Which VST track and program number should I set?")
            return self._single_step_plan(goal, "set_vst_program", {"track": track, "program": program}, 0.88)
        if "vst param" in text or "vst parameter" in text:
            track = self._extract_track_name(text, state)
            numbers = [int(tok) for tok in re.findall(r"\b\d{1,4}\b", text)]
            if not track or len(numbers) < 2:
                return self._clarify(goal, "Which VST track, param index, and value should I set?")
            return self._single_step_plan(
                goal, "set_vst_param", {"track": track, "param": numbers[0], "value": numbers[1]}, 0.87
            )
        if "sf2 patch" in text or "soundfont patch" in text:
            track = self._extract_track_name(text, state)
            patch = self._extract_int(text)
            if patch is None or not track:
                return self._clarify(goal, "Which track and SF2 patch number should I set?")
            return self._single_step_plan(goal, "set_sf2_patch", {"track": track, "patch": patch}, 0.89)
        if "describe instrument" in text:
            track = self._extract_track_name(text, state)
            args: Dict[str, Any] = {}
            if track:
                args["track"] = track
            return self._single_step_plan(goal, "describe_instrument", args, 0.92)
        if (
            "set cutoff" in text
            or "set parameter" in text
            or "turn up resonance" in text
            or "tweak the knob" in text
            or ("set" in text and "cutoff" in text)
        ):
            track = self._extract_track_name(text, state)
            param = "cutoff"
            if "resonance" in text:
                param = "resonance"
            value = self._extract_percent(text)
            if value is None:
                value = 0.6
            args: Dict[str, Any] = {"param": param, "value": value}
            if track:
                args["track"] = track
            return self._single_step_plan(goal, "set_param", args, 0.88)
        return None

    def _plan_sound(self, goal: str, text: str, state: Dict[str, Any]) -> Optional[PlannerOutput]:
        track = self._extract_track_name(text, state)
        if "envelope" in text or "attack" in text or "sustain" in text or "decay" in text or "release" in text or "plucky" in text or "punchy" in text:
            args: Dict[str, Any] = {}
            if "attack" in text:
                args["attack"] = 0.3 if any(word in text for word in ("long", "soft", "slow")) else 0.05
            if "decay" in text:
                args["decay"] = 0.2
            if "sustain" in text:
                args["sustain"] = 0.7 if "high" in text or "long" in text else 0.4
            if "release" in text:
                args["release"] = 0.3 if any(word in text for word in ("long", "soft")) else 0.08
            if "plucky" in text or "punchy" in text:
                args.update({"attack": 0.01, "decay": 0.15, "sustain": 0.2, "release": 0.05})
            if not args:
                args["attack"] = 0.1
            if track:
                args["track"] = track
            return self._single_step_plan(goal, "set_envelope", args, 0.9)
        if "filter" in text or "lowpass" in text or "highpass" in text or "bandpass" in text or "notch" in text or "moog" in text or "cutoff sweep" in text:
            args: Dict[str, Any] = {}
            filter_type = None
            for candidate in ("lowpass", "highpass", "bandpass", "notch", "allpass", "moog", "2xlowpass"):
                if candidate in text:
                    filter_type = candidate
                    break
            if filter_type:
                args["type"] = filter_type
            if "cutoff" in text or "sweep" in text or "open the filter" in text:
                cutoff = self._extract_percent(text)
                args["cutoff"] = cutoff if cutoff is not None else 0.7
            if "resonance" in text:
                reso = self._extract_percent(text)
                args["resonance"] = reso if reso is not None else 0.4
            if "enable" in text or "on" in text:
                args["enabled"] = True
            if not args:
                args["type"] = "lowpass"
            if track:
                args["track"] = track
            return self._single_step_plan(goal, "set_filter", args, 0.9)
        if (
            ("lfo" in text or "wobble" in text or "tremolo" in text or "vibrato" in text or "modulate cutoff" in text)
            and "controller" not in text
        ):
            args: Dict[str, Any] = {}
            if "cutoff" in text:
                args["target"] = "cutoff"
            elif "volume" in text:
                args["target"] = "volume"
            speed = self._extract_float(text)
            if speed is not None:
                args["speed"] = speed
            if "amount" in text:
                amount = self._extract_percent(text)
                if amount is not None:
                    args["amount"] = amount
            if not args:
                args["amount"] = 0.3
                args["speed"] = 0.5
            if track:
                args["track"] = track
            return self._single_step_plan(goal, "set_lfo", args, 0.89)
        if "arp" in text or "arpeggio settings" in text:
            args: Dict[str, Any] = {}
            if "off" in text or "disable" in text:
                args["enabled"] = False
            else:
                args["enabled"] = True
            if "range" in text:
                args["range"] = 2
            if track:
                args["track"] = track
            return self._single_step_plan(goal, "set_arp", args, 0.88)
        if "note stacking" in text or "note stack" in text:
            args: Dict[str, Any] = {}
            if "off" in text or "disable" in text:
                args["enabled"] = False
            else:
                args["enabled"] = True
            if track:
                args["track"] = track
            return self._single_step_plan(goal, "set_note_stacking", args, 0.87)
        return None

    def _plan_effects(self, goal: str, text: str, discovery: DiscoveryIndex) -> Optional[PlannerOutput]:
        if "add effect" in text:
            effect_name = text.split("add effect", 1)[-1].strip()
            resolved = discovery.resolve_plugin(effect_name, "effect")
            if not resolved:
                return self._clarify(goal, f"Which effect plugin should I use for '{effect_name}'?")
            step = PlanStep(
                action="add_effect",
                args={"effect": resolved["canonical_name"]},
                confidence=0.86,
                risk="safe",
                requires_snapshot=True,
            )
            return PlannerOutput(
                goal=goal,
                mode="plan",
                needs_clarification=False,
                subgoals=[Subgoal(id="sg1", title="Add effect", steps=[step])],
            )
        if "remove effect" in text:
            effect_name = text.split("remove effect", 1)[-1].strip()
            step = PlanStep(
                action="remove_effect",
                args={"effect": effect_name},
                confidence=0.78,
                risk="destructive",
                requires_snapshot=True,
            )
            return PlannerOutput(
                goal=goal,
                mode="plan",
                needs_clarification=False,
                subgoals=[Subgoal(id="sg1", title="Remove effect", steps=[step])],
            )
        if "effect param" in text or "wet dry" in text or "reverb amount" in text or "delay time" in text or "compressor ratio" in text:
            effect = text.split("effect", 1)[-1].strip() if "effect" in text else None
            param = None
            for candidate in ("wet dry", "wet/dry", "amount", "time", "ratio", "cutoff", "mix"):
                if candidate in text:
                    param = candidate.replace("/", "_")
                    break
            value = self._extract_percent(text)
            if value is None:
                value = 0.3
            args: Dict[str, Any] = {"param": param or "wetdry", "value": value}
            if effect:
                args["effect"] = effect
            return self._single_step_plan(goal, "set_effect_param", args, 0.88)
        if "move effect" in text:
            effect = text.split("effect", 1)[-1].strip() if "effect" in text else None
            direction = "up" if any(word in text for word in ("up", "earlier")) else "down"
            args: Dict[str, Any] = {"direction": direction}
            if effect:
                args["effect"] = effect
            return self._single_step_plan(goal, "move_effect", args, 0.87)
        if "enable effect" in text or "disable effect" in text or "bypass effect" in text:
            effect = text.split("effect", 1)[-1].strip() if "effect" in text else None
            enabled = "enable" in text and "disable" not in text
            args: Dict[str, Any] = {"enabled": enabled}
            if effect:
                args["effect"] = effect
            return self._single_step_plan(goal, "set_effect_enabled", args, 0.88)
        return None

    def _plan_mixer(self, goal: str, text: str, state: Dict[str, Any]) -> Optional[PlannerOutput]:
        if (
            "create send" in text
            or "send to channel" in text
            or "route to reverb bus" in text
            or "sidechain through mixer" in text
            or ("send" in text and any(verb in text for verb in ("create", "add")))
        ):
            from_match = re.search(r"(?:from|on)\s+([a-z0-9 _-]+?)\s+(?:to|into)\s+([a-z0-9 _-]+)", text)
            if not from_match:
                return self._clarify(goal, "Which channels should the send go from and to (for example from Bass to Reverb)?")
            amount = self._extract_percent(text)
            args: Dict[str, Any] = {"from": from_match.group(1).strip(), "to": from_match.group(2).strip()}
            if amount is not None:
                args["amount"] = amount
            return self._single_step_plan(goal, "create_send", args, 0.89)
        if "delete send" in text:
            from_match = re.search(r"from\s+([a-z0-9 _-]+?)\s+to\s+([a-z0-9 _-]+)", text)
            if not from_match:
                return self._clarify(goal, "Which send (from and to channels) should I delete?")
            return self._single_step_plan(
                goal, "delete_send", {"from": from_match.group(1).strip(), "to": from_match.group(2).strip()}, 0.88
            )
        if "send amount" in text:
            from_match = re.search(r"(?:from|on)\s+([a-z0-9 _-]+?)\s+(?:to|into)\s+([a-z0-9 _-]+)", text)
            amount = self._extract_percent(text)
            if not from_match or amount is None:
                return self._clarify(goal, "Give me the send (from and to channels) and the amount (0-100%).")
            return self._single_step_plan(
                goal,
                "set_send_amount",
                {"from": from_match.group(1).strip(), "to": from_match.group(2).strip(), "amount": amount},
                0.88,
            )
        if "delete channel" in text:
            channel = text.split("delete channel", 1)[-1].strip()
            if not channel:
                return self._clarify(goal, "Which mixer channel should I delete?")
            return self._single_step_plan(
                goal, "delete_channel", {"channel": channel}, 0.89, risk="destructive", requires_snapshot=True
            )
        if "rename channel" in text:
            channel = text.split("rename channel", 1)[-1].split(" to ", 1)[0].strip()
            new_name = text.split(" to ", 1)[-1].strip()
            if not channel or not new_name:
                return self._clarify(goal, "Which mixer channel should I rename, and to what?")
            return self._single_step_plan(goal, "rename_channel", {"channel": channel, "name": new_name}, 0.89)
        if "move channel" in text:
            channel = text.split("move channel", 1)[-1].split(" ", 1)[-1].strip() or None
            direction = "left" if "left" in text else "right"
            args: Dict[str, Any] = {"direction": direction}
            if channel:
                args["channel"] = channel
            return self._single_step_plan(goal, "move_channel", args, 0.87)
        if "channel volume" in text or "lower the channel" in text:
            channel = text.split("channel", 1)[-1].strip() or None
            value = self._extract_percent(text)
            if value is None:
                value = 0.5 if any(word in text for word in ("down", "lower")) else 0.8
            args: Dict[str, Any] = {"value": value}
            if channel:
                args["channel"] = channel
            return self._single_step_plan(goal, "set_channel_volume", args, 0.88)
        if "mute channel" in text or "solo channel" in text:
            channel = text.split("channel", 1)[-1].strip() or None
            is_solo = "solo" in text
            state_value = True
            if "unmute" in text or "unsolo" in text:
                state_value = False
            args: Dict[str, Any] = {("solo" if is_solo else "mute"): state_value}
            if channel:
                args["channel"] = channel
            return self._single_step_plan(
                goal, "set_channel_solo" if is_solo else "set_channel_mute", args, 0.89
            )
        if "effect to channel" in text or "effect on channel" in text:
            effect = text.split("effect", 1)[-1].strip().split(" on ", 1)[0].split(" to ", 1)[0].strip()
            channel = self._extract_track_name(text, state)
            if not effect or not channel:
                return self._clarify(goal, "Which effect should I add, and to which mixer channel?")
            return self._single_step_plan(
                goal, "add_channel_effect", {"channel": channel, "effect": effect}, 0.88
            )
        if "remove effect from channel" in text:
            effect = text.split("effect", 1)[-1].strip()
            channel = self._extract_track_name(text, state)
            if not effect or not channel:
                return self._clarify(goal, "Which effect should I remove, and from which mixer channel?")
            return self._single_step_plan(
                goal, "remove_channel_effect", {"channel": channel, "effect": effect}, 0.87
            )
        if (
            "route track to channel" in text
            or "route to mixer channel" in text
            or "send track to channel" in text
            or ("route" in text and "channel" in text)
        ):
            track = self._extract_track_name(text, state)
            channel_match = re.search(r"(?:channel|mixer channel)\s+(\d+|[\w-]+)", text)
            channel = channel_match.group(1) if channel_match else None
            if not track or not channel:
                return self._clarify(goal, "Which track should I route, and to which mixer channel?")
            return self._single_step_plan(goal, "route_track_to_channel", {"track": track, "channel": channel}, 0.89)
        if "add mixer channel" in text or "new fx channel" in text or "create channel" in text or "add a bus" in text:
            name_match = re.search(r"(?:channel|bus)\s+([a-z0-9 _-]+)$", text)
            args: Dict[str, Any] = {}
            if name_match:
                args["name"] = name_match.group(1).strip()
            return self._single_step_plan(goal, "create_channel", args, 0.91)
        return None

    def _plan_automation(self, goal: str, text: str, state: Dict[str, Any]) -> Optional[PlannerOutput]:
        track = self._extract_track_name(text, state)
        if "global automation" in text or "automate master volume" in text or "song wide automation" in text or "song wide" in text and "automate" in text:
            address = "song.master.volume"
            if "tempo" in text:
                address = "song.tempo"
            ticks, values = self._automation_curve(text)
            return self._single_step_plan(
                goal,
                "global_automate",
                {"address": address, "ticks": ticks, "values": values},
                0.9,
                risk="destructive",
                requires_snapshot=True,
            )
        if "create automation" in text or "automation track" in text or "add automation" in text:
            address = self._automation_address(text, track)
            args: Dict[str, Any] = {}
            if address:
                args["address"] = address
            name_match = re.search(r"(?:automation|track)\s+([a-z0-9 _-]+)$", text)
            if name_match:
                args["name"] = name_match.group(1).strip()
            return self._single_step_plan(goal, "create_automation", args, 0.9)
        if "automate" in text or "filter sweep" in text or "fade in volume" in text or "fade out" in text or "volume automation" in text or "tempo ramp" in text or "draw automation" in text:
            address = self._automation_address(text, track)
            if not address:
                if any(word in text for word in ("cutoff", "volume", "pan", "pitch", "resonance", "filter")):
                    return self._clarify(goal, "Which track should I automate this parameter on?")
                return self._clarify(
                    goal,
                    "Which parameter should I automate (for example cutoff, volume, pan, or tempo)?",
                )
            ticks, values = self._automation_curve(text)
            args: Dict[str, Any] = {"address": address, "ticks": ticks, "values": values}
            return self._single_step_plan(goal, "automate", args, 0.89)
        if "automation node" in text:
            clip_match = re.search(r"(?:clip|on)\s+([a-z0-9:_-]+)", text)
            tick = self._bar_tick(text, default=0)
            value = self._extract_percent(text)
            if clip_match is None or value is None:
                return self._clarify(goal, "Give me the automation clip, tick (or bar), and value for the node.")
            return self._single_step_plan(
                goal,
                "set_automation_node",
                {"clip": clip_match.group(1).strip(), "tick": tick, "value": value},
                0.86,
            )
        if "remove automation node" in text:
            clip_match = re.search(r"(?:clip|on)\s+([a-z0-9:_-]+)", text)
            tick = self._bar_tick(text, default=0)
            if clip_match is None:
                return self._clarify(goal, "Which automation clip should I remove the node from?")
            return self._single_step_plan(
                goal, "remove_automation_node", {"clip": clip_match.group(1).strip(), "tick": tick}, 0.86
            )
        if "tension" in text and "automation" in text:
            clip_match = re.search(r"(?:clip|on)\s+([a-z0-9:_-]+)", text)
            tension = self._extract_float(text)
            if clip_match is None or tension is None:
                return self._clarify(goal, "Give me the automation clip and tension (-1 to 1) to set.")
            return self._single_step_plan(
                goal,
                "set_automation_tension",
                {"clip": clip_match.group(1).strip(), "tension": max(-1.0, min(1.0, tension))},
                0.85,
            )
        if "progression" in text and "automation" in text:
            progression = "cubic" if "cubic" in text else "linear" if "linear" in text else "discrete"
            clip_match = re.search(r"(?:clip|on)\s+([a-z0-9:_-]+)", text)
            args: Dict[str, Any] = {"progression": progression}
            if clip_match:
                args["clip"] = clip_match.group(1).strip()
            return self._single_step_plan(goal, "set_automation_progression", args, 0.86)
        return None

    def _plan_controllers(self, goal: str, text: str, state: Dict[str, Any]) -> Optional[PlannerOutput]:
        if "disconnect controller" in text:
            address = None
            address_match = re.search(r"from\s+([a-z0-9:._-]+)", text)
            if address_match:
                address = address_match.group(1).strip()
            if not address:
                return self._clarify(goal, "From which address (for example track:Bass.volume) should I disconnect the controller?")
            return self._single_step_plan(
                goal, "disconnect_controller", {"address": address}, 0.88, risk="destructive", requires_snapshot=True
            )
        if (
            "connect controller" in text
            or "assign controller" in text
            or "link lfo" in text
            or "modulate with controller" in text
            or (("connect" in text or "assign" in text or "link" in text) and "controller" in text)
        ):
            controller = None
            name_match = re.search(r"(?:controller|the)\s+([a-z][a-z0-9 _-]*?)\s+(?:to|with)\s+", text)
            if name_match:
                candidate = name_match.group(1).strip()
                if not any(word in candidate.split() for word in ("and", "the", "it", "to", "with", "of")):
                    controller = candidate
            address_match = re.search(r"to\s+(?:the\s+)?([a-z0-9:._-]+)$", text)
            address = address_match.group(1).strip() if address_match else None
            track = self._extract_track_name(text, state)
            if not controller or not address:
                return self._clarify(goal, "Which controller should I connect, and to which address?")
            derived = self._automation_address(text, track)
            if derived:
                address = derived
            return self._single_step_plan(
                goal, "connect_controller", {"controller": controller, "address": address}, 0.89
            )
        if "set lfo controller" in text or "lfo controller" in text or "lfo speed" in text:
            controller = None
            name_match = re.search(r"(?:controller|the)\s+([a-z0-9 _-]+)", text)
            if name_match:
                controller = name_match.group(1).strip()
            args: Dict[str, Any] = {}
            if controller:
                args["controller"] = controller
            speed = self._extract_float(text)
            if speed is not None:
                args["speed"] = speed
            if not args:
                args["speed"] = 0.5
                args["amount"] = 0.3
            return self._single_step_plan(goal, "set_lfo_controller", args, 0.88)
        if (
            (
                "create controller" in text
                or "add lfo controller" in text
                or "peak controller" in text
                or "add a controller" in text
                or (
                    any(kind in text for kind in ("lfo controller", "peak controller", "midi controller"))
                    and any(verb in text for verb in ("add", "create", "new"))
                )
            )
            and "duck" not in text
            and "sidechain" not in text
        ):
            controller_type = "lfo"
            if "peak" in text or "duck" in text:
                controller_type = "peak"
            elif "midi" in text:
                controller_type = "midi"
            name_match = re.search(r"(?:controller|named)\s+([a-z0-9 _-]+)$", text)
            args: Dict[str, Any] = {"type": controller_type}
            if name_match and name_match.group(1).strip() not in ("controller",):
                args["name"] = name_match.group(1).strip()
            return self._single_step_plan(goal, "create_controller", args, 0.9)
        return None

    def _plan_render(self, goal: str, text: str) -> Optional[PlannerOutput]:
        if "render song" in text or "export song" in text or "render the project" in text or "export as" in text or "render to" in text:
            fmt = self._extract_format(text) or "wav"
            path_match = re.search(r"(?:to|as)\s+([^\s]+(?:\.\w+)?)", text)
            if path_match and "." in path_match.group(1):
                path = path_match.group(1).strip()
            else:
                path = f"export/song.{fmt}"
            return self._single_step_plan(
                goal,
                "render_song",
                {"path": path, "format": fmt, "sample_rate": 44100, "bit_depth": 16},
                0.92,
            )
        if (
            "render tracks" in text
            or "render stems" in text
            or "export stems" in text
            or "render tracks separately" in text
            or "export per track" in text
            or "export all tracks" in text
        ):
            dir_match = re.search(r"(?:to|into|in)\s+([^\s]+)", text)
            matched_dir = dir_match.group(1).strip() if dir_match else ""
            if not matched_dir or matched_dir in {"a", "an", "the", "my", "folder", "directory"}:
                matched_dir = "export/stems"
            args: Dict[str, Any] = {"dir": matched_dir, "prefix": "stem"}
            fmt = self._extract_format(text)
            if fmt:
                args["format"] = fmt
            return self._single_step_plan(goal, "render_tracks", args, 0.91)
        if ("render" in text and "preview" in text) or "render the loop" in text:
            begin = self._bar_tick(text)
            args: Dict[str, Any] = {}
            if begin is not None:
                args["begin_tick"] = begin
            match_end = re.search(r"to bar\s*(\d+)", text)
            if match_end:
                args["end_tick"] = int(match_end.group(1)) * TICKS_PER_BAR
            return self._single_step_plan(goal, "render_preview", args, 0.9)
        if "export midi" in text:
            path_match = re.search(r"(?:to|as)\s+([^\s]+)", text)
            path = path_match.group(1).strip() if path_match else "export/song.mid"
            return self._single_step_plan(goal, "export_midi", {"path": path}, 0.9)
        return None

    def _plan_misc(self, goal: str, text: str) -> Optional[PlannerOutput]:
        if "project notes" in text and ("set" in text or "write" in text or "add" in text):
            note = text.split("notes", 1)[-1].strip().lstrip(":").strip()
            if not note:
                return self._clarify(goal, "What should the project notes say?")
            return self._single_step_plan(goal, "set_project_notes", {"text": note}, 0.9)
        if "microtuner" in text:
            enabled = not any(word in text for word in ("off", "disable"))
            args: Dict[str, Any] = {"enabled": enabled}
            scale_match = re.search(r"scale\s+([a-z0-9 _-]+)", text)
            if scale_match:
                args["scale"] = scale_match.group(1).strip()
            return self._single_step_plan(goal, "set_microtuner", args, 0.88)
        return None

    def _plan_legacy(self, goal: str, text: str, discovery: DiscoveryIndex) -> Optional[PlannerOutput]:
        if text.startswith("show ") or text.startswith("open "):
            target = text.replace("show", "", 1).replace("open", "", 1).strip()
            return self._single_step_plan(goal, "open_tool", {"name": target}, 0.88)
        if "import midi" in text:
            path = goal.split("import midi", 1)[-1].strip()
            return self._single_step_plan(goal, "import_midi", {"path": path}, 0.92)
        if "import hydrogen" in text:
            path = goal.split("import hydrogen", 1)[-1].strip()
            return self._single_step_plan(goal, "import_hydrogen", {"path": path}, 0.92)
        if "import" in text and any(ext in text for ext in [".wav", ".mp3", ".aiff", ".flac", ".ogg"]):
            path = goal.split("import", 1)[-1].strip()
            return self._single_step_plan(goal, "import_audio", {"path": path}, 0.90)
        if text.startswith("create ") and "track" in text:
            track_type = "instrument"
            for candidate in ("sample", "instrument", "automation", "pattern"):
                if candidate in text:
                    track_type = candidate
                    break
            return self._single_step_plan(goal, "create_track", {"type": track_type}, 0.91)
        if "rename track" in text and " to " in text:
            left, right = text.split(" to ", 1)
            old_name = left.replace("rename track", "").strip()
            new_name = right.strip()
            return self._single_step_plan(goal, "rename_track", {"track": old_name, "new_name": new_name}, 0.89)
        if "load instrument" in text:
            plugin_query = text.split("load instrument", 1)[-1].strip()
            resolved = discovery.resolve_plugin(plugin_query, "instrument")
            if not resolved:
                return self._clarify(goal, f"I could not resolve '{plugin_query}'. Which instrument plugin should I load?")
            return PlannerOutput(
                goal=goal,
                mode="plan",
                needs_clarification=False,
                subgoals=[
                    Subgoal(
                        id="sg1",
                        title="Resolve instrument plugin",
                        steps=[
                            PlanStep(
                                action="resolve_plugin",
                                args={"query": plugin_query, "type": "instrument"},
                                confidence=0.84,
                                risk="safe",
                                requires_snapshot=False,
                            )
                        ],
                    ),
                    Subgoal(
                        id="sg2",
                        title="Load instrument",
                        steps=[
                            PlanStep(
                                action="load_instrument",
                                args={"plugin": resolved["canonical_name"]},
                                confidence=0.9,
                                risk="safe",
                                requires_snapshot=True,
                            )
                        ],
                    ),
                ],
            )
        if "load sample" in text or "add sample" in text:
            sample_query = text.replace("load sample", "").replace("add sample", "").strip()
            candidate_path = Path(sample_query).expanduser()
            if candidate_path.exists():
                return self._single_step_plan(
                    goal, "load_sample", {"sample_path": str(candidate_path)}, 0.9, requires_snapshot=True
                )
            resolved = discovery.resolve_sample(sample_query)
            if not resolved:
                return self._clarify(goal, f"I could not resolve sample '{sample_query}'. Which file should I use?")
            return PlannerOutput(
                goal=goal,
                mode="plan",
                needs_clarification=False,
                subgoals=[
                    Subgoal(
                        id="sg1",
                        title="Resolve sample asset",
                        steps=[
                            PlanStep(
                                action="resolve_sample",
                                args={"query": sample_query},
                                confidence=0.82,
                                risk="safe",
                                requires_snapshot=False,
                            )
                        ],
                    ),
                    Subgoal(
                        id="sg2",
                        title="Load sample",
                        steps=[
                            PlanStep(
                                action="load_sample",
                                args={"sample_path": resolved.get("path", sample_query)},
                                confidence=0.87,
                                risk="safe",
                                requires_snapshot=True,
                            )
                        ],
                    ),
                ],
            )
        if "mute" in text and "track" in text:
            target = text.split("mute", 1)[-1].replace("track", "").strip()
            step = PlanStep(
                action="mute_track",
                args={"track": target, "mute": True},
                confidence=0.83,
                risk="safe",
                requires_snapshot=False,
            )
            return self._compose_track_oriented_plan(goal, target, step)
        if "solo" in text and "track" in text:
            target = text.split("solo", 1)[-1].replace("track", "").strip()
            step = PlanStep(
                action="solo_track",
                args={"track": target, "solo": True},
                confidence=0.83,
                risk="safe",
                requires_snapshot=False,
            )
            return self._compose_track_oriented_plan(goal, target, step)
        if "undo" in text:
            return self._single_step_plan(goal, "undo_last_action", {}, 0.96)
        if "rollback" in text and "snapshot" in text:
            snapshot_id = text.split()[-1]
            return self._single_step_plan(goal, "rollback_to_snapshot", {"snapshot_id": snapshot_id}, 0.91)
        return None

    def _plan_single(
        self,
        goal: str,
        text: str,
        state: Dict[str, Any],
        discovery: DiscoveryIndex,
        fallback: bool = True,
    ) -> Optional[PlannerOutput]:
        handlers = [
            self._plan_project_transport,
            self._plan_arrangement,
            self._plan_notes,
            self._plan_patterns,
            self._plan_samples,
            lambda g, t, s: self._plan_instruments(g, t, discovery, s),
            self._plan_automation,
            self._plan_sound,
            lambda g, t, s: self._plan_effects(g, t, discovery),
            self._plan_mixer,
            self._plan_controllers,
            lambda g, t, s: self._plan_render(g, t),
            lambda g, t, s: self._plan_misc(g, t),
            lambda g, t, s: self._plan_legacy(g, t, discovery),
        ]
        for handler in handlers:
            plan = handler(goal, text, state)
            if plan is None:
                continue
            # A clarification means the intent was recognized but under-specified;
            # let the playbook layer (or the fallback probe) decide instead.
            if plan.mode == "clarify" and not fallback:
                return None
            return plan

        if not fallback:
            return None

        for rule in INTENT_RULES:
            if any(keyword in text for keyword in rule.keywords):
                return self._single_step_plan(
                    goal,
                    rule.action,
                    dict(rule.args or {}),
                    rule.confidence,
                    risk=rule.risk,
                    requires_snapshot=rule.requires_snapshot,
                )

        # Unknown goals: probe the project first (discovery-driven planning),
        # then ask the user to pin down the exact operation with state in hand.
        return PlannerOutput(
            goal=goal,
            mode="plan",
            needs_clarification=False,
            subgoals=[
                Subgoal(
                    id="sg1",
                    title="Probe current project state",
                    steps=[
                        PlanStep(
                            action="describe_song",
                            args={},
                            confidence=0.9,
                            risk="safe",
                            requires_snapshot=False,
                        )
                    ],
                ),
                Subgoal(
                    id="sg2",
                    title="Clarify exact operation",
                    steps=[
                        PlanStep(
                            action="guide_note",
                            args={
                                "note": (
                                    "I probed the project with describe_song. Tell me the exact typed operation "
                                    "to run (for example: add notes C4 E4 G4, automate the cutoff over 4 bars, "
                                    "create a send from Bass to Reverb, render song to wav)."
                                )
                            },
                            confidence=0.9,
                            risk="safe",
                            requires_snapshot=False,
                        )
                    ],
                ),
            ],
        )

    @staticmethod
    def _merge_plans(goal: str, plans: List[PlannerOutput]) -> PlannerOutput:
        subgoals: List[Subgoal] = []
        for plan in plans:
            for subgoal in plan.subgoals:
                subgoals.append(
                    Subgoal(
                        id=f"sg{len(subgoals)}",
                        title=subgoal.title,
                        steps=list(subgoal.steps),
                    )
                )
        return PlannerOutput(
            goal=goal,
            mode="plan",
            needs_clarification=False,
            subgoals=subgoals,
        )

    def plan(
        self,
        goal: str,
        *,
        state: Dict[str, Any],
        discovery: DiscoveryIndex,
        preferences: Optional[Dict[str, Any]] = None,
    ) -> PlannerOutput:
        text = self._norm(goal)
        if not text:
            return self._clarify(goal, "Please provide a request.")

        if any(token in text for token in ["list playbooks", "what is possible", "what can i do", "beginner help"]):
            catalog_lines = [f"- {pb['title']} ({pb['id']})" for pb in list_playbooks()]
            return PlannerOutput(
                goal=goal,
                mode="plan",
                needs_clarification=False,
                subgoals=[
                    Subgoal(
                        id="sg1",
                        title="Beginner playbook catalog",
                        steps=[
                            PlanStep(
                                action="guide_note",
                                args={"note": "Available beginner workflows:\n" + "\n".join(catalog_lines)},
                                confidence=0.99,
                                risk="safe",
                                requires_snapshot=False,
                            )
                        ],
                    )
                ],
            )

        if any(
            token in text
            for token in [
                "manual map",
                "manual feature",
                "full manual",
                "map features",
                "all features",
                "feature map",
            ]
        ):
            matches = find_feature_areas(text, limit=3)
            note = render_feature_catalog()
            if matches:
                detail_chunks = [render_feature_area(area) for area in matches]
                note = note + "\n\n" + "\n\n".join(detail_chunks)
            return PlannerOutput(
                goal=goal,
                mode="plan",
                needs_clarification=False,
                subgoals=[
                    Subgoal(
                        id="sg1",
                        title="Manual feature mapping",
                        steps=[
                            PlanStep(
                                action="guide_note",
                                args={"note": note},
                                confidence=0.99,
                                risk="safe",
                                requires_snapshot=False,
                            )
                        ],
                    )
                ],
            )

        if "manual" in text and ("how to" in text or "where" in text or "map" in text):
            matches = find_feature_areas(text, limit=2)
            if matches:
                note = "\n\n".join([render_feature_area(area) for area in matches])
                return PlannerOutput(
                    goal=goal,
                    mode="plan",
                    needs_clarification=False,
                    subgoals=[
                        Subgoal(
                            id="sg1",
                            title="Manual-guided capability map",
                            steps=[
                                PlanStep(
                                    action="guide_note",
                                    args={"note": note},
                                    confidence=0.97,
                                    risk="safe",
                                    requires_snapshot=False,
                                )
                            ],
                        )
                    ],
                )

        question = self._find_creative_question(text)
        if question:
            return self._clarify(goal, question)

        if self._is_ambiguous(text):
            return self._clarify(goal, "Can you specify one target track and one exact change to apply?")

        if text in EXACT_INTENTS:
            intent, action = EXACT_INTENTS[text]
            return self._single_step_plan(goal, action, {}, 0.97)

        # Compositional goals: "do A and do B" -> two subgoal chains.
        if " and " in text:
            left_text, right_text = text.split(" and ", 1)
            left_plan = self._plan_single(goal, left_text, state, discovery, fallback=False)
            right_plan = self._plan_single(goal, right_text, state, discovery, fallback=False)
            if (
                left_plan is not None
                and right_plan is not None
                and left_plan.subgoals
                and right_plan.subgoals
            ):
                return self._merge_plans(goal, [left_plan, right_plan])

        # Specific single-intent match first (precise args beat full workflows).
        direct = self._plan_single(goal, text, state, discovery, fallback=False)
        if direct is not None:
            return direct

        playbook = find_playbook_for_goal(text)
        if playbook is not None:
            subgoals: List[Subgoal] = [
                Subgoal(
                    id="sg0",
                    title="Compatibility check",
                    steps=[
                        PlanStep(
                            action="guide_note",
                            args={
                                "note": (
                                    f"Using manual playbook '{playbook.title}' from '{playbook.manual_section}'. "
                                    + playbook.caution
                                )
                            },
                            confidence=0.99,
                            risk="safe",
                            requires_snapshot=False,
                        )
                    ],
                )
            ]
            for idx, step in enumerate(playbook.steps, start=1):
                subgoals.append(
                    Subgoal(
                        id=f"sg{idx}",
                        title=step.title,
                        steps=[
                            PlanStep(
                                action=step.action,
                                args=step.args,
                                confidence=step.confidence,
                                risk=step.risk,  # type: ignore[arg-type]
                                requires_snapshot=step.requires_snapshot,
                            )
                        ],
                    )
                )
            return PlannerOutput(goal=goal, mode="plan", needs_clarification=False, subgoals=subgoals)

        # Unknown goals: discovery-driven fallback (describe probe first).
        return self._plan_single(goal, text, state, discovery, fallback=True)
