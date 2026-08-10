import { Client } from "@modelcontextprotocol/sdk/client/index.js";
import { InMemoryTransport } from "@modelcontextprotocol/sdk/inMemory.js";
import { describe, expect, it } from "vitest";
import { createServer } from "../src/server.js";
import { FIXTURE } from "./helpers.js";

const fetchStub = (async () =>
  new Response(
    JSON.stringify({
      number: 777,
      title: "Fixture issue",
      state: "open",
      user: { login: "fixtureuser" },
      labels: [],
      created_at: "2026-01-01T00:00:00Z",
      html_url: "https://github.com/lmms/lmms/issues/777",
    }),
    { status: 200, headers: { "Content-Type": "application/json" } },
  )) as typeof fetch;

async function makeClient() {
  const server = createServer({ repoRoot: FIXTURE, fetchFn: fetchStub });
  const [clientTransport, serverTransport] = InMemoryTransport.createLinkedPair();
  const client = new Client({ name: "lmms-mcp-test", version: "0.0.0" });
  await server.connect(serverTransport);
  await client.connect(clientTransport);
  return { client, server };
}

describe("lmms-mcp over the MCP protocol", () => {

  it("advertises all tools, resources, and prompts", async () => {
    const { client } = await makeClient();
    const tools = await client.listTools();
    expect(tools.tools.map((t) => t.name).sort()).toEqual(
      [
        "lmms_build_info",
        "lmms_issue_lookup",
        "lmms_list_directory",
        "lmms_plugin_catalog",
        "lmms_read_file",
        "lmms_search",
        "lmms_symbol_lookup",
      ].sort(),
    );
    for (const t of tools.tools) {
      expect(t.description?.length).toBeGreaterThan(10);
      expect(t.annotations?.readOnlyHint).toBe(true);
    }
    const resources = await client.listResources();
    expect(resources.resources.map((r) => r.uri).sort()).toEqual(
      [
        "lmms://agents",
        "lmms://architecture",
        "lmms://build",
        "lmms://coding-conventions",
        "lmms://documentation",
        "lmms://overview",
        "lmms://plugins",
      ].sort(),
    );
    const prompts = await client.listPrompts();
    expect(prompts.prompts.map((p) => p.name).sort()).toEqual(
      ["lmms-build-setup", "lmms-bug-investigation", "lmms-code-review", "lmms-new-plugin"].sort(),
    );
    await client.close();
  });

  it("lists the repository root through the protocol", async () => {
    const { client } = await makeClient();
    const res = await client.callTool({ name: "lmms_list_directory", arguments: {} });
    expect(res.isError).toBeFalsy();
    const entries = (res.structuredContent as { entries: { name: string; type: string }[] })
      .entries;
    expect(entries.map((e) => e.name)).toContain("src");
    expect(entries.map((e) => e.name)).toContain("plugins");
    expect(entries.find((e) => e.name === "src")!.type).toBe("dir");
    await client.close();
  });

  it("searches the checkout through the protocol", async () => {
    const { client } = await makeClient();
    const res = await client.callTool({
      name: "lmms_search",
      arguments: { pattern: "FixtureEngine", caseSensitive: true },
    });
    expect(res.isError).toBeFalsy();
    const matches = (res.structuredContent as { matches: { file: string }[] }).matches;
    expect(matches.some((m) => m.file === "src/core/FixtureEngine.h")).toBe(true);
    expect(matches.some((m) => m.file.startsWith("node_modules"))).toBe(false);
    await client.close();
  });

  it("reads files with line ranges through the protocol", async () => {
    const { client } = await makeClient();
    const res = await client.callTool({
      name: "lmms_read_file",
      arguments: { path: "CMakeLists.txt", startLine: 1, endLine: 2 },
    });
    expect(res.isError).toBeFalsy();
    expect(res.content[0]!.text).toContain("cmake_minimum_required(VERSION 3.20)");
    await client.close();
  });

  it("refuses files outside the repo through the protocol", async () => {
    const { client } = await makeClient();
    const res = await client.callTool({
      name: "lmms_read_file",
      arguments: { path: "../escape" },
    });
    expect(res.isError).toBe(true);
    expect(res.content[0]!.text).toContain("escapes");
    await client.close();
  });

  it("looks up symbols through the protocol", async () => {
    const { client } = await makeClient();
    const res = await client.callTool({
      name: "lmms_symbol_lookup",
      arguments: { symbol: "FixtureEngine", kind: "definitions" },
    });
    expect(res.isError).toBeFalsy();
    const defs = (res.structuredContent as { definitions: { text: string }[] }).definitions;
    expect(defs.some((d) => d.text.includes("class FixtureEngine"))).toBe(true);
    await client.close();
  });

  it("reports build info through the protocol", async () => {
    const { client } = await makeClient();
    const res = await client.callTool({ name: "lmms_build_info", arguments: {} });
    expect(res.isError).toBeFalsy();
    expect((res.structuredContent as { cmakeMinimum: string }).cmakeMinimum).toBe("3.20");
    await client.close();
  });

  it("catalogs plugins through the protocol", async () => {
    const { client } = await makeClient();
    const res = await client.callTool({ name: "lmms_plugin_catalog", arguments: {} });
    expect(res.isError).toBeFalsy();
    const plugins = (res.structuredContent as { plugins: { name: string; kind: string }[] })
      .plugins;
    expect(plugins).toHaveLength(3);
    expect(plugins.find((p) => p.name === "FakeSynth")!.kind).toBe("instrument");
    expect(plugins.find((p) => p.name === "FakeVerb")!.kind).toBe("effect");
    await client.close();
  });

  it("looks up GitHub issues through the protocol with an injected fetch", async () => {
    const { client } = await makeClient();
    const res = await client.callTool({
      name: "lmms_issue_lookup",
      arguments: { number: 777 },
    });
    expect(res.isError).toBeFalsy();
    const rows = (res.structuredContent as { rows: { number: number; title: string }[] }).rows;
    expect(rows).toHaveLength(1);
    expect(rows[0]!.number).toBe(777);
    expect(rows[0]!.title).toBe("Fixture issue");
    await client.close();
  });

  it("serves resources through the protocol", async () => {
    const { client } = await makeClient();
    const res = await client.readResource({ uri: "lmms://plugins" });
    expect(res.contents[0]!.text).toContain("FakeSynth");
    await client.close();
  });

  it("renders prompts and enforces required arguments", async () => {
    const { client } = await makeClient();
    const res = await client.getPrompt({
      name: "lmms-new-plugin",
      arguments: { name: "TestSynth", kind: "effect" },
    });
    expect(res.messages[0]!.content.text).toContain("plugins/TestSynth");
    expect(res.messages[0]!.content.text).toContain("Effect");
    await expect(client.getPrompt({ name: "lmms-new-plugin" })).rejects.toThrow();
    await client.close();
  });
});
