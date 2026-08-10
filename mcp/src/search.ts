/**
 * Repository-scoped source search. Prefers ripgrep when installed; falls back
 * to a bounded recursive walker so the server still works without it.
 */
import { spawn } from "node:child_process";
import { promises as fs } from "node:fs";
import path from "node:path";
import { resolveInRepo } from "./repo.js";

export interface SearchMatch {
  /** Path relative to the repository root. */
  file: string;
  line: number;
  text: string;
}

export interface SearchOptions {
  pattern: string;
  /** Optional in-repo directory to restrict the search to. */
  subPath?: string;
  maxResults?: number;
  caseSensitive?: boolean;
}

export interface SearchOutcome {
  matches: SearchMatch[];
  truncated: boolean;
  usedFallback: boolean;
}

export type SearchRunner = (
  root: string,
  pattern: string,
  subPath: string,
  max: number,
  caseSensitive: boolean,
) => Promise<SearchMatch[]>;

/** Directories excluded from search by name at any depth. */
export const DEFAULT_EXCLUDES = [
  ".git",
  "build",
  "dist",
  "node_modules",
  ".cache",
  "cmake-build-debug",
  "cmake-build-release",
];

const RG_TIMEOUT_MS = 15_000;
const MAX_FILE_BYTES = 512 * 1024;
const MAX_WALKED_FILES = 4000;
const MAX_WALK_DEPTH = 24;

function parseRgLine(line: string): SearchMatch | null {
  const m = /^(.+?):(\d+):(.*)$/s.exec(line);
  if (!m) {
    return null;
  }
  return { file: m[1] ?? "", line: Number(m[2]), text: m[3] ?? "" };
}

async function runRipgrep(
  root: string,
  pattern: string,
  subPath: string,
  max: number,
  caseSensitive: boolean,
): Promise<SearchMatch[]> {
  const args = [
    "-n",
    "--no-heading",
    "--color",
    "never",
    caseSensitive ? "-s" : "-S",
    "--max-count",
    "20",
    "--max-filesize",
    "2M",
    ...DEFAULT_EXCLUDES.flatMap((g) => ["-g", `!${g}`]),
    "--",
    pattern,
    subPath,
  ];
  const child = spawn("rg", args, {
    cwd: root,
    stdio: ["ignore", "pipe", "pipe"],
  });
  const timer = setTimeout(() => child.kill("SIGKILL"), RG_TIMEOUT_MS);
  let stdout = "";
  let stderr = "";
  child.stdout.setEncoding("utf8");
  child.stderr.setEncoding("utf8");
  child.stdout.on("data", (c: string) => {
    stdout += c;
  });
  child.stderr.on("data", (c: string) => {
    stderr += c;
  });
  const code = await new Promise<number | null>((resolve) => {
    child.on("close", resolve);
    child.on("error", (err) => {
      (err as NodeJS.ErrnoException & { cmdNotFound?: boolean }).cmdNotFound = true;
      resolve(-1);
    });
  });
  clearTimeout(timer);
  if (code === null) {
    throw new Error("search timed out");
  }
  if (code === -1) {
    const err = new Error("ripgrep unavailable") as NodeJS.ErrnoException;
    err.code = "ENOENT";
    throw err;
  }
  if (code === 2) {
    throw new Error(`invalid search pattern: ${stderr.trim() || pattern}`);
  }
  if (code === 1) {
    return []; // no matches
  }
  const matches: SearchMatch[] = [];
  for (const line of stdout.split("\n")) {
    if (matches.length >= max) {
      break;
    }
    const parsed = parseRgLine(line);
    if (parsed && parsed.file !== "") {
      matches.push({ ...parsed, file: path.relative(root, parsed.file) });
    }
  }
  return matches;
}

