import { describe, expect, it } from "vitest";
import { catalogPlugins } from "../src/plugins.js";
import { FIXTURE } from "./helpers.js";

describe("catalogPlugins", () => {
  it("lists every plugin directory with sources and CMake presence", async () => {
    const { plugins, total } = await catalogPlugins(FIXTURE);
    expect(total).toBe(3);
    const names = plugins.map((p) => p.name).sort();
    expect(names).toEqual(["FakeSynth", "FakeVerb", "NoClass"]);
    for (const p of plugins) {
      expect(p.hasCMakeLists).toBe(true);
      expect(p.sourceFiles.length).toBeGreaterThan(0);
    }
  });

  it("classifies instrument and effect plugins from their class hierarchy", async () => {
    const { plugins } = await catalogPlugins(FIXTURE);
    const byName = Object.fromEntries(plugins.map((p) => [p.name, p]));
    expect(byName["FakeSynth"]!.kind).toBe("instrument");
    expect(byName["FakeVerb"]!.kind).toBe("effect");
    expect(byName["NoClass"]!.kind).toBe("unknown");
  });

  it("records the class declaration lines for classified plugins", async () => {
    const { plugins } = await catalogPlugins(FIXTURE);
    const synth = plugins.find((p) => p.name === "FakeSynth")!;
    expect(synth.classDeclarations.some((c) => c.includes("class FakeSynth"))).toBe(true);
  });

  it("filters by kind", async () => {
    const { plugins } = await catalogPlugins(FIXTURE, { kind: "effect" });
    expect(plugins).toHaveLength(1);
    expect(plugins[0]!.name).toBe("FakeVerb");
  });

  it("filters by name", async () => {
    const { plugins } = await catalogPlugins(FIXTURE, { name: "NoClass" });
    expect(plugins).toHaveLength(1);
    expect(plugins[0]!.kind).toBe("unknown");
  });

  it("returns an empty catalog for a checkout without plugins", async () => {
    const { promises: fs } = await import("node:fs");
    const os = await import("node:os");
    const path = await import("node:path");
    const tmp = await fs.mkdtemp(path.join(os.tmpdir(), "lmms-mcp-plug-"));
    try {
      const { plugins, total } = await catalogPlugins(tmp);
      expect(total).toBe(0);
      expect(plugins).toEqual([]);
    } finally {
      await fs.rm(tmp, { recursive: true, force: true });
    }
  });
});
