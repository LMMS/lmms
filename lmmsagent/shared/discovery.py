from __future__ import annotations

from dataclasses import dataclass, field
from pathlib import Path
from typing import Any, Dict, List, Optional

from .tool_client import ToolClient, ToolClientError

AUDIO_EXTS = {".wav", ".aiff", ".aif", ".flac", ".ogg", ".mp3"}
PRESET_EXTS = {".xiz", ".sf2", ".pat", ".xp"}
PROJECT_EXTS = {".mmp", ".mmpz"}

# Static render format knowledge (TOOL_CONTRACT_V2.md 2.12).
RENDER_FORMATS = [
    {"id": "render:wav", "type": "render_format", "display_name": "WAV", "canonical_name": "wav",
     "aliases": ["wave", "pcm"], "tags": ["render", "uncompressed", "44100", "48000", "16bit", "24bit", "32bit"]},
    {"id": "render:flac", "type": "render_format", "display_name": "FLAC", "canonical_name": "flac",
     "aliases": ["lossless"], "tags": ["render", "lossless", "44100", "48000", "16bit", "24bit"]},
    {"id": "render:ogg", "type": "render_format", "display_name": "OGG Vorbis", "canonical_name": "ogg",
     "aliases": ["vorbis"], "tags": ["render", "lossy", "bitrate"]},
    {"id": "render:mp3", "type": "render_format", "display_name": "MP3", "canonical_name": "mp3",
     "aliases": ["mpeg"], "tags": ["render", "lossy", "bitrate", "mp3"]},
]

DEFAULT_DATA_ROOT = str(Path(__file__).resolve().parents[2] / "data")


@dataclass
class Asset:
    id: str
    type: str
    display_name: str
    canonical_name: str
    aliases: List[str] = field(default_factory=list)
    tags: List[str] = field(default_factory=list)
    path: Optional[str] = None
    source: str = "unknown"

    def as_dict(self) -> Dict[str, Any]:
        return {
            "id": self.id,
            "type": self.type,
            "display_name": self.display_name,
            "canonical_name": self.canonical_name,
            "aliases": self.aliases,
            "tags": self.tags,
            "path": self.path,
            "source": self.source,
        }


