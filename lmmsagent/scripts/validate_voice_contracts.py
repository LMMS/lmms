#!/usr/bin/env python3
"""Validate LMMS Agent voice command contracts (manifest + schema consistency)."""

from __future__ import annotations

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
INTEGRATION_DIR = ROOT / "integrations" / "lmms" / "AgentControl"
MANIFEST_V3 = INTEGRATION_DIR / "command_manifest.v3.json"
MANIFEST_V2 = INTEGRATION_DIR / "command_manifest.v2.json"
MANIFEST = MANIFEST_V3 if MANIFEST_V3.exists() else MANIFEST_V2
MANIFEST_SCHEMA = INTEGRATION_DIR / "command_manifest.schema.json"
LLM_SCHEMA = INTEGRATION_DIR / "llm_interpretation.schema.json"
GOLDEN = ROOT / "evals" / "voice_golden_scenarios.v2.json"


def load_json(path: Path) -> dict:
    with path.open("r", encoding="utf-8") as f:
        return json.load(f)


def fail(message: str) -> None:
    print(f"ERROR: {message}")
    raise SystemExit(1)


def validate_manifest(manifest: dict) -> None:
    version = str(manifest.get("version", "")).strip()
    if not (version.startswith("2.") or version.startswith("3.")):
        fail(f"manifest version must start with '2.' or '3.' but got '{version}'")

    intents = manifest.get("intents")
    if not isinstance(intents, list) or not intents:
        fail("manifest intents must be a non-empty array")

    intent_names: set[str] = set()
    capability_names: set[str] = set()
    capability_duplicates: list[str] = []

    for idx, item in enumerate(intents):
        where = f"intents[{idx}]"
        if not isinstance(item, dict):
            fail(f"{where} must be an object")

        intent = str(item.get("intent", "")).strip()
        if not intent:
            fail(f"{where}.intent is required")
        if intent in intent_names:
            fail(f"duplicate intent '{intent}'")
        intent_names.add(intent)

        aliases = item.get("aliases")
        if not isinstance(aliases, list) or not aliases:
            fail(f"{where}.aliases must be a non-empty array")

        risk = item.get("risk_level")
        if risk not in {"safe", "confirm"}:
            fail(f"{where}.risk_level must be safe|confirm")

        capability = str(item.get("capability_flag", "")).strip()
        if not capability:
            fail(f"{where}.capability_flag is required")
        if capability in capability_names:
            capability_duplicates.append(capability)
        capability_names.add(capability)

    for capability in capability_duplicates:
        print(f"WARNING: capability_flag '{capability}' reused across intents (expected in v3)")

        policy = item.get("confirmation_policy")
        if not isinstance(policy, dict):
            fail(f"{where}.confirmation_policy must be an object")

        mode = policy.get("mode")
        if mode not in {"never", "always", "confidence_below"}:
            fail(f"{where}.confirmation_policy.mode must be never|always|confidence_below")
        if mode == "confidence_below":
            threshold = policy.get("threshold")
            if not isinstance(threshold, (int, float)):
                fail(f"{where}.confirmation_policy.threshold must be numeric when mode=confidence_below")
            if threshold < 0.0 or threshold > 1.0:
                fail(f"{where}.confirmation_policy.threshold must be in [0,1]")


def validate_llm_schema(schema: dict) -> None:
    required = set(schema.get("required", []))
    expected = {"familiar", "intent", "command", "arguments", "confidence", "risk_level", "reason"}
    if not expected.issubset(required):
        fail("llm_interpretation.schema.json missing required fields for structured parser output")


def validate_golden(golden: dict, manifest: dict) -> None:
    cases = golden.get("cases")
    if not isinstance(cases, list) or not cases:
        fail("golden scenarios must contain a non-empty cases array")

    known_intents = {str(item.get("intent", "")).strip() for item in manifest.get("intents", [])}
    for idx, case in enumerate(cases):
        where = f"golden.cases[{idx}]"
        if not isinstance(case, dict):
            fail(f"{where} must be an object")
        utterance = str(case.get("utterance", "")).strip()
        if not utterance:
            fail(f"{where}.utterance is required")
        expected_intent = str(case.get("expected_intent", "")).strip()
        if expected_intent and expected_intent not in known_intents:
            fail(f"{where}.expected_intent '{expected_intent}' does not exist in manifest")


def main() -> int:
    for path in (MANIFEST, MANIFEST_SCHEMA, LLM_SCHEMA, GOLDEN):
        if not path.exists():
            fail(f"missing required contract file: {path}")

    manifest = load_json(MANIFEST)
    _ = load_json(MANIFEST_SCHEMA)
    llm_schema = load_json(LLM_SCHEMA)
    golden = load_json(GOLDEN)

    validate_manifest(manifest)
    validate_llm_schema(llm_schema)
    validate_golden(golden, manifest)

    print("Voice contract validation passed:")
    print(f"- manifest: {MANIFEST}")
    print(f"- manifest schema: {MANIFEST_SCHEMA}")
    print(f"- llm schema: {LLM_SCHEMA}")
    print(f"- golden scenarios: {GOLDEN}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
