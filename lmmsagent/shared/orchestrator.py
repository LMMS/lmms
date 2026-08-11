from __future__ import annotations

import time
import uuid
from dataclasses import dataclass
from typing import Any, Callable, Dict, List, Optional

from .discovery import DiscoveryIndex
from .memory import ProjectMemory
from .planner import CONFIRM_GATED_ACTIONS, Planner
from .tool_client import ToolClient, ToolClientError


@dataclass
class Orchestrator:
    tool_client: ToolClient
    discovery: DiscoveryIndex
    planner: Planner
    memory: ProjectMemory

    def _execute_step(
        self,
        action: str,
        args: Dict[str, Any],
        context: Dict[str, Any],
    ) -> Dict[str, Any]:
        if action == "guide_note":
            return {"ok": True, "result": {"note": args.get("note", "")}}

        if action == "resolve_plugin":
            plugin_type = args.get("type", "instrument")
            resolved = self.discovery.resolve_plugin(args.get("query", ""), plugin_type)
            if not resolved:
                raise ToolClientError(f"could_not_resolve_plugin: {args.get('query', '')}")
            context["resolved_plugin"] = resolved
            return {"ok": True, "result": resolved}

        if action == "resolve_sample":
            resolved = self.discovery.resolve_sample(args.get("query", ""))
            if not resolved:
                raise ToolClientError(f"could_not_resolve_sample: {args.get('query', '')}")
            context["resolved_sample"] = resolved
            return {"ok": True, "result": resolved}

        if action == "resolve_track_reference":
            resolved = self.discovery.resolve_track_reference(args.get("query", ""))
            if not resolved:
                raise ToolClientError(f"could_not_resolve_track: {args.get('query', '')}")
            context["resolved_track"] = resolved
            return {"ok": True, "result": resolved}

        if action == "resolve_preset":
            resolved = self.discovery.resolve_preset(args.get("query", ""))
            if not resolved:
                raise ToolClientError(f"could_not_resolve_preset: {args.get('query', '')}")
            context["resolved_preset"] = resolved
            return {"ok": True, "result": resolved}

        runtime_args = dict(args)
        if action == "load_instrument_preset" and not runtime_args.get("path"):
            resolved_preset = context.get("resolved_preset")
            if isinstance(resolved_preset, dict):
                runtime_args["path"] = resolved_preset.get("path")

        if action in {"load_sample", "import_audio"} and not runtime_args.get("sample_path"):
            resolved = context.get("resolved_sample")
            if isinstance(resolved, dict):
                runtime_args["sample_path"] = resolved.get("path")

        if action in {"load_instrument", "add_effect"} and not runtime_args.get("plugin") and not runtime_args.get("effect"):
            resolved_plugin = context.get("resolved_plugin")
            if isinstance(resolved_plugin, dict):
                if action == "load_instrument":
                    runtime_args["plugin"] = resolved_plugin.get("canonical_name")
                if action == "add_effect":
                    runtime_args["effect"] = resolved_plugin.get("canonical_name")

        if "track" not in runtime_args:
            resolved_track = context.get("resolved_track")
            if isinstance(resolved_track, dict) and resolved_track.get("name"):
                runtime_args["track"] = resolved_track["name"]

        return self.tool_client.call_tool(action, runtime_args)

    def run(
        self,
        goal: str,
        *,
        project_path: Optional[str] = None,
        confirm_step: Optional[Callable[[Dict[str, Any]], bool]] = None,
        request_id: Optional[str] = None,
    ) -> Dict[str, Any]:
        resolved_request_id = request_id or f"req_{uuid.uuid4().hex[:12]}"
        total_started = time.monotonic()
        stage_timings_ms: Dict[str, int] = {}
        trace_events: List[Dict[str, Any]] = []

        def add_stage_timing(stage: str, elapsed_ms: int) -> None:
            stage_timings_ms[stage] = stage_timings_ms.get(stage, 0) + elapsed_ms

        def trace(stage: str, event: str, **data: Any) -> None:
            trace_events.append(
                {
                    "ts_ms": int(time.time() * 1000),
                    "stage": stage,
                    "event": event,
                    "data": data,
                }
            )

        def telemetry(step_count: int) -> Dict[str, Any]:
            return {
                "request_id": resolved_request_id,
                "total_runtime_ms": int((time.monotonic() - total_started) * 1000),
                "stage_timings_ms": stage_timings_ms,
                "step_count": step_count,
                "trace_events": trace_events,
            }

        trace("orchestrator", "start", goal=goal)

        started = time.monotonic()
        self.discovery.refresh(project_path=project_path)
        add_stage_timing("discovery_refresh", int((time.monotonic() - started) * 1000))

        started = time.monotonic()
        state = self.tool_client.get_project_state()
        add_stage_timing("initial_state_read", int((time.monotonic() - started) * 1000))

        started = time.monotonic()
        preferences = self.memory.load_preferences(project_path)
        add_stage_timing("preferences_load", int((time.monotonic() - started) * 1000))

        started = time.monotonic()
        plan = self.planner.plan(goal, state=state, discovery=self.discovery, preferences=preferences)
        add_stage_timing("planning", int((time.monotonic() - started) * 1000))
        trace("planner", "result", mode=plan.mode, subgoals=len(plan.subgoals))

        if plan.mode in {"clarify", "reject"}:
            payload = plan.to_dict()
            started = time.monotonic()
            self.memory.append_journal_entry(
                project_path,
                {
                    "request": goal,
                    "request_id": resolved_request_id,
                    "plan_id": None,
                    "clarification": payload.get("clarification_question"),
                    "steps": [],
                    "resolved_entities": [],
                    "outcome": "clarify" if plan.mode == "clarify" else "reject",
                },
            )
            add_stage_timing("journal_write", int((time.monotonic() - started) * 1000))
            payload["telemetry"] = telemetry(step_count=0)
            trace("orchestrator", "finish", outcome=plan.mode)
            return payload

        flat_steps = [step for subgoal in plan.subgoals for step in subgoal.steps]
        for step in flat_steps:
            if step.confidence < self.planner.low_confidence_threshold:
                payload = {
                    "goal": goal,
                    "mode": "clarify",
                    "needs_clarification": True,
                    "clarification_question": (
                        f"I am only {step.confidence:.2f} confident about '{step.action}'. "
                        "Can you confirm this operation?"
                    ),
                }
                payload["telemetry"] = telemetry(step_count=0)
                trace("orchestrator", "finish", outcome="clarify_low_confidence", action=step.action)
                return payload
            if step.risk in {"destructive", "irreversible"}:
                payload = {
                    "goal": goal,
                    "mode": "clarify",
                    "needs_clarification": True,
                    "clarification_question": (
                        f"'{step.action}' is marked as {step.risk}. Should I proceed?"
                    ),
                }
                payload["telemetry"] = telemetry(step_count=0)
                trace("orchestrator", "finish", outcome="clarify_risk", action=step.action)
                return payload

        step_results: List[Dict[str, Any]] = []
        resolved_entities: List[Dict[str, Any]] = []
        context: Dict[str, Any] = {}

        for subgoal in plan.subgoals:
            for step in subgoal.steps:
                if confirm_step is not None:
                    should_run = confirm_step(
                        {
                            "subgoal": subgoal.id,
                            "subgoal_title": subgoal.title,
                            "action": step.action,
                            "args": step.args,
                            "confidence": step.confidence,
                            "risk": step.risk,
                            "requires_snapshot": step.requires_snapshot,
                        }
                    )
                    if not should_run:
                        aborted = {
                            "goal": goal,
                            "mode": "plan",
                            "needs_clarification": False,
                            "status": "aborted",
                            "aborted_step": step.action,
                            "steps": step_results,
                        }
                        started = time.monotonic()
                        self.memory.append_journal_entry(
                            project_path,
                            {
                                "request": goal,
                                "request_id": resolved_request_id,
                                "clarification": None,
                                "plan_id": "p_001",
                                "steps": step_results,
                                "resolved_entities": resolved_entities,
                                "outcome": "aborted",
                            },
                        )
                        add_stage_timing("journal_write", int((time.monotonic() - started) * 1000))
                        aborted["telemetry"] = telemetry(step_count=len(step_results))
                        trace("orchestrator", "finish", outcome="aborted", action=step.action)
                        return aborted

                snapshot_id = None
                if (
                    (step.requires_snapshot or step.action in CONFIRM_GATED_ACTIONS)
                    and step.action not in {"create_snapshot", "rollback_to_snapshot"}
                ):
                    snap_started = time.monotonic()
                    snapshot_resp = self.tool_client.call_tool("create_snapshot", {"label": f"pre_{step.action}"})
                    add_stage_timing("snapshot_calls", int((time.monotonic() - snap_started) * 1000))
                    snapshot_id = snapshot_resp.get("result", {}).get("snapshot_id")

                started = time.monotonic()
                trace("step", "start", subgoal=subgoal.id, action=step.action)
                try:
                    response = self._execute_step(step.action, step.args, context)
                    elapsed_ms = int((time.monotonic() - started) * 1000)
                    add_stage_timing("step_execution", elapsed_ms)

                    if step.action.startswith("resolve_"):
                        resolved_entities.append(
                            {
                                "type": step.action,
                                "query": step.args.get("query"),
                                "result": response.get("result", {}),
                            }
                        )

                    state_started = time.monotonic()
                    state_after = self.tool_client.get_project_state()
                    add_stage_timing("post_step_state_reads", int((time.monotonic() - state_started) * 1000))
                    tool_transport_ms = None
                    transport = response.get("_transport", {})
                    if isinstance(transport, dict):
                        value = transport.get("latency_ms")
                        if isinstance(value, int):
                            tool_transport_ms = value

                    step_results.append(
                        {
                            "subgoal": subgoal.id,
                            "action": step.action,
                            "args": step.args,
                            "result": response.get("result", {}),
                            "state_delta": response.get("state_delta", {}),
                            "latency_ms": elapsed_ms,
                            "tool_transport_ms": tool_transport_ms,
                            "state_after_tempo": state_after.get("tempo"),
                        }
                    )
                    trace("step", "end", subgoal=subgoal.id, action=step.action, latency_ms=elapsed_ms)
                except ToolClientError as exc:
                    add_stage_timing("step_execution", int((time.monotonic() - started) * 1000))
                    trace("step", "error", subgoal=subgoal.id, action=step.action, error=str(exc))
                    rollback_result: Dict[str, Any] = {}
                    if snapshot_id:
                        try:
                            rollback_started = time.monotonic()
                            rollback_result = self.tool_client.call_tool(
                                "rollback_to_snapshot",
                                {"snapshot_id": snapshot_id},
                            )
                            add_stage_timing("rollback_calls", int((time.monotonic() - rollback_started) * 1000))
                        except ToolClientError:
                            rollback_result = {
                                "ok": False,
                                "error": "rollback_failed",
                            }

                    failure = {
                        "goal": goal,
                        "mode": "plan",
                        "needs_clarification": False,
                        "status": "failed",
                        "failed_step": step.action,
                        "error": str(exc),
                        "rollback": rollback_result,
                        "steps": step_results,
                    }
                    started = time.monotonic()
                    self.memory.append_journal_entry(
                        project_path,
                        {
                            "request": goal,
                            "request_id": resolved_request_id,
                            "clarification": None,
                            "plan_id": "p_001",
                            "steps": step_results,
                            "resolved_entities": resolved_entities,
                            "outcome": "failed",
                        },
                    )
                    add_stage_timing("journal_write", int((time.monotonic() - started) * 1000))
                    failure["telemetry"] = telemetry(step_count=len(step_results))
                    trace("orchestrator", "finish", outcome="failed", failed_step=step.action)
                    return failure

        started = time.monotonic()
        final_state = self.tool_client.get_project_state()
        add_stage_timing("final_state_read", int((time.monotonic() - started) * 1000))
        if resolved_entities:
            last_entity = resolved_entities[-1].get("result", {})
            if isinstance(last_entity, dict):
                pref_key = "last_resolved_asset"
                if "canonical_name" in last_entity:
                    started = time.monotonic()
                    self.memory.update_preferences(project_path, {pref_key: last_entity["canonical_name"]})
                    add_stage_timing("preferences_update", int((time.monotonic() - started) * 1000))

        response = {
            "goal": goal,
            "mode": "plan",
            "needs_clarification": False,
            "status": "success",
            "subgoals_executed": len(plan.subgoals),
            "steps": step_results,
            "final_state": {
                "tempo": final_state.get("tempo"),
                "track_count": final_state.get("track_count"),
                "project_file": final_state.get("project_file"),
            },
        }

        started = time.monotonic()
        self.memory.append_journal_entry(
            project_path,
            {
                "request": goal,
                "request_id": resolved_request_id,
                "clarification": None,
                "plan_id": "p_001",
                "steps": step_results,
                "resolved_entities": resolved_entities,
                "outcome": "success",
            },
        )
        add_stage_timing("journal_write", int((time.monotonic() - started) * 1000))
        response["telemetry"] = telemetry(step_count=len(step_results))
        trace("orchestrator", "finish", outcome="success", steps=len(step_results))
        return response
