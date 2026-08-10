/**
 * Repository root resolution and path safety for the LMMS MCP server.
 *
 * Every tool that touches the filesystem goes through resolveInRepo /
 * assertRealpathInsideRepo so a client can never read outside the repository
 * checkout, even through `..` traversal or symlinks.
 */
import { promises as fs } from "node:fs";
import { accessSync } from "node:fs";
import path from "node:path";

/** Marker file that identifies a checkout as the LMMS repository root. */
export const REPO_MARKER = "CMakeLists.txt";

/** Default cap for whole-file reads; line ranges bypass it. */
export const MAX_FILE_LINES = 4000;

/** Raised for any path that escapes the repository root or names a bad file. */
export class RepoPathError extends Error {}

function fsExistsSync(p: string): boolean {
  try {
    accessSync(p);
    return true;
  } catch {
    return false;
  }
}

/**
 * Locate the repository root: explicit LMMS_REPO wins, otherwise walk up from
 * the given start directory looking for the CMakeLists.txt marker.
 */
export function findRepoRoot(
  startDir: string,
  env: NodeJS.ProcessEnv = process.env,
): string | null {
  const explicit = env.LMMS_REPO;
  if (explicit) {
    return path.resolve(explicit);
  }
  let dir = path.resolve(startDir);
  for (let depth = 0; depth < 64; depth += 1) {
    if (fsExistsSync(path.join(dir, REPO_MARKER))) {
      return dir;
    }
    const parent = path.dirname(dir);
    if (parent === dir) {
      return null;
    }
    dir = parent;
  }
  return null;
}

/**
 * Resolve a client-supplied path (relative or absolute) to a path inside the
 * repository root, rejecting any lexical escape via `..` or an absolute path
 * outside the root.
 */
export function resolveInRepo(root: string, rel: string): string {
  const abs = path.resolve(root, rel);
  const relPath = path.relative(root, abs);
  if (
    relPath === ".." ||
    relPath.startsWith(`..${path.sep}`) ||
    path.isAbsolute(relPath)
  ) {
    throw new RepoPathError(`path escapes the repository root: ${rel}`);
  }
  return abs;
}

/**
 * Verify a resolved path does not escape the root through symlinks. Missing
 * files are allowed (lexical containment already holds); the check only runs
 * when both paths exist.
 */
export async function assertRealpathInsideRepo(
  root: string,
  abs: string,
): Promise<void> {
  try {
    const [realAbs, realRoot] = await Promise.all([
      fs.realpath(abs),
      fs.realpath(root),
    ]);
    const rel = path.relative(realRoot, realAbs);
    if (
      rel === ".." ||
      rel.startsWith(`..${path.sep}`) ||
      path.isAbsolute(rel)
    ) {
      throw new RepoPathError(`path resolves outside the repository root: ${abs}`);
    }
  } catch (err) {
    if (err instanceof RepoPathError) {
      throw err;
    }
    // ENOENT/ENOTDIR etc. mean the path does not exist; lexical check stands.
  }
}

export interface RepoFile {
  file: string;
  content: string;
  totalLines: number;
  truncated: boolean;
}

/**
 * Read a file inside the repository. Supports a 1-based inclusive line range;
 * without one, output is capped at MAX_FILE_LINES with `truncated` set.
 * Binary files are refused.
 */
export async function readRepoFile(
  root: string,
  rel: string,
  startLine?: number,
  endLine?: number,
): Promise<RepoFile> {
  const abs = resolveInRepo(root, rel);
  await assertRealpathInsideRepo(root, abs);
  let stat;
  try {
    stat = await fs.stat(abs);
  } catch {
    throw new RepoPathError(`no such file: ${rel}`);
  }
  if (stat.isDirectory()) {
    throw new RepoPathError(`not a file: ${rel}`);
  }
  const buf = await fs.readFile(abs);
  if (buf.includes(0)) {
    throw new RepoPathError(`binary file: ${rel}`);
  }
  const text = buf.toString("utf8");
  const lines = text.split("\n");
  const totalLines = lines.length;

  let slice = lines;
  let truncated = false;
  if (startLine !== undefined || endLine !== undefined) {
    const start = Math.max(1, startLine ?? 1);
    const end = Math.min(totalLines, endLine ?? totalLines);
    if (start > end) {
      throw new RepoPathError(
        `invalid line range ${startLine}-${endLine} for ${rel} (${totalLines} lines)`,
      );
    }
    slice = lines.slice(start - 1, end);
  } else if (totalLines > MAX_FILE_LINES) {
    slice = lines.slice(0, MAX_FILE_LINES);
    truncated = true;
  }
  return { file: rel, content: slice.join("\n"), totalLines, truncated };
}

export interface DirEntry {
  name: string;
  type: "dir" | "file" | "other";
  size: number;
  mtime: string;
}

/** List a directory inside the repository, directories first, skipping .git. */
export async function listRepoDir(root: string, rel = "."): Promise<DirEntry[]> {
  const abs = resolveInRepo(root, rel);
  await assertRealpathInsideRepo(root, abs);
  let dirents;
  try {
    dirents = await fs.readdir(abs, { withFileTypes: true });
  } catch {
    throw new RepoPathError(`no such directory: ${rel}`);
  }
  const entries: DirEntry[] = [];
  for (const d of dirents) {
    if (d.name === ".git") {
      continue;
    }
    let size = 0;
    let mtime = "";
    try {
      const s = await fs.stat(path.join(abs, d.name));
      size = s.size;
      mtime = s.mtime.toISOString();
    } catch {
      // dangling symlink or unreadable entry; report what we know
    }
    entries.push({
      name: d.name,
      type: d.isDirectory() ? "dir" : d.isFile() ? "file" : "other",
      size,
      mtime,
    });
  }
  entries.sort((a, b) =>
    a.type === b.type ? a.name.localeCompare(b.name) : a.type === "dir" ? -1 : 1,
  );
  return entries;
}
