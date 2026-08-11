#!/usr/bin/env node
/**
 * lmms-operator entrypoint: stdio transport for MCP clients that want to
 * operate a running LMMS (tracks, instruments, samples, effects, patterns,
 * transport, project files) through another LLM.
 *
 * Env:
 *   LMMS_AGENT_HOST / LMMS_AGENT_PORT   AgentControl tool server in LMMS
 *                                       (default 127.0.0.1:7777)
 *   LMMS_AGENTD_HOST / LMMS_AGENTD_PORT agent daemon for lmms_goal
 *                                       (default 127.0.0.1:7781)
 */
import { StdioServerTransport } from "@modelcontextprotocol/sdk/server/stdio.js";
import { AgentdClient, LmmsClient } from "./lmms-client.js";
import { createOperatorServer } from "./operator.js";

function intEnv(name: string, fallback: number): number {
  const raw = process.env[name];
  if (!raw) {
    return fallback;
  }
  const parsed = Number.parseInt(raw, 10);
  return Number.isFinite(parsed) && parsed > 0 ? parsed : fallback;
}

async function main(): Promise<void> {
  const client = new LmmsClient({
    host: process.env.LMMS_AGENT_HOST ?? "127.0.0.1",
    port: intEnv("LMMS_AGENT_PORT", 7777),
  });
  const agentd = new AgentdClient({
    host: process.env.LMMS_AGENTD_HOST ?? "127.0.0.1",
    port: intEnv("LMMS_AGENTD_PORT", 7781),
  });

  const server = createOperatorServer({ client, agentd });
  const transport = new StdioServerTransport();
  await server.connect(transport);
}

main().catch((err: unknown) => {
  console.error("lmms-operator:", err instanceof Error ? err.message : err);
  process.exit(1);
});
