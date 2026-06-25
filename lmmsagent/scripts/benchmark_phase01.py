#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import time
import wave
from datetime import datetime, timezone
from pathlib import Path
from typing import Any, Dict, List

ROOT = Path(__file__).resolve().parents[1]

import sys

if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from shared import DiscoveryIndex, Orchestrator, Planner, ProjectMemory, ToolClient, ToolClientError


def percentile(values: List[int], pct: float) -> int:
    if not values:
        return 0
    ordered = sorted(values)
    idx = int(round((pct / 100.0) * (len(ordered) - 1)))
    return ordered[max(0, min(idx, len(ordered) - 1))]


def ensure_silent_wav(path: Path, duration_s: float = 0.2, sample_rate: int = 16000) -> None:
    if path.exists():
        return
    frame_count = int(duration_s * sample_rate)
    path.parent.mkdir(parents=True, exist_ok=True)
    with wave.open(str(path), "wb") as handle:
        handle.setnchannels(1)
        handle.setsampwidth(2)
        handle.setframerate(sample_rate)
        handle.writeframes(b"\x00\x00" * frame_count)


def load_commands(suite_path: Path, selected_buckets: List[str]) -> List[str]:
    payload = json.loads(suite_path.read_text(encoding="utf-8"))
    buckets = payload.get("buckets", {})
    commands: List[str] = []
    for bucket in selected_buckets:
        for command in buckets.get(bucket, []):
            if isinstance(command, str) and command.strip():
                commands.append(command.strip())
    return commands


def build_orchestrator(sample_roots: List[str]) -> Orchestrator:
    client = ToolClient()
    discovery = DiscoveryIndex(client, sample_roots=sample_roots)
    planner = Planner()
    memory = ProjectMemory()
    return Orchestrator(client, discovery, planner, memory)


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Phase 0/1 baseline benchmark for LMMS agent orchestration latency and success"
    )
    parser.add_argument(
        "--suite",
        default=str(ROOT / "evals" / "task_suite.json"),
        help="path to benchmark suite JSON",
    )
    parser.add_argument(
        "--bucket",
        action="append",
        default=["atomic", "compositional"],
        help="suite bucket to include (repeatable)",
    )
    parser.add_argument("--repeat", type=int, default=1, help="number of times to execute each command")
    parser.add_argument("--project-path", default=None, help="project file path for memory scoping")
    parser.add_argument("--sample-root", action="append", default=[], help="additional sample search roots")
    parser.add_argument(
        "--out",
        default=None,
        help="optional explicit output JSON report path",
    )
    args = parser.parse_args()

    if args.repeat < 1:
        raise SystemExit("--repeat must be >= 1")

    import tempfile
    ensure_silent_wav(Path(tempfile.gettempdir()) / "loop.wav")
    ensure_silent_wav(Path(tempfile.gettempdir()) / "kick.wav")
    ensure_silent_wav(Path(tempfile.gettempdir()) / "vocal.wav")

    suite_path = Path(args.suite).expanduser().resolve()
    commands = load_commands(suite_path, selected_buckets=args.bucket)
    if not commands:
        raise SystemExit(f"No benchmark commands loaded from {suite_path}")

    orchestrator = build_orchestrator(args.sample_root)
    started_at = datetime.now(timezone.utc).isoformat()

    rows: List[Dict[str, Any]] = []
    failures = 0
    clarifications = 0
    wall_times: List[int] = []
    stage_samples: Dict[str, List[int]] = {}

    for _ in range(args.repeat):
        for command in commands:
            run_started = time.monotonic()
            try:
                result = orchestrator.run(command, project_path=args.project_path)
                wall_ms = int((time.monotonic() - run_started) * 1000)
                wall_times.append(wall_ms)

                mode = result.get("mode")
                status = result.get("status")
                success = status == "success"
                needs_clarification = mode == "clarify" or bool(result.get("needs_clarification", False))

                if not success:
                    failures += 1
                if needs_clarification:
                    clarifications += 1

                telemetry = result.get("telemetry", {})
                stage_timings = telemetry.get("stage_timings_ms", {})
                if isinstance(stage_timings, dict):
                    for stage, value in stage_timings.items():
                        if isinstance(value, int):
                            stage_samples.setdefault(stage, []).append(value)

                rows.append(
                    {
                        "command": command,
                        "ok": success,
                        "mode": mode,
                        "status": status,
                        "needs_clarification": needs_clarification,
                        "wall_time_ms": wall_ms,
                        "telemetry": telemetry,
                    }
                )
            except ToolClientError as exc:
                wall_ms = int((time.monotonic() - run_started) * 1000)
                wall_times.append(wall_ms)
                failures += 1
                rows.append(
                    {
                        "command": command,
                        "ok": False,
                        "mode": "error",
                        "status": "failed",
                        "needs_clarification": False,
                        "wall_time_ms": wall_ms,
                        "error": str(exc),
                    }
                )

    total = len(rows)
    success_count = sum(1 for row in rows if row.get("ok"))

    report: Dict[str, Any] = {
        "started_at": started_at,
        "finished_at": datetime.now(timezone.utc).isoformat(),
        "suite_path": str(suite_path),
        "buckets": args.bucket,
        "repeat": args.repeat,
        "total_commands": total,
        "success_count": success_count,
        "failure_count": failures,
        "clarification_count": clarifications,
        "success_rate": round((success_count / total), 4) if total else 0.0,
        "clarification_rate": round((clarifications / total), 4) if total else 0.0,
        "latency_ms": {
            "wall_p50": percentile(wall_times, 50),
            "wall_p95": percentile(wall_times, 95),
            "wall_max": max(wall_times) if wall_times else 0,
        },
        "stage_latency_ms": {
            stage: {
                "p50": percentile(samples, 50),
                "p95": percentile(samples, 95),
                "max": max(samples) if samples else 0,
            }
            for stage, samples in sorted(stage_samples.items())
        },
        "rows": rows,
    }

    if args.out:
        out_path = Path(args.out).expanduser().resolve()
    else:
        report_dir = ROOT / "evals" / "reports"
        report_dir.mkdir(parents=True, exist_ok=True)
        stamp = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ")
        out_path = report_dir / f"phase01_baseline_{stamp}.json"

    out_path.write_text(json.dumps(report, ensure_ascii=True, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"ok": True, "report": str(out_path), "summary": report["latency_ms"]}, ensure_ascii=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
