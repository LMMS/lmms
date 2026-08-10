import { describe, expect, it } from "vitest";
import { fallbackWalk, searchRepo } from "../src/search.js";
import { FIXTURE } from "./helpers.js";

const rgMissing = async () => {
  const err = new Error("ripgrep unavailable") as NodeJS.ErrnoException;
  err.code = "ENOENT";
  throw err;
};

describe("searchRepo with ripgrep", () => {
  it("finds matches with repo-relative paths and snippets", async () => {
    const { matches, usedFallback } = await searchRepo(FIXTURE, {
      pattern: "FixtureEngine",
      caseSensitive: true,
    });
    expect(usedFallback).toBe(false);
    const files = matches.map((m) => m.file).sort();
    expect(files).toContain("src/core/FixtureEngine.h");
    expect(files).toContain("src/core/FixtureEngine.cpp");
    expect(files).toContain("src/gui/FixtureWindow.cpp");
    const classHit = matches.find((m) => m.text.includes("class FixtureEngine"));
    expect(classHit).toBeTruthy();
    expect(classHit!.line).toBeGreaterThan(0);
  });

  it("respects case sensitivity", async () => {
    const sensitive = await searchRepo(FIXTURE, {
      pattern: "fixtureengine",
      caseSensitive: true,
    });
    expect(sensitive.matches).toHaveLength(0);
    const insensitive = await searchRepo(FIXTURE, {
      pattern: "fixtureengine",
      caseSensitive: false,
    });
    expect(insensitive.matches.length).toBeGreaterThan(0);
  });

  it("scopes to a subdirectory", async () => {
    const { matches } = await searchRepo(FIXTURE, {
      pattern: "FixtureEngine",
      subPath: "plugins",
      caseSensitive: true,
    });
    expect(matches).toHaveLength(0);
  });

  it("never searches excluded directories like node_modules", async () => {
    const { matches } = await searchRepo(FIXTURE, {
      pattern: "FixtureEngine",
      caseSensitive: true,
    });
    expect(matches.some((m) => m.file.startsWith("node_modules"))).toBe(false);
  });

  it("caps results and reports truncation", async () => {
    const { matches, truncated } = await searchRepo(FIXTURE, {
      pattern: ".",
      maxResults: 5,
    });
    expect(matches.length).toBe(5);
    expect(truncated).toBe(true);
  });

  it("reports an invalid pattern as an error", async () => {
    await expect(
      searchRepo(FIXTURE, { pattern: "(", caseSensitive: true }),
    ).rejects.toThrow(/invalid search pattern/);
  });
});

describe("searchRepo fallback without ripgrep", () => {
  it("falls back to the walker and reports it", async () => {
    const { matches, usedFallback } = await searchRepo(
      FIXTURE,
      { pattern: "FixtureEngine", caseSensitive: true },
      rgMissing,
    );
    expect(usedFallback).toBe(true);
    expect(matches.some((m) => m.file === "src/core/FixtureEngine.h")).toBe(true);
    expect(matches.some((m) => m.file.startsWith("node_modules"))).toBe(false);
  });

  it("throws on an invalid regex in the walker", async () => {
    await expect(
      searchRepo(FIXTURE, { pattern: "(" }, rgMissing),
    ).rejects.toThrow(/invalid search pattern/);
  });
});

describe("fallbackWalk", () => {
  it("finds matches directly with relative paths", async () => {
    const matches = await fallbackWalk(FIXTURE, "class FakeSynth", FIXTURE, 100, true);
    expect(matches).toHaveLength(1);
    expect(matches[0]!.file).toBe("plugins/FakeSynth/FakeSynth.h");
  });
});
