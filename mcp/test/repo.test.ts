import { promises as fs } from "node:fs";
import os from "node:os";
import path from "node:path";
import { afterAll, beforeAll, describe, expect, it } from "vitest";
import {
  RepoPathError,
  assertRealpathInsideRepo,
  findRepoRoot,
  listRepoDir,
  readRepoFile,
  resolveInRepo,
} from "../src/repo.js";
import { FIXTURE } from "./helpers.js";

describe("findRepoRoot", () => {
  it("walks up from a nested directory to the CMakeLists.txt marker", () => {
    expect(findRepoRoot(path.join(FIXTURE, "doc/wiki"), {})).toBe(FIXTURE);
  });

  it("honors an explicit LMMS_REPO override", () => {
    expect(findRepoRoot("/", { LMMS_REPO: FIXTURE })).toBe(FIXTURE);
  });

  it("returns null when no marker exists up the tree", async () => {
    const tmp = await fs.mkdtemp(path.join(os.tmpdir(), "lmms-mcp-root-"));
    expect(findRepoRoot(tmp, {})).toBeNull();
    await fs.rm(tmp, { recursive: true, force: true });
  });
});

describe("resolveInRepo", () => {
  it("joins relative paths under the root", () => {
    expect(resolveInRepo(FIXTURE, "src/core/FixtureEngine.h")).toBe(
      path.join(FIXTURE, "src/core/FixtureEngine.h"),
    );
  });

  it("rejects traversal outside the root", () => {
    expect(() => resolveInRepo(FIXTURE, "../escape")).toThrow(RepoPathError);
    expect(() => resolveInRepo(FIXTURE, "src/../../escape")).toThrow(RepoPathError);
    expect(() => resolveInRepo(FIXTURE, "..")).toThrow(RepoPathError);
  });

  it("rejects absolute paths outside the root", () => {
    expect(() => resolveInRepo(FIXTURE, "/etc/passwd")).toThrow(RepoPathError);
  });

  it("allows absolute paths inside the root", () => {
    expect(resolveInRepo(FIXTURE, path.join(FIXTURE, "README.md"))).toBe(
      path.join(FIXTURE, "README.md"),
    );
  });
});

describe("assertRealpathInsideRepo", () => {
  let outside: string;
  let link: string;

  beforeAll(async () => {
    outside = await fs.mkdtemp(path.join(os.tmpdir(), "lmms-mcp-link-"));
    await fs.writeFile(path.join(outside, "secret.txt"), "top secret");
    link = path.join(FIXTURE, "escape-link");
    await fs.symlink(outside, link, "dir");
  });

  afterAll(async () => {
    await fs.rm(link, { recursive: true, force: true });
    await fs.rm(outside, { recursive: true, force: true });
  });

  it("rejects a symlink that resolves outside the root", async () => {
    await expect(assertRealpathInsideRepo(FIXTURE, link)).rejects.toThrow(RepoPathError);
  });

  it("accepts ordinary files inside the root", async () => {
    await expect(
      assertRealpathInsideRepo(FIXTURE, path.join(FIXTURE, "README.md")),
    ).resolves.toBeUndefined();
  });
});

describe("readRepoFile", () => {
  it("reads a whole file with line counts", async () => {
    const f = await readRepoFile(FIXTURE, "CMakeLists.txt");
    expect(f.file).toBe("CMakeLists.txt");
    expect(f.content).toContain("cmake_minimum_required(VERSION 3.20)");
    expect(f.totalLines).toBe(7);
    expect(f.truncated).toBe(false);
  });

  it("returns a 1-based inclusive line range", async () => {
    const f = await readRepoFile(FIXTURE, "CMakeLists.txt", 1, 2);
    expect(f.content).toBe(
      "cmake_minimum_required(VERSION 3.20)\nproject(fixturelmms)",
    );
  });

  it("clamps an oversized range to the file length", async () => {
    const f = await readRepoFile(FIXTURE, "CMakeLists.txt", 5, 999);
    expect(f.content).toContain("FIXTURE_NOQUOTE");
    expect(f.content.split("\n").length).toBeLessThanOrEqual(3);
  });

  it("rejects an inverted range", async () => {
    await expect(readRepoFile(FIXTURE, "CMakeLists.txt", 5, 2)).rejects.toThrow(
      RepoPathError,
    );
  });

  it("rejects missing files and directories", async () => {
    await expect(readRepoFile(FIXTURE, "nope.txt")).rejects.toThrow(RepoPathError);
    await expect(readRepoFile(FIXTURE, "src")).rejects.toThrow(RepoPathError);
  });

  it("refuses binary files", async () => {
    const bin = path.join(FIXTURE, "bin.dat");
    await fs.writeFile(bin, Buffer.from([0x00, 0x01, 0x02, 0xff]));
    try {
      await expect(readRepoFile(FIXTURE, "bin.dat")).rejects.toThrow(/binary/);
    } finally {
      await fs.rm(bin, { force: true });
    }
  });

  it("never reads outside the root even through traversal", async () => {
    await expect(readRepoFile(FIXTURE, "../README.md")).rejects.toThrow(RepoPathError);
  });
});

describe("listRepoDir", () => {
  it("lists directories first and hides .git", async () => {
    const entries = await listRepoDir(FIXTURE);
    const names = entries.map((e) => e.name);
    expect(names).not.toContain(".git");
    expect(names).toContain("src");
    expect(names).toContain("plugins");
    expect(entries.find((e) => e.name === "src")?.type).toBe("dir");
    expect(entries.find((e) => e.name === "README.md")?.type).toBe("file");
    expect(entries.find((e) => e.name === "src")!.mtime).toBeTruthy();
  });

  it("lists a subdirectory", async () => {
    const entries = await listRepoDir(FIXTURE, "plugins");
    expect(entries.map((e) => e.name).sort()).toEqual([
      "FakeSynth",
      "FakeVerb",
      "NoClass",
    ]);
  });

  it("rejects traversal", async () => {
    await expect(listRepoDir(FIXTURE, "../..")).rejects.toThrow(RepoPathError);
  });
});
