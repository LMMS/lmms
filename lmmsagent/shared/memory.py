from __future__ import annotations

import hashlib
import json
from dataclasses import dataclass
from datetime import datetime, timezone
from pathlib import Path
from typing import Any, Dict, List, Optional


@dataclass
class ProjectMemory:
    base_dir: Path = Path.home() / ".lmmsagent" / "memory"

    def _project_id(self, project_path: Optional[str]) -> str:
        key = (project_path or "untitled").encode("utf-8")
        return hashlib.sha1(key, usedforsecurity=False).hexdigest()

    def _project_dir(self, project_path: Optional[str]) -> Path:
        pid = self._project_id(project_path)
        path = self.base_dir / pid
        path.mkdir(parents=True, exist_ok=True)
        return path

    def append_journal_entry(self, project_path: Optional[str], entry: Dict[str, Any]) -> Dict[str, Any]:
        path = self._project_dir(project_path)
        entry_with_meta = {
            "project_id": self._project_id(project_path),
            "timestamp": datetime.now(timezone.utc).isoformat(),
            **entry,
        }
        with (path / "journal.jsonl").open("a", encoding="utf-8") as handle:
            handle.write(json.dumps(entry_with_meta, ensure_ascii=True) + "\n")
        return entry_with_meta

    def read_journal(self, project_path: Optional[str], limit: int = 100) -> List[Dict[str, Any]]:
        journal_path = self._project_dir(project_path) / "journal.jsonl"
        if not journal_path.exists():
            return []
        lines = journal_path.read_text(encoding="utf-8").splitlines()
        rows = [json.loads(line) for line in lines if line.strip()]
        return rows[-limit:]

    def load_preferences(self, project_path: Optional[str]) -> Dict[str, Any]:
        pref_path = self._project_dir(project_path) / "preferences.json"
        if not pref_path.exists():
            return {}
        return json.loads(pref_path.read_text(encoding="utf-8"))

    def update_preferences(self, project_path: Optional[str], updates: Dict[str, Any]) -> Dict[str, Any]:
        prefs = self.load_preferences(project_path)
        prefs.update(updates)
        pref_path = self._project_dir(project_path) / "preferences.json"
        pref_path.write_text(json.dumps(prefs, ensure_ascii=True, indent=2) + "\n", encoding="utf-8")
        return prefs
