/**
 * LMMS MCP server: wires the repository, search, symbol, build, plugin, and
 * GitHub modules into MCP tools, resources, and prompts. All tools are
 * read-only and safe to call from any agent.
 */
import path from "node:path";
import { fileURLToPath } from "node:url";
import { McpServer } from "@modelcontextprotocol/sdk/server/mcp.js";
import type { CallToolResult } from "@modelcontextprotocol/sdk/types.js";
import { z } from "zod";
import { parseBuildInfo } from "./cmake.js";
import { GitHubLookupSchema, lookupIssues, type GitHubLookupArgs } from "./github.js";
import { catalogPlugins } from "./plugins.js";
import { PROMPTS } from "./prompts.js";
import { listRepoDir, findRepoRoot, readRepoFile, RepoPathError } from "./repo.js";
import { RESOURCES } from "./resources.js";
import { searchRepo, type SearchRunner } from "./search.js";
import { lookupSymbol } from "./symbols.js";

export interface ServerDeps {
  repoRoot?: string;
  fetchFn?: typeof fetch;
  rgRunner?: SearchRunner;
}

const READ_ONLY_ANNOTATIONS = {
  readOnlyHint: true,
  idempotentHint: true,
  destructiveHint: false,
  openWorldHint: false,
};

const SearchSchema = z
  .object({
    pattern: z.string().min(1).describe("Regular expression to search for (ripgrep syntax)."),
    path: z.string().optional().describe("Optional in-repo directory to restrict the search to."),
    maxResults: z.number().int().min(1).max(500).optional().describe("Maximum results (default 100)."),
    caseSensitive: z.boolean().optional().describe("Force case-sensitive matching (default smart-case)."),
  })
  .strict();

const ReadFileSchema = z
  .object({
    path: z.string().min(1).describe("In-repo file path, e.g. src/core/Track.cpp."),
    startLine: z.number().int().positive().optional().describe("First line (1-based, inclusive)."),
    endLine: z.number().int().positive().optional().describe("Last line (1-based, inclusive)."),
  })
  .strict();

const ListDirSchema = z
  .object({
    path: z.string().optional().describe("In-repo directory (default: repository root)."),
  })
  .strict();

const SymbolSchema = z
  .object({
    symbol: z.string().min(1).describe("C++ symbol to look up, e.g. Instrument or Track::setName."),
    kind: z
      .enum(["definitions", "references", "all"])
      .optional()
      .describe("What to return (default all)."),
  })
  .strict();

const BuildInfoSchema = z.object({}).strict();

const PluginSchema = z
  .object({
    name: z.string().optional().describe("Restrict to one plugin directory name."),
    kind: z
      .enum(["instrument", "effect", "tool", "unknown"])
      .optional()
      .describe("Restrict by classified kind."),
  })
  .strict();

function errorResult(message: string): CallToolResult {
  return { content: [{ type: "text", text: `error: ${message}` }], isError: true };
}

function textResult(
  text: string,
  structuredContent?: Record<string, unknown>,
): CallToolResult {
  return {
    content: [{ type: "text", text }],
    ...(structuredContent ? { structuredContent } : {}),
  };
}

function guard<A>(
  fn: (args: A) => Promise<CallToolResult>,
): (args: A) => Promise<CallToolResult> {
  return async (args: A): Promise<CallToolResult> => {
    try {
      return await fn(args);
    } catch (err) {
      if (err instanceof Error) {
        return errorResult(err.message);
      }
      return errorResult(String(err));
    }
  };
}

/**
 * Build the fully wired MCP server. `deps` lets tests point at a fixture
 * checkout and inject fetch/runner implementations.
 */