class DiscoveryIndex:
    def __init__(
        self,
        tool_client: ToolClient,
        sample_roots: Optional[List[str]] = None,
        data_roots: Optional[List[str]] = None,
    ) -> None:
        self.tool_client = tool_client
        self.sample_roots = sample_roots or []
        self.data_roots = data_roots if data_roots is not None else [DEFAULT_DATA_ROOT]
        self._assets: List[Asset] = []
        for fmt in RENDER_FORMATS:
            self._add_asset(Asset(**fmt, source="contract"))

    @staticmethod
    def _norm(value: str) -> str:
        return "".join(ch for ch in value.lower() if ch.isalnum())

    @staticmethod
    def _tokens(value: str) -> List[str]:
        return [part for part in "".join(ch.lower() if ch.isalnum() else " " for ch in value).split() if part]

    def _score(self, query: str, asset: Asset, preferred_type: Optional[str] = None) -> float:
        q_norm = self._norm(query)
        if not q_norm:
            return 0.0

        names = [asset.canonical_name, asset.display_name, *asset.aliases]
        normalized_names = [self._norm(name) for name in names if name]

        if q_norm in normalized_names:
            base = 1.0
        elif any(name.startswith(q_norm) for name in normalized_names):
            base = 0.9
        elif any(q_norm in name for name in normalized_names):
            base = 0.75
        else:
            q_tokens = set(self._tokens(query))
            a_tokens = set(self._tokens(" ".join(names + asset.tags)))
            overlap = len(q_tokens & a_tokens)
            union = max(1, len(q_tokens | a_tokens))
            base = overlap / union

        if preferred_type and asset.type == preferred_type:
            base += 0.1
        if any(token in asset.tags for token in self._tokens(query)):
            base += 0.05
        return min(base, 1.0)

    def _add_asset(self, asset: Asset) -> None:
        self._assets.append(asset)

    def index_plugins(self) -> Dict[str, int]:
        managed = {"instrument_plugin", "effect_plugin", "tool_window"}
        self._assets = [a for a in self._assets if a.type not in managed]
        instruments = self._safe_list("list_instruments", "installed")
        effects = self._safe_list("list_effects", "installed")
        tools = self._safe_list("list_tool_windows", "tools")

        for item in instruments:
            name = item.get("name", "")
            display_name = item.get("display_name", name)
            self._add_asset(
                Asset(
                    id=f"plugin:instrument:{name}",
                    type="instrument_plugin",
                    display_name=display_name,
                    canonical_name=name,
                    aliases=[display_name],
                    tags=["instrument", "plugin"],
                    source="plugin",
                )
            )

        for item in effects:
            name = item.get("name", "")
            display_name = item.get("display_name", name)
            self._add_asset(
                Asset(
                    id=f"plugin:effect:{name}",
                    type="effect_plugin",
                    display_name=display_name,
                    canonical_name=name,
                    aliases=[display_name],
                    tags=["effect", "plugin"],
                    source="plugin",
                )
            )

        for item in tools:
            name = item.get("name", "")
            display_name = item.get("display_name", name)
            self._add_asset(
                Asset(
                    id=f"tool:{name}",
                    type="tool_window",
                    display_name=display_name,
                    canonical_name=name,
                    aliases=[display_name],
                    tags=["tool", "window"],
                    source="plugin",
                )
            )

        windows = self._safe_list("list_tool_windows", "windows")
        for window_name in windows:
            self._add_asset(
                Asset(
                    id=f"window:{self._norm(window_name)}",
                    type="tool_window",
                    display_name=window_name,
                    canonical_name=window_name,
                    aliases=[],
                    tags=["window"],
                    source="factory",
                )
            )

        return {
            "instruments": len(instruments),
            "effects": len(effects),
            "tool_windows": len(windows),
        }

    def index_samples(self, project_path: Optional[str] = None) -> Dict[str, int]:
        self._assets = [a for a in self._assets if a.type != "sample"]

        added = 0
        project_audio = self._safe_list("search_project_audio", "matches", args={"query": ""})
        for idx, item in enumerate(project_audio):
            sample_path = item.get("sample_path")
            if not sample_path:
                continue
            sample_name = Path(sample_path).name
            self._add_asset(
                Asset(
                    id=f"sample:project:{idx}",
                    type="sample",
                    display_name=sample_name,
                    canonical_name=sample_name,
                    aliases=[item.get("track_name", "")],
                    tags=["project", "audio"],
                    path=sample_path,
                    source="project",
                )
            )
            added += 1

        roots = list(self.sample_roots)
        if project_path:
            roots.append(str(Path(project_path).parent))
        roots.append(str(Path.home() / "Downloads"))

        seen_paths = {asset.path for asset in self._assets if asset.path}
        for root in roots:
            if not root:
                continue
            root_path = Path(root).expanduser()
            if not root_path.exists() or not root_path.is_dir():
                continue
            for path in root_path.rglob("*"):
                if not path.is_file() or path.suffix.lower() not in AUDIO_EXTS:
                    continue
                path_str = str(path)
                if path_str in seen_paths:
                    continue
                seen_paths.add(path_str)
                self._add_asset(
                    Asset(
                        id=f"sample:file:{len(seen_paths)}",
                        type="sample",
                        display_name=path.name,
                        canonical_name=path.name,
                        aliases=[path.stem],
                        tags=["audio", path.suffix.lower().lstrip(".")],
                        path=path_str,
                        source="downloads" if "Downloads" in path_str else "user_library",
                    )
                )
                added += 1

        return {"samples": added}

    def index_mixer_channels(self) -> Dict[str, int]:
        self._assets = [a for a in self._assets if a.type != "mixer_channel"]
        channels = self._safe_list("list_mixer_channels", "channels")
        for idx, item in enumerate(channels):
            if not isinstance(item, dict):
                continue
            name = str(item.get("name") or f"Channel {item.get('index', idx)}")
            self._add_asset(
                Asset(
                    id=f"mixer:{self._norm(name)}",
                    type="mixer_channel",
                    display_name=name,
                    canonical_name=name,
                    aliases=[str(item.get("index", ""))],
                    tags=["mixer", "channel"],
                    source="plugin",
                )
            )
        return {"mixer_channels": len(channels)}

    def index_patterns(self) -> Dict[str, int]:
        self._assets = [a for a in self._assets if a.type != "pattern"]
        patterns = self._safe_list("list_patterns", "patterns")
        for idx, item in enumerate(patterns):
            if not isinstance(item, dict):
                continue
            name = str(item.get("name") or f"Pattern {idx}")
            self._add_asset(
                Asset(
                    id=f"pattern:{self._norm(name)}",
                    type="pattern",
                    display_name=name,
                    canonical_name=name,
                    aliases=[],
                    tags=["pattern", "bb"],
                    source="plugin",
                )
            )
        return {"patterns": len(patterns)}

    def index_controllers(self) -> Dict[str, int]:
        self._assets = [a for a in self._assets if a.type != "controller"]
        controllers = self._safe_list("describe_controllers", "controllers")
        for idx, item in enumerate(controllers):
            if not isinstance(item, dict):
                continue
            name = str(item.get("name") or item.get("id") or f"Controller {idx}")
            controller_type = str(item.get("type") or "lfo")
            self._add_asset(
                Asset(
                    id=f"controller:{self._norm(name)}",
                    type="controller",
                    display_name=name,
                    canonical_name=name,
                    aliases=[str(item.get("id", ""))],
                    tags=["controller", controller_type],
                    source="plugin",
                )
            )
        return {"controllers": len(controllers)}

    def index_automation(self) -> Dict[str, int]:
        self._assets = [a for a in self._assets if a.type != "automation"]
        clips = self._safe_list("list_automation", "clips")
        for idx, item in enumerate(clips):
            if not isinstance(item, dict):
                continue
            clip_id = str(item.get("id") or f"auto:{idx}")
            address = str(item.get("address") or "")
            self._add_asset(
                Asset(
                    id=f"automation:{self._norm(clip_id)}",
                    type="automation",
                    display_name=clip_id,
                    canonical_name=clip_id,
                    aliases=[address] if address else [],
                    tags=["automation", "clip"],
                    source="plugin",
                )
            )
        return {"automation_clips": len(clips)}

    def index_presets_projects(self) -> Dict[str, int]:
        self._assets = [a for a in self._assets if a.type not in {"preset", "project"}]
        added = 0
        for root in self.data_roots:
            if not root:
                continue
            root_path = Path(root).expanduser()
            if not root_path.exists() or not root_path.is_dir():
                continue
            for path in root_path.rglob("*"):
                if not path.is_file():
                    continue
                suffix = path.suffix.lower()
                if suffix in PRESET_EXTS:
                    relative = path.relative_to(root_path)
                    bank = str(relative.parent) if str(relative.parent) != "." else ""
                    self._add_asset(
                        Asset(
                            id=f"preset:{self._norm(str(relative))}",
                            type="preset",
                            display_name=path.stem,
                            canonical_name=path.stem,
                            aliases=[bank, path.name],
                            tags=["preset", suffix.lstrip(".")] + ([bank] if bank else []),
                            path=str(path),
                            source="data_library",
                        )
                    )
                    added += 1
                elif suffix in PROJECT_EXTS:
                    relative = path.relative_to(root_path)
                    self._add_asset(
                        Asset(
                            id=f"project:{self._norm(str(relative))}",
                            type="project",
                            display_name=path.stem,
                            canonical_name=path.stem,
                            aliases=[path.name],
                            tags=["project", suffix.lstrip(".")],
                            path=str(path),
                            source="data_library",
                        )
                    )
                    added += 1
        return {"presets_and_projects": added}

    def search_assets(
        self,
        query: str,
        *,
        asset_type: Optional[str] = None,
        limit: int = 10,
    ) -> List[Dict[str, Any]]:
        scored = []
        for asset in self._assets:
            if asset_type and asset.type != asset_type:
                continue
            score = self._score(query, asset, preferred_type=asset_type)
            if score <= 0:
                continue
            scored.append((score, asset))

        scored.sort(key=lambda pair: pair[0], reverse=True)
        return [
            {
                **asset.as_dict(),
                "score": round(score, 3),
            }
            for score, asset in scored[:limit]
        ]

    def resolve_plugin(self, query: str, plugin_type: str) -> Optional[Dict[str, Any]]:
        expected_type = "instrument_plugin" if plugin_type == "instrument" else "effect_plugin"
        matches = self.search_assets(query, asset_type=expected_type, limit=3)
        return matches[0] if matches else None

    def resolve_sample(self, query: str) -> Optional[Dict[str, Any]]:
        matches = self.search_assets(query, asset_type="sample", limit=3)
        return matches[0] if matches else None

    def resolve_preset(self, query: str) -> Optional[Dict[str, Any]]:
        matches = self.search_assets(query, asset_type="preset", limit=3)
        return matches[0] if matches else None

    def resolve_track_reference(self, query: str) -> Optional[Dict[str, Any]]:
        tracks = self._safe_list("list_tracks", "tracks")
        if not tracks:
            return None
        wanted = self._norm(query)
        exact = next((t for t in tracks if self._norm(t.get("name", "")) == wanted), None)
        if exact:
            return exact
        return next((t for t in tracks if wanted in self._norm(t.get("name", ""))), None)

    def list_mixer_channels(self) -> List[Dict[str, Any]]:
        return self._safe_list("list_mixer_channels", "channels")

    def list_patterns(self) -> List[Dict[str, Any]]:
        return self._safe_list("list_patterns", "patterns")

    def describe_controllers(self) -> List[Dict[str, Any]]:
        return self._safe_list("describe_controllers", "controllers")

    def list_automation(self) -> List[Dict[str, Any]]:
        return self._safe_list("list_automation", "clips")

    def _safe_list(self, tool: str, key: str, args: Optional[Dict[str, Any]] = None) -> List[Any]:
        """Live tool call that degrades to [] while the C++ surface is still landing."""
        try:
            payload = self.tool_client.call_tool(tool, args)
        except ToolClientError:
            return []
        result = payload.get("result", {})
        if not isinstance(result, dict):
            return []
        value = result.get(key, [])
        return value if isinstance(value, list) else []

    def refresh(self, project_path: Optional[str] = None) -> Dict[str, Any]:
        return {
            "plugins": self.index_plugins(),
            "samples": self.index_samples(project_path=project_path),
            "mixer_channels": self.index_mixer_channels(),
            "patterns": self.index_patterns(),
            "controllers": self.index_controllers(),
            "automation": self.index_automation(),
            "presets_and_projects": self.index_presets_projects(),
            "asset_count": len(self._assets),
        }
