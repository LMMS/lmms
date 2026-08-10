#!/usr/bin/env node
/**
 * lmms-mcp entrypoint: stdio transport for local MCP clients.
 * Run from inside the LMMS checkout, or point LMMS_REPO at it.
 */
import path from "node:path";
import { fileURLToPath } from "node:url";
import { StdioServerTransport } from "@modelcontextprotocol/sdk/server/stdio.js";
import { createServer } from "./server.js";
import { findRepoRoot } from "./repo.js";

async function main(): Promise<void> {
  const here = path.dirname(fileURLToPath(import.meta.url));
  const root = findRepoRoot(process.cwd()) ?? findRepoRoot(here);
  if (!root) {
    console.error(
      "lmms-mcp: cannot locate the LMMS repository root. Run from inside the checkout or set LMMS_REPO.",
    );
    process.exit(1);
  }
  const server = createServer({ repoRoot: root });
  const transport = new StdioServerTransport();
  await server.connect(transport);
}

main().catch((err: unknown) => {
  console.error("lmms-mcp:", err instanceof Error ? err.message : err);
  process.exit(1);
});
