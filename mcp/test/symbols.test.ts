import { describe, expect, it } from "vitest";
import { lookupSymbol } from "../src/symbols.js";
import { FIXTURE } from "./helpers.js";

describe("lookupSymbol", () => {
  it("finds the class declaration and member definitions", async () => {
    const result = await lookupSymbol(FIXTURE, "FixtureEngine", "definitions");
    const texts = result.definitions.map((m) => m.text);
    expect(texts.some((t) => t.includes("class FixtureEngine"))).toBe(true);
    expect(
      result.definitions.some((m) => m.text.includes("FixtureEngine::sampleRate")),
    ).toBe(true);
    expect(result.references).toHaveLength(0);
  });

  it("finds references across translation units", async () => {
    const result = await lookupSymbol(FIXTURE, "FixtureEngine", "references");
    expect(
      result.references.some(
        (m) => m.file === "src/gui/FixtureWindow.cpp" && m.text.includes("engine"),
      ),
    ).toBe(true);
    expect(result.definitions).toHaveLength(0);
  });

  it("combines both in all mode without duplicate lines", async () => {
    const result = await lookupSymbol(FIXTURE, "FixtureEngine", "all");
    expect(result.definitions.length).toBeGreaterThan(0);
    expect(result.references.length).toBeGreaterThan(0);
    const keys = new Set(result.references.map((m) => `${m.file}:${m.line}`));
    expect(keys.size).toBe(result.references.length);
  });

  it("rejects names that are not identifiers", async () => {
    await expect(lookupSymbol(FIXTURE, "a b", "all")).rejects.toThrow(/invalid symbol/);
    await expect(lookupSymbol(FIXTURE, "../../etc", "all")).rejects.toThrow(/invalid symbol/);
  });
});
