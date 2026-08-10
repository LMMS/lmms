/**
 * MCP resources: curated, always-fresh views of the LMMS checkout. Every
 * resource is generated from the live repository at read time so it never
 * goes stale.
 */
import { promises as fs } from "node:fs";
import path from "node:path";
import { parseBuildInfo } from "./cmake.js";
import { catalogPlugins } from "./plugins.js";
import { listRepoDir, readRepoFile, type DirEntry } from "./repo.js";
import type { SearchRunner } from "./search.js";

export interface ResourceDef {
  uri: string;
  name: string;
  description?: string;
  load: (root: string, runner?: SearchRunner) => Promise<string>;
}

const MAX_EXCERPT_LINES = 120;

async function excerpt(root: string, rel: string, maxLines = MAX_EXCERPT_LINES): Promise<string | null> {
  try {
    const f = await readRepoFile(root, rel);
    return f.content.split("\n").slice(0, maxLines).join("\n");
  } catch {
    return null;
  }
}

function dirListMarkdown(entries: DirEntry[]): string {
  if (entries.length === 0) {
    return "(empty)";
  }
  return entries.map((e) => `- ${e.type === "dir" ? `**${e.name}/**` : e.name}`).join("\n");
}

async function overview(root: string): Promise<string> {
  const entries = await listRepoDir(root, ".");
  const readme = (await excerpt(root, "README.md", 40)) ?? "(no README.md)";
  const extra: string[] = [];
  try {
    await fs.access(path.join(root, "AGENT_COMMAND_MAP.md"));
    extra.push("- `AGENT_COMMAND_MAP.md` - natural-language command map used by the agent layer");
  } catch {
    // not present in this checkout
  }
  return [
    "# LMMS Repository Overview",
    "",
    "## Top-level layout",
    dirListMarkdown(entries),
    ...extra,
    "",
    "## README",
    "",
    "```text",
    readme,
    "```",
    "",
  ].join("\n");
}

async function build(root: string): Promise<string> {
  const info = parseBuildInfo(root);
  const install = (await excerpt(root, "INSTALL.txt", 60)) ?? "(no INSTALL.txt)";
  const wikiDir = path.join(root, "doc/wiki");
  let buildingDocs = "(none found)";
  try {
    const names = (await fs.readdir(wikiDir))
      .filter((n) => /building|compile|build/i.test(n))
      .sort();
    if (names.length > 0) {
      buildingDocs = names.map((n) => `- doc/wiki/${n}`).join("\n");
    }
  } catch {
    // no doc/wiki
  }
  const options =
    info.options.length > 0
      ? info.options
          .map((o) => `- \`${o.name}\` (${o.default}) - ${o.description}`)
          .join("\n")
      : "(no option() switches found in the scanned CMake files)";
  return [
    "# LMMS Build",
    "",
    `- CMake minimum: \`${info.cmakeMinimum ?? "unknown"}\``,
    `- Project: \`${info.project ?? "unknown"}\``,
    "",
    "## CMake options",
    "",
    options,
    "",
    "## Building docs",
    "",
    buildingDocs,
    "",
    "## INSTALL.txt",
    "",
    "```text",
    install,
    "```",
    "",
  ].join("\n");
}

async function architecture(root: string): Promise<string> {
  const wiki = await excerpt(root, "doc/wiki/LMMS-Architecture.md", 300);
  if (wiki) {
    return ["# LMMS Architecture", "", wiki, ""].join("\n");
  }
  const src = await listRepoDir(root, "src");
  const inc = await listRepoDir(root, "include");
  return [
    "# LMMS Architecture",
    "",
    "No `doc/wiki/LMMS-Architecture.md` in this checkout; here is the layout.",
    "",
    "## src/",
    dirListMarkdown(src),
    "",
    "## include/",
    dirListMarkdown(inc),
    "",
  ].join("\n");
}

async function codingConventions(root: string): Promise<string> {
  const files = [".clang-format", ".clang-tidy", ".editorconfig", ".gitattributes"];
  const present: string[] = [];
  for (const f of files) {
    try {
      await fs.access(path.join(root, f));
      present.push(f);
    } catch {
      // absent
    }
  }
  const doxy = await excerpt(root, "doc/Doxyfile.in", 20);
  const notes: string[] = [];
  try {
    const names = (await fs.readdir(path.join(root, "doc/wiki"))).filter((n) =>
      /convention|style|contribute|coding/i.test(n),
    );
    notes.push(...names.map((n) => `- doc/wiki/${n}`));
  } catch {
    // no doc/wiki
  }
  return [
    "# LMMS Coding Conventions",
    "",
    `Tooling files present: ${present.length > 0 ? present.join(", ") : "(none)"}`,
    "",
    notes.length > 0 ? "## Style guidance" : "## Style guidance",
    "",
    notes.length > 0 ? notes.join("\n") : "(no dedicated style docs found; follow the dominant conventions in src/core and the .clang-format file)",
    "",
    doxy
      ? "## Doxygen configuration (first lines)\n\n```text\n" + doxy + "\n```\n"
      : "",
  ].join("\n");
}

async function pluginsResource(root: string, runner?: SearchRunner): Promise<string> {
  const { plugins } = await catalogPlugins(root, {}, runner);
  const byKind: Record<PluginKindGroup, string[]> = { instrument: [], effect: [], tool: [], unknown: [] };
  for (const p of plugins) {
    byKind[p.kind].push(p.name);
  }
  const section = (label: string, names: string[]): string =>
    names.length > 0 ? `## ${label} (${names.length})\n\n${names.map((n) => `- ${n}`).join("\n")}` : "";
  return [
    "# LMMS Plugins",
    "",
    `Total: ${plugins.length}`,
    "",
    section("Instruments", byKind.instrument),
    "",
    section("Effects", byKind.effect),
    "",
    section("Tools", byKind.tool),
    "",
    section("Unclassified", byKind.unknown),
    "",
  ].join("\n");
}

type PluginKindGroup = "instrument" | "effect" | "tool" | "unknown";

async function documentation(root: string): Promise<string> {
  const doc = await listRepoDir(root, "doc");
  let wiki = "(no doc/wiki)";
  try {
    const names = (await fs.readdir(path.join(root, "doc/wiki"))).sort();
    wiki = names.map((n) => `- doc/wiki/${n}`).join("\n");
  } catch {
    // no doc/wiki
  }
  return ["# LMMS Documentation", "", "## doc/", dirListMarkdown(doc), "", "## doc/wiki/", "", wiki, ""].join("\n");
}

async function agents(root: string): Promise<string> {
  const readme = (await excerpt(root, "lmmsagent/README.md", 100)) ?? null;
  if (!readme) {
    return "# LMMS Agent Layer\n\nNo lmmsagent/ directory in this checkout.\n";
  }
  const map = await excerpt(root, "AGENT_COMMAND_MAP.md", 40);
  return [
    "# LMMS Agent Layer",
    "",
    "## lmmsagent/README.md",
    "",
    "```text",
    readme,
    "```",
    "",
    map ? "## AGENT_COMMAND_MAP.md (first lines)\n\n```text\n" + map + "\n```\n" : "",
  ].join("\n");
}

/** All resources exposed by the server. */
export const RESOURCES: ResourceDef[] = [
  { uri: "lmms://overview", name: "LMMS repository overview", description: "Top-level layout and README", load: overview },
  { uri: "lmms://build", name: "LMMS build system", description: "CMake version, options, and build docs", load: build },
  { uri: "lmms://architecture", name: "LMMS architecture", description: "Source layout and architecture doc", load: architecture },
  { uri: "lmms://coding-conventions", name: "LMMS coding conventions", description: "Style tooling and guidance", load: codingConventions },
  { uri: "lmms://plugins", name: "LMMS plugin catalog", description: "All plugins by kind", load: pluginsResource },
  { uri: "lmms://documentation", name: "LMMS documentation index", description: "doc/ and doc/wiki contents", load: documentation },
  { uri: "lmms://agents", name: "LMMS agent layer", description: "lmmsagent stack overview", load: agents },
];