export function createServer(deps: ServerDeps = {}): McpServer {
  const root =
    deps.repoRoot ??
    findRepoRoot(process.cwd()) ??
    findRepoRoot(path.dirname(fileURLToPath(import.meta.url)));
  if (!root) {
    throw new Error(
      "cannot locate the LMMS repository root: run the server from inside the checkout or set LMMS_REPO",
    );
  }
  const runner = deps.rgRunner;
  const server = new McpServer(
    { name: "lmms-mcp", version: "0.1.0" },
    { capabilities: { tools: {}, resources: {}, prompts: {} } },
  );

  server.registerTool(
    "lmms_search",
    {
      title: "Search LMMS source",
      description:
        "Regex search across the LMMS checkout (ripgrep syntax), scoped to the repository, excluding build artifacts and vendored dependencies. Returns file:line matches with snippets.",
      inputSchema: SearchSchema,
      annotations: READ_ONLY_ANNOTATIONS,
    },
    guard(async (args) => {
      const { matches, truncated } = await searchRepo(root, {
        pattern: args.pattern,
        subPath: args.path,
        maxResults: args.maxResults,
        caseSensitive: args.caseSensitive,
      });
      const lines = matches.map((m) => `${m.file}:${m.line}: ${m.text}`);
      const head = `Found ${matches.length} match${matches.length === 1 ? "" : "es"}${truncated ? " (truncated)" : ""} for /${args.pattern}/`;
      return textResult([head, "", ...lines].join("\n"), {
        matches,
        truncated,
        pattern: args.pattern,
      });
    }),
  );

  server.registerTool(
    "lmms_read_file",
    {
      title: "Read a file from the checkout",
      description:
        "Read a file inside the LMMS repository with an optional 1-based line range. Whole-file reads are capped at 4000 lines; binary files are refused.",
      inputSchema: ReadFileSchema,
      annotations: READ_ONLY_ANNOTATIONS,
    },
    guard(async (args) => {
      const f = await readRepoFile(root, args.path, args.startLine, args.endLine);
      const note = f.truncated
        ? `\n\n[truncated: file has ${f.totalLines} lines; showing first 4000]`
        : "";
      return textResult(f.content + note, {
        file: f.file,
        totalLines: f.totalLines,
        truncated: f.truncated,
      });
    }),
  );

  server.registerTool(
    "lmms_list_directory",
    {
      title: "List a repository directory",
      description:
        "List entries in a directory of the LMMS checkout (directories first, .git hidden).",
      inputSchema: ListDirSchema,
      annotations: READ_ONLY_ANNOTATIONS,
    },
    guard(async (args) => {
      const entries = await listRepoDir(root, args.path ?? ".");
      return textResult(
        entries.map((e) => `${e.type === "dir" ? `${e.name}/` : e.name}\t${e.size}`).join("\n"),
        { path: args.path ?? ".", entries },
      );
    }),
  );

  server.registerTool(
    "lmms_symbol_lookup",
    {
      title: "Look up a C++ symbol",
      description:
        "Find a class/struct/enum/namespace or member definition and its references across the LMMS source tree.",
      inputSchema: SymbolSchema,
      annotations: READ_ONLY_ANNOTATIONS,
    },
    guard(async (args) => {
      const result = await lookupSymbol(root, args.symbol, args.kind ?? "all", runner);
      const defLines = result.definitions.map((m) => `def ${m.file}:${m.line}: ${m.text}`);
      const refLines = result.references.map((m) => `ref ${m.file}:${m.line}: ${m.text}`);
      return textResult(
        [
          `${result.symbol}: ${result.definitions.length} definition(s), ${result.references.length} reference(s)`,
          "",
          ...defLines,
          ...refLines,
        ].join("\n"),
        result,
      );
    }),
  );

  server.registerTool(
    "lmms_build_info",
    {
      title: "LMMS build system facts",
      description:
        "Report the checkout's CMake minimum version, project name, and every CMake option switch found in the build definition files.",
      inputSchema: BuildInfoSchema,
      annotations: READ_ONLY_ANNOTATIONS,
    },
    guard(async () => {
      const info = parseBuildInfo(root);
      const lines = [
        `CMake minimum: ${info.cmakeMinimum ?? "unknown"}`,
        `Project: ${info.project ?? "unknown"}`,
        `Options (${info.options.length}):`,
        ...info.options.map((o) => `- ${o.name} (${o.default}): ${o.description}`),
      ];
      return textResult(lines.join("\n"), info);
    }),
  );

  server.registerTool(
    "lmms_plugin_catalog",
    {
      title: "LMMS plugin catalog",
      description:
        "List plugins in the checkout with their classified kind (instrument/effect/tool/unknown), CMake presence, source files, and class declarations. Pass name for one plugin's detail.",
      inputSchema: PluginSchema,
      annotations: READ_ONLY_ANNOTATIONS,
    },
    guard(async (args) => {
      const { plugins } = await catalogPlugins(root, { name: args.name, kind: args.kind }, runner);
      const lines = plugins.map(
        (p) =>
          `- ${p.name} [${p.kind}] ${p.hasCMakeLists ? "" : "(no CMakeLists.txt) "}${p.sourceFiles.length} source files`,
      );
      return textResult([`Plugins (${plugins.length}):`, ...lines].join("\n"), { plugins });
    }),
  );

  server.registerTool(
    "lmms_issue_lookup",
    {
      title: "LMMS GitHub issues and PRs",
      description:
        "Look up issues and pull requests on GitHub (lmms/lmms by default; override with LMMS_GITHUB_REPO). Read-only, no auth required for public repos.",
      inputSchema: GitHubLookupSchema,
      annotations: READ_ONLY_ANNOTATIONS,
    },
    guard(async (args) => {
      const { rows, repo } = await lookupIssues(args as GitHubLookupArgs, {
        fetchFn: deps.fetchFn,
      });
      const lines = rows.map(
        (r) => `#${r.number} [${r.state}] ${r.kind === "pull_request" ? "PR" : "issue"} ${r.title} (@${r.author}) ${r.url}`,
      );
      return textResult([`${repo}: ${rows.length} result(s)`, "", ...lines].join("\n"), {
        repo,
        rows,
      });
    }),
  );

  for (const def of RESOURCES) {
    server.registerResource(
      def.name,
      def.uri,
      { description: def.description },
      async (uri) => {
        const text = await def.load(root, runner);
        return { contents: [{ uri: uri.href, text }] };
      },
    );
  }

  for (const prompt of PROMPTS) {
    server.registerPrompt(
      prompt.name,
      { description: prompt.description, argsSchema: prompt.schema },
      (args) => ({
        messages: [
          {
            role: "user" as const,
            content: {
              type: "text" as const,
              text: prompt.render((args ?? {}) as Record<string, unknown>),
            },
          },
        ],
      }),
    );
  }

  return server;
}
