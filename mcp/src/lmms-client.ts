/**
 * TCP JSON-line clients for the LMMS agent stack.
 *
 * - `LmmsClient` talks to the AgentControl tool server that runs inside LMMS
 *   itself (plugins/AgentControl, default 127.0.0.1:7777). The protocol is a
 *   newline-delimited JSON request/response per connection:
 *
 *     -> {"tool":"settempo","args":{"tempo":120}}
 *     <- {"ok":true,"result":{...},"state_delta":{...},"warnings":[],"error_code":null,"error_message":null}
 *
 *   A request without `tool` is interpreted as a natural-language command and
 *   may carry `command`, `file`, `path`, `plugin`, `track` fields.
 *
 * - `AgentdClient` talks to the persistent agent daemon
 *   (lmmsagent/lmms-agentd, default 127.0.0.1:7781) whose `run_goal` op runs
 *   the hybrid NL interpreter (deterministic + heuristic + Ollama).
 */
import net from "node:net";

export class LmmsClientError extends Error {
  constructor(
    public readonly code: string,
    message: string,
  ) {
    super(message);
    this.name = "LmmsClientError";
  }
}

export interface LmmsEnvelope {
  ok: boolean;
  result: Record<string, unknown>;
  state_delta?: Record<string, unknown>;
  warnings?: unknown[];
  error_code?: string | null;
  error_message?: string | null;
}

export interface LmmsClientOptions {
  host?: string;
  port?: number;
  timeoutMs?: number;
}

interface JsonLineClientOptions extends LmmsClientOptions {
  /** Short human label used in connection errors, e.g. "AgentControl tool server". */
  name: string;
  /** Hint appended to connection errors, e.g. how to start the service. */
  connectHint: string;
}

/**
 * One connection per request (mirrors lmmsagent/shared/tool_client.py) so a
 * hung or stale LMMS socket never wedges the MCP server.
 */
async function jsonLineExchange(
  options: JsonLineClientOptions,
  payload: Record<string, unknown>,
): Promise<Record<string, unknown>> {
  const host = options.host ?? "127.0.0.1";
  const port = options.port ?? 7777;
  const timeoutMs = options.timeoutMs ?? 8000;
  const name = options.name;
  const connectHint = options.connectHint;

  const raw = `${JSON.stringify(payload)}\n`;

  const { promise, resolve, reject } = Promise.withResolvers<Record<string, unknown>>();
  const socket = new net.Socket();
  let buffer = Buffer.alloc(0);
  let settled = false;

  const fail = (code: string, message: string): void => {
    if (settled) {
      return;
    }
    settled = true;
    socket.destroy();
    reject(new LmmsClientError(code, message));
  };

  socket.setTimeout(timeoutMs);
  socket.on("timeout", () => {
    fail("timeout", `${name} at ${host}:${port} did not respond within ${timeoutMs}ms`);
  });
  socket.on("error", (err: NodeJS.ErrnoException) => {
    if (err.code === "ECONNREFUSED" || err.code === "EHOSTUNREACH" || err.code === "ENOTFOUND") {
      fail(
        "connection_refused",
        `${name} is not reachable at ${host}:${port}. ${connectHint} (${err.message})`,
      );
    } else {
      fail("connection_error", `error talking to ${name} at ${host}:${port}: ${err.message}`);
    }
  });
  socket.on("data", (chunk: Buffer) => {
    buffer = Buffer.concat([buffer, chunk]);
    const newline = buffer.indexOf(0x0a);
    if (newline < 0) {
      if (buffer.length > 1_000_000) {
        fail("protocol_error", `${name} returned an oversized response`);
      }
      return;
    }
    const line = buffer.subarray(0, newline).toString("utf8");
    settled = true;
    socket.destroy();
    let parsed: unknown;
    try {
      parsed = JSON.parse(line);
    } catch (err) {
      reject(
        new LmmsClientError(
          "protocol_error",
          `${name} returned invalid JSON: ${err instanceof Error ? err.message : String(err)}`,
        ),
      );
      return;
    }
    if (!parsed || typeof parsed !== "object" || Array.isArray(parsed)) {
      reject(new LmmsClientError("protocol_error", `${name} returned a non-object response`));
      return;
    }
    resolve(parsed as Record<string, unknown>);
  });
  socket.on("close", () => {
    if (!settled) {
      fail("connection_error", `${name} closed the connection without a response`);
    }
  });

  socket.connect(port, host, () => {
    socket.write(raw);
  });

  return promise;
}

