# AgentControl Integration

Canonical LMMS integration boundary for the agent stack.

Implementation lives in the LMMS plugin source:

- `/Users/saksham/grp/docs/lmmsagent/lmms/plugins/AgentControl/AgentControl.h`
- `/Users/saksham/grp/docs/lmmsagent/lmms/plugins/AgentControl/AgentControl.cpp`

Transport:

- TCP localhost `127.0.0.1:7777`
- newline-delimited JSON request/response

Phase 1 runtime split:

- external daemon `lmmsagent/lmms-agentd/main.py` on `127.0.0.1:7781`
- daemon request envelope fields: `op`, `request_id`, `idempotency_key`, `timeout_class`, `retries`
- daemon forwards typed actions to this AgentControl transport

Request:

```json
{
  "tool": "load_sample",
  "args": {
    "track": "Agent 808",
    "sample_path": "/path/to/sample.wav"
  }
}
```

Response envelope:

```json
{
  "ok": true,
  "result": {},
  "state_delta": {},
  "warnings": [],
  "error_code": null,
  "error_message": null
}
```

## Voice Command Contracts (v3)

- Command manifest schema: `/Users/saksham/grp/docs/lmmsagent/lmms/lmmsagent/integrations/lmms/AgentControl/command_manifest.schema.json`
- Default command manifest: `/Users/saksham/grp/docs/lmmsagent/lmms/lmmsagent/integrations/lmms/AgentControl/command_manifest.v3.json` (canonical; v2 kept for compatibility)
- LLM parse schema: `/Users/saksham/grp/docs/lmmsagent/lmms/lmmsagent/integrations/lmms/AgentControl/llm_interpretation.schema.json`

`AgentControl` loads the default manifest at runtime when available from the current working tree path, or from `LMMS_COMMAND_MANIFEST` when explicitly set.

## Internal Interpreter Contract

Interpreter output is normalized before dispatch:

```json
{
  "intent": "slicer.split.equal",
  "args": { "segments": 16 },
  "confidence": 0.91,
  "risk_level": "safe",
  "source": "deterministic|llm|hybrid"
}
```

## Runtime Controls

Core environment knobs:

- `LMMS_OLLAMA_MODEL` (default planner model; expected `qwen2.5:7b-instruct`)
- `LMMS_OLLAMA_URL` (default `http://127.0.0.1:11434/api/chat`)
- `LMMS_OLLAMA_TIMEOUT_MS` (default `2500`)
- `LMMS_OLLAMA_CONFIDENCE` (LLM viability threshold; default `0.72`)
- `LMMS_HYBRID_MARGIN` (LLM arbitration margin over heuristic; default `0.12`)
- `LMMS_CONFIRM_CONFIDENCE` (low-confidence confirm gate; default `0.74`)
- `LMMS_CONFIRM_WINDOW_MS` (confirm timeout; default `9000`)
- `LMMS_WAKE_REQUIRED` (default `1`)
- `LMMS_WAKE_PHRASES` (comma-separated, default `hey lmms,ok lmms`)
- `LMMS_WAKE_WINDOW_MS` (default `8000`)
- `LMMS_VOICE_POLL_MS` (default `250`)
- `LMMS_VOICE_MIN_BYTES` (default `4096`)
- `LMMS_VOICE_SILENCE_ABS_THRESHOLD` (default `120`)
- `LMMS_VOICE_TRACE` (`1` enables JSON stage traces)
- `LMMS_CAP_*` per-intent capability flags (for phased rollout).

Daemon controls:

- `--host` (default `127.0.0.1`)
- `--port` (default `7781`)
- `--cache-ttl-s` idempotency cache TTL
