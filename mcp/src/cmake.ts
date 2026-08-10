/**
 * Build-system facts extracted from the repository's CMake files: minimum
 * CMake version, project name, and every `option(...)` switch found in the
 * build definition files.
 */
import { accessSync, readdirSync, readFileSync } from "node:fs";
import path from "node:path";

export type CmakeOption = {
  name: string;
  description: string;
  default: string;
  file: string;
};

export type BuildInfo = {
  cmakeMinimum: string | null;
  project: string | null;
  options: CmakeOption[];
  filesScanned: string[];
};

const OPTION_RE =
  /option\s*\(\s*([A-Za-z_][A-Za-z0-9_]*)\s+(?:"((?:[^"\\]|\\.)*)"|([A-Za-z0-9_][A-Za-z0-9_ -]*))\s+([A-Za-z0-9_]+)\s*\)/g;

const MINIMUM_RE = /cmake[_ ]?minimum[_ ]?required\s*\(\s*VERSION\s+([0-9]+(?:\.[0-9]+)*)/i;
const PROJECT_RE = /project\s*\(\s*([A-Za-z_][A-Za-z0-9_]*)/i;

/** CMake files whose options and version constraints matter for a build. */
export function buildInfoFiles(root: string): string[] {
  const candidates = [
    "CMakeLists.txt",
    "src/CMakeLists.txt",
    "plugins/CMakeLists.txt",
    "cmake/modules/PluginList.cmake",
    "cmake/modules/InstallDependencies.cmake",
  ];
  const files = candidates.filter((f) => {
    try {
      accessSync(path.join(root, f));
      return true;
    } catch {
      return false;
    }
  });
  try {
    const modules = readdirSync(path.join(root, "cmake/modules"));
    for (const name of modules.sort()) {
      if (name.endsWith(".cmake")) {
        files.push(path.join("cmake/modules", name));
      }
    }
  } catch {
    // no cmake/modules directory in this checkout
  }
  return files;
}

function unquote(s: string): string {
  return s.startsWith('"') && s.endsWith('"') ? s.slice(1, -1) : s;
}

/** Parse build facts from the checkout. Pure function over file contents. */
export function parseBuildInfo(root: string): BuildInfo {
  const files = buildInfoFiles(root);
  const options: CmakeOption[] = [];
  const seen = new Set<string>();
  let cmakeMinimum: string | null = null;
  let project: string | null = null;

  for (const file of files) {
    let text: string;
    try {
      text = readFileSync(path.join(root, file), "utf8");
    } catch {
      continue;
    }
    if (cmakeMinimum === null) {
      const m = MINIMUM_RE.exec(text);
      if (m?.[1]) {
        cmakeMinimum = m[1];
      }
    }
    if (project === null) {
      const m = PROJECT_RE.exec(text);
      if (m?.[1]) {
        project = m[1];
      }
    }
    for (const m of text.matchAll(OPTION_RE)) {
      const name = m[1];
      if (!name || seen.has(name)) {
        continue;
      }
      seen.add(name);
      const quoted = m[2];
      const bare = m[3];
      const defaultValue = m[4];
      if (defaultValue === undefined) {
        continue;
      }
      options.push({
        name,
        description: unquote(quoted ?? bare ?? ""),
        default: defaultValue,
        file,
      });
    }
  }
  options.sort((a, b) => a.name.localeCompare(b.name));
  return { cmakeMinimum, project, options, filesScanned: files };
}
