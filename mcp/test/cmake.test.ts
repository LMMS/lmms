import { describe, expect, it } from "vitest";
import { parseBuildInfo } from "../src/cmake.js";
import { FIXTURE } from "./helpers.js";

describe("parseBuildInfo", () => {
  it("extracts the CMake minimum version and project name", () => {
    const info = parseBuildInfo(FIXTURE);
    expect(info.cmakeMinimum).toBe("3.20");
    expect(info.project).toBe("fixturelmms");
  });

  it("extracts quoted and unquoted option descriptions with defaults", () => {
    const info = parseBuildInfo(FIXTURE);
    const byName = Object.fromEntries(info.options.map((o) => [o.name, o]));
    expect(byName["FIXTURE_FEATURE"]).toMatchObject({
      description: "Enable the fixture feature",
      default: "ON",
    });
    expect(byName["FIXTURE_EXTRA"]).toMatchObject({
      description: "Enable extra fixture bits",
      default: "OFF",
    });
    expect(byName["FIXTURE_NOQUOTE"]).toMatchObject({
      description: "Enable an unquoted description",
      default: "ON",
    });
  });

  it("deduplicates options by name and sorts them", () => {
    const info = parseBuildInfo(FIXTURE);
    const names = info.options.map((o) => o.name);
    expect(new Set(names).size).toBe(names.length);
    expect([...names].sort()).toEqual(names);
  });

  it("reports which files were scanned", () => {
    const info = parseBuildInfo(FIXTURE);
    expect(info.filesScanned).toContain("CMakeLists.txt");
    expect(info.filesScanned).toContain("src/CMakeLists.txt");
  });

  it("handles a checkout without CMake files", async () => {
    const { promises: fs } = await import("node:fs");
    const os = await import("node:os");
    const path = await import("node:path");
    const tmp = await fs.mkdtemp(path.join(os.tmpdir(), "lmms-mcp-cmake-"));
    try {
      const info = parseBuildInfo(tmp);
      expect(info.cmakeMinimum).toBeNull();
      expect(info.project).toBeNull();
      expect(info.options).toEqual([]);
    } finally {
      await fs.rm(tmp, { recursive: true, force: true });
    }
  });
});
