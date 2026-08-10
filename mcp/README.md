# lmms-mcp

A Model Context Protocol server for the [LMMS](https://github.com/lmms/lmms)
codebase. It gives coding agents safe, read-only access to the checkout it
lives in: source search, symbol lookup, build-system facts, the plugin
catalog, GitHub issues/PRs, and curated resources and prompts for common
LMMS workflows.

Everything is read-only: no tool mutates the repository, the filesystem, or
GitHub. Paths are confined to the repository root (traversal and symlink
escapes are rejected).

## Features

### Tools

| Tool | Purpose |
| --- | --- |
| `lmms_search` | Regex search (ripgrep syntax) across the checkout, excluding build artifacts and `node_modules`; falls back to a bounded walker when ripgrep is absent |
| `lmms_read_file` | Read any repo file with an optional 1-based line range; caps whole-file reads at 4000 lines, refuses binary files |
| `lmms_list_directory` | List a repo directory (directories first, `.git` hidden) |
| `lmms_symbol_lookup` | Find C++ class/struct/enum/namespace declarations, `Name::` member definitions, and references |
| `lmms_build_info` | CMake minimum version, project name, and every `option(...)` switch parsed from the build files |
| `lmms_plugin_catalog` | Every plugin in `plugins/` classified as instrument/effect/tool/unknown, with sources and class declarations |
| `lmms_issue_lookup` | GitHub issues/PRs for `lmms/lmms` (or `LMMS_GITHUB_REPO`); no auth needed for public repos |

### Resources

`lmms://overview`, `lmms://build`, `lmms://architecture`,
`lmms://coding-conventions`, `lmms://plugins`, `lmms://documentation`,
`lmms://agents` — all generated from the live checkout at read time.

### Prompts

`lmms-new-plugin`, `lmms-build-setup`, `lmms-bug-investigation`,
`lmms-code-review`.

## Requirements

- Node.js >= 20.11
- `rg` (ripgrep) is optional but recommended for fast search; the server
  falls back to a built-in walker when it is missing.

## Install and run

```sh
cd mcp
npm ci
npm run build
```

Run the server over stdio:

```sh
node mcp/dist/index.js
```

The server locates the repository root by walking up from its own location or
the current directory. To point it at a specific checkout:

```sh
LMMS_REPO=/path/to/lmms node mcp/dist/index.js
```

## MCP client configuration

Generic clients (Claude Desktop, etc.):

```json
{
  "mcpServers": {
    "lmms": {
      "command": "node",
      "args": ["/path/to/lmms/mcp/dist/index.js"],
      "env": { "LMMS_REPO": "/path/to/lmms" }
    }
  }
}
```

Claude Code:

```sh
claude mcp add lmms -- node /path/to/lmms/mcp/dist/index.js
```

## Environment variables

| Variable | Default | Purpose |
| --- | --- | --- |
| `LMMS_REPO` | auto-detected | Path to the LMMS checkout the server operates on |
| `LMMS_GITHUB_REPO` | `lmms/lmms` | GitHub repo used by `lmms_issue_lookup` |
| `LMMS_GITHUB_TOKEN` | unset | Optional token to raise the GitHub rate limit or read private repos |

## Development

```sh
npm test        # vitest suite (fixture repo + protocol-level tests)
npm run build   # type-check and emit dist/
```

The test suite exercises the server over an in-memory MCP transport against a
small fixture checkout, plus unit tests for path safety, search fallback,
CMake parsing, plugin classification, and GitHub mapping.
