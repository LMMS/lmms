import { describe, expect, it } from "vitest";
import { RESOURCES } from "../src/resources.js";
import { FIXTURE } from "./helpers.js";

describe("resources", () => {
  it("exposes the documented resource set", () => {
    expect(RESOURCES.map((r) => r.uri).sort()).toEqual(
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
  });

  it("overview describes the layout and README", async () => {
    const text = await RESOURCES[0]!.load(FIXTURE);
    expect(text).toContain("**src/**");
    expect(text).toContain("Fixture LMMS");
  });

  it("build reports CMake facts and install notes", async () => {
    const def = RESOURCES.find((r) => r.uri === "lmms://build")!;
    const text = await def.load(FIXTURE);
    expect(text).toContain("3.20");
    expect(text).toContain("FIXTURE_FEATURE");
    expect(text).toContain("Building-fixture.md");
    expect(text).toContain("cmake -S . -B build");
  });

  it("architecture degrades gracefully without the wiki doc", async () => {
    const def = RESOURCES.find((r) => r.uri === "lmms://architecture")!;
    const text = await def.load(FIXTURE);
    expect(text).toContain("No `doc/wiki/LMMS-Architecture.md`");
    expect(text).toContain("## src/");
  });

  it("coding-conventions reports the tooling files present", async () => {
    const def = RESOURCES.find((r) => r.uri === "lmms://coding-conventions")!;
    const text = await def.load(FIXTURE);
    expect(text).toContain(".clang-format");
  });

  it("plugins resource lists every plugin by kind", async () => {
    const def = RESOURCES.find((r) => r.uri === "lmms://plugins")!;
    const text = await def.load(FIXTURE);
    expect(text).toContain("Total: 3");
    expect(text).toContain("Instruments (1)");
    expect(text).toContain("FakeSynth");
    expect(text).toContain("Effects (1)");
    expect(text).toContain("FakeVerb");
  });

  it("documentation indexes doc and doc/wiki", async () => {
    const def = RESOURCES.find((r) => r.uri === "lmms://documentation")!;
    const text = await def.load(FIXTURE);
    expect(text).toContain("Building-fixture.md");
  });

  it("agents resource degrades gracefully without lmmsagent", async () => {
    const def = RESOURCES.find((r) => r.uri === "lmms://agents")!;
    const text = await def.load(FIXTURE);
    expect(text).toContain("No lmmsagent/ directory");
  });
});