const SKIP_EXT: Record<string, true> = {
  ".png": true, ".jpg": true, ".jpeg": true, ".gif": true, ".bmp": true,
  ".ico": true, ".mp3": true, ".wav": true, ".ogg": true, ".flac": true,
  ".mid": true, ".zip": true, ".gz": true, ".bz2": true, ".xz": true,
  ".7z": true, ".pdf": true, ".qm": true, ".svg": true, ".ttf": true,
  ".woff": true, ".woff2": true, ".qmlc": true, ".obj": true, ".o": true,
  ".a": true, ".so": true, ".dylib": true, ".dll": true, ".exe": true,
  ".lib": true, ".mmp": true, ".mmpt": true, ".mmpz": true,
};

function shouldSkipDir(name: string): boolean {
  return name.startsWith(".") || DEFAULT_EXCLUDES.includes(name);
}

/**
 * Bounded regex walker used when ripgrep is not installed. Skips binaries,
 * hidden and excluded directories, and stops after MAX_WALKED_FILES files.
 */
export async function fallbackWalk(
  root: string,
  pattern: string,
  subPath: string,
  max: number,
  caseSensitive: boolean,
): Promise<SearchMatch[]> {
  let re: RegExp;
  try {
    re = new RegExp(pattern, caseSensitive ? "" : "i");
  } catch {
    throw new Error(`invalid search pattern: ${pattern}`);
  }
  const matches: SearchMatch[] = [];
  let visited = 0;

  const walk = async (dir: string, depth: number): Promise<void> => {
    if (depth > MAX_WALK_DEPTH || matches.length >= max || visited >= MAX_WALKED_FILES) {
      return;
    }
    let entries;
    try {
      entries = await fs.readdir(dir, { withFileTypes: true });
    } catch {
      return;
    }
    for (const entry of entries) {
      if (matches.length >= max || visited >= MAX_WALKED_FILES) {
        return;
      }
      if (entry.isDirectory()) {
        if (entry.isSymbolicLink() || shouldSkipDir(entry.name)) {
          continue;
        }
        await walk(path.join(dir, entry.name), depth + 1);
        continue;
      }
      if (!entry.isFile() || entry.isSymbolicLink()) {
        continue;
      }
      const ext = path.extname(entry.name).toLowerCase();
      if (SKIP_EXT[ext]) {
        continue;
      }
      const abs = path.join(dir, entry.name);
      let buf: Buffer;
      try {
        const stat = await fs.stat(abs);
        if (stat.size > MAX_FILE_BYTES) {
          continue;
        }
        buf = await fs.readFile(abs);
      } catch {
        continue;
      }
      visited += 1;
      if (buf.includes(0)) {
        continue;
      }
      const lines = buf.toString("utf8").split("\n");
      for (let i = 0; i < lines.length && matches.length < max; i += 1) {
        if (re.test(lines[i] ?? "")) {
          matches.push({
            file: path.relative(root, abs),
            line: i + 1,
            text: (lines[i] ?? "").slice(0, 500),
          });
        }
      }
    }
  };

  await walk(subPath, 0);
  return matches;
}

/**
 * Run a repository-scoped search. `primary` is injectable for tests; the
 * default is ripgrep, and searchRepo falls back to the bounded walker when
 * ripgrep is not installed (reported via usedFallback).
 */
export async function searchRepo(
  root: string,
  opts: SearchOptions,
  primary: SearchRunner = runRipgrep,
): Promise<SearchOutcome> {
  const max = Math.max(1, Math.min(500, opts.maxResults ?? 100));
  const sub = opts.subPath ? resolveInRepo(root, opts.subPath) : root;
  try {
    const matches = await primary(root, opts.pattern, sub, max, opts.caseSensitive ?? false);
    return { matches, truncated: matches.length >= max, usedFallback: false };
  } catch (err) {
    if ((err as NodeJS.ErrnoException).code === "ENOENT") {
      const matches = await fallbackWalk(root, opts.pattern, sub, max, opts.caseSensitive ?? false);
      return { matches, truncated: matches.length >= max, usedFallback: true };
    }
    throw err;
  }
}
