/**
 * LMMS plugin catalog: enumerates plugins/ subdirectories, classifies each as
 * instrument, effect, tool, or unknown from its class hierarchy, and can
 * return per-plugin detail (sources and class declarations).
 */
import { promises as fs } from "node:fs";
import path from "node:path";
import { searchRepo, type SearchRunner } from "./search.js";

export type PluginKind = "instrument" | "effect" | "tool" | "unknown";

export interface PluginInfo {
  name: string;
  kind: PluginKind;
  hasCMakeLists: boolean;
  sourceFiles: string[];
  classDeclarations: string[];
}

export interface PluginCatalogOptions {
  name?: string;
  kind?: PluginKind;
}

const SOURCE_EXT: Record<string, true> = {
  ".cpp": true,
  ".cc": true,
  ".cxx": true,
  ".c": true,
  ".h": true,
  ".hpp": true,
  ".hh": true,
  ".hxx": true,
  ".ipp": true,
};

const KIND_PATTERN = (kind: PluginKind) =>
  kind === "instrument"
    ? "public\\s+(?:virtual\\s+)?Instrument\\b"
    : kind === "effect"
      ? "public\\s+(?:virtual\\s+)?Effect\\b"
      : "public\\s+(?:virtual\\s+)?(?:Plugin|Tool)\\b";

const CLASS_PATTERN = "class\\s+\\w+\\s*:\\s*public\\s+\\w+";

async function listPluginNames(root: string): Promise<string[]> {
  let dirents;
  try {
    dirents = await fs.readdir(path.join(root, "plugins"), { withFileTypes: true });
  } catch {
    return [];
  }
  return dirents
    .filter((d) => d.isDirectory() && !d.name.startsWith("."))
    .map((d) => d.name)
    .sort();
}

async function collectSources(pluginDir: string, depth = 0): Promise<string[]> {
  if (depth > 4) {
    return [];
  }
  let entries;
  try {
    entries = await fs.readdir(pluginDir, { withFileTypes: true });
  } catch {
    return [];
  }
  const files: string[] = [];
  for (const e of entries) {
    if (e.name === ".git" || e.name === "build" || e.name === "node_modules") {
      continue;
    }
    if (e.isDirectory() && !e.isSymbolicLink()) {
      files.push(...(await collectSources(path.join(pluginDir, e.name), depth + 1)));
    } else if (e.isFile() && SOURCE_EXT[path.extname(e.name).toLowerCase()]) {
      files.push(path.join(pluginDir, e.name));
    }
  }
  return files.sort();
}

/**
 * Classify one plugin directory by searching its sources for the LMMS base
 * classes. Returns the strongest match (instrument > effect > tool).
 */
export async function classifyPlugin(
  root: string,
  pluginName: string,
  runner?: SearchRunner,
): Promise<{ kind: PluginKind; classes: string[] }> {
  const dir = path.join(root, "plugins", pluginName);
  const order: PluginKind[] = ["instrument", "effect", "tool"];
  for (const kind of order) {
    const outcome = await searchRepo(
      root,
      { pattern: KIND_PATTERN(kind), subPath: path.join("plugins", pluginName), maxResults: 5, caseSensitive: true },
      runner,
    );
    if (outcome.matches.length > 0) {
      const classesOutcome = await searchRepo(
        root,
        { pattern: CLASS_PATTERN, subPath: path.join("plugins", pluginName), maxResults: 8, caseSensitive: true },
        runner,
      );
      return { kind, classes: classesOutcome.matches.map((m) => `${m.file}:${m.line}: ${m.text}`) };
    }
  }
  return { kind: "unknown", classes: [] };
}

/**
 * Catalog every plugin in the checkout, optionally filtered by name or kind.
 * `runner` is injectable for tests.
 */
export async function catalogPlugins(
  root: string,
  opts: PluginCatalogOptions = {},
  runner?: SearchRunner,
): Promise<{ plugins: PluginInfo[]; total: number }> {
  const names = await listPluginNames(root);
  const plugins: PluginInfo[] = [];
  for (const name of names) {
    if (opts.name && name !== opts.name) {
      continue;
    }
    const pluginDir = path.join(root, "plugins", name);
    const [sources, hasCMakeLists, classification] = await Promise.all([
      collectSources(pluginDir),
      fs
        .access(path.join(pluginDir, "CMakeLists.txt"))
        .then(() => true)
        .catch(() => false),
      classifyPlugin(root, name, runner),
    ]);
    if (opts.kind && classification.kind !== opts.kind) {
      continue;
    }
    plugins.push({
      name,
      kind: classification.kind,
      hasCMakeLists,
      sourceFiles: sources.map((s) => path.relative(root, s)),
      classDeclarations: classification.classes,
    });
  }
  return { plugins, total: plugins.length };
}