function envelopeOf(response: Record<string, unknown>): LmmsEnvelope {
  return {
    ok: response.ok === true,
    result: (response.result as Record<string, unknown> | undefined) ?? {},
    state_delta: response.state_delta as Record<string, unknown> | undefined,
    warnings: (response.warnings as unknown[] | undefined) ?? [],
    error_code: (response.error_code as string | null | undefined) ?? null,
    error_message: (response.error_message as string | null | undefined) ?? null,
  };
}

/**
 * Client for the AgentControl tool server embedded in LMMS
 * (plugins/AgentControl, TCP 127.0.0.1:7777 by default).
 */
export class LmmsClient {
  private readonly options: JsonLineClientOptions;

  constructor(options: LmmsClientOptions = {}) {
    this.options = {
      ...options,
      name: "LMMS AgentControl tool server",
      connectHint:
        "Start LMMS with the AgentControl plugin loaded (or run lmmsagent/scripts/run_agentd.sh)",
    };
  }

  /** Invoke one typed DAW tool, e.g. "settempo", "loadinstrument", "addnotes". */
  async callTool(tool: string, args: Record<string, unknown> = {}): Promise<LmmsEnvelope> {
    return envelopeOf(await jsonLineExchange(this.options, { tool, args }));
  }

  /**
   * Send a natural-language command ("play", "add 808", "open slicer and
   * import loop.wav and split into 16"). The plugin answers with a message
   * string; commands gated by the confirmation policy return a
   * "Confirmation required: ..." message instead of executing.
   */
  async command(
    text: string,
    extra: { file?: string; path?: string; plugin?: string; track?: string } = {},
  ): Promise<LmmsEnvelope> {
    const payload: Record<string, unknown> = { command: text };
    if (extra.file !== undefined) {
      payload.file = extra.file;
    }
    if (extra.path !== undefined) {
      payload.path = extra.path;
    }
    if (extra.plugin !== undefined) {
      payload.plugin = extra.plugin;
    }
    if (extra.track !== undefined) {
      payload.track = extra.track;
    }
    return envelopeOf(await jsonLineExchange(this.options, payload));
  }
}

export interface AgentdEnvelope {
  ok: boolean;
  op: string;
  request_id?: string;
  timeout_class?: string;
  attempts?: number;
  replayed?: boolean;
  duration_ms?: number;
  result?: Record<string, unknown>;
  error?: { code?: string; message?: string } | null;
  meta?: Record<string, unknown>;
}

export interface RunGoalOptions {
  project_path?: string;
  timeout_class?: "interactive" | "standard" | "background";
  retries?: number;
  idempotency_key?: string;
}

/**
 * Client for the persistent agent daemon (lmmsagent/lmms-agentd, TCP
 * 127.0.0.1:7781 by default). Its `run_goal` op interprets a natural-language
 * goal through the hybrid planner and executes the resulting tool plan inside
 * LMMS, with retries, idempotency, and confirmation gating.
 */
export class AgentdClient {
  private readonly options: JsonLineClientOptions;

  constructor(options: LmmsClientOptions & { name?: string } = {}) {
    this.options = {
      ...options,
      port: options.port ?? 7781,
      name: options.name ?? "LMMS agent daemon",
      connectHint:
        "Start the daemon first: lmmsagent/scripts/run_agentd.sh (it forwards typed actions to AgentControl inside LMMS)",
    };
  }

  async runGoal(goal: string, runOptions: RunGoalOptions = {}): Promise<AgentdEnvelope> {
    const payload: Record<string, unknown> = {
      op: "run_goal",
      goal,
      timeout_class: runOptions.timeout_class ?? "standard",
    };
    if (runOptions.project_path !== undefined) {
      payload.project_path = runOptions.project_path;
    }
    if (runOptions.retries !== undefined) {
      payload.retries = runOptions.retries;
    }
    if (runOptions.idempotency_key !== undefined) {
      payload.idempotency_key = runOptions.idempotency_key;
    }
    const response = await jsonLineExchange(this.options, payload);
    return {
      ok: response.ok === true,
      op: (response.op as string | undefined) ?? "run_goal",
      request_id: response.request_id as string | undefined,
      timeout_class: response.timeout_class as string | undefined,
      attempts: response.attempts as number | undefined,
      replayed: response.replayed as boolean | undefined,
      duration_ms: response.duration_ms as number | undefined,
      result: response.result as Record<string, unknown> | undefined,
      error: (response.error as { code?: string; message?: string } | null | undefined) ?? null,
      meta: response.meta as Record<string, unknown> | undefined,
    };
  }
}
