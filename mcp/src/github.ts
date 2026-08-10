/**
 * GitHub issues and pull requests for the LMMS project, via the public REST
 * API. Read-only. The target repository defaults to lmms/lmms and can be
 * overridden with LMMS_GITHUB_REPO; an optional LMMS_GITHUB_TOKEN raises the
 * rate limit for private forks.
 */
import { z } from "zod";

export const GitHubLookupSchema = z
  .object({
    number: z
      .number()
      .int()
      .positive()
      .describe("Fetch a single issue or PR by its number."),
    query: z
      .string()
      .describe("Free-text GitHub search query (title/body/labels/author)."),
    state: z
      .enum(["open", "closed", "all"])
      .optional()
      .describe("Filter by open/closed state; defaults to open."),
    kind: z
      .enum(["issue", "pull_request", "any"])
      .optional()
      .describe("What to list; defaults to issues only."),
    limit: z
      .number()
      .int()
      .min(1)
      .max(50)
      .optional()
      .describe("Maximum results (default 10)."),
  })
  .partial()
  .refine((v) => v.number !== undefined || v.query !== undefined || v.kind !== undefined, {
    message: "provide a number, a query, or a kind to list",
  });

export type GitHubLookupArgs = z.infer<typeof GitHubLookupSchema>;

export interface IssueRow {
  number: number;
  title: string;
  state: "open" | "closed";
  kind: "issue" | "pull_request";
  labels: string[];
  author: string;
  createdAt: string;
  url: string;
}

export interface GitHubDeps {
  fetchFn?: typeof fetch;
  repo?: string;
  token?: string;
}

const API = "https://api.github.com";

function rowFromApiItem(
  item: Record<string, unknown>,
  repoName: string,
): IssueRow | null {
  const number = typeof item.number === "number" ? item.number : Number(item.number);
  const title = typeof item.title === "string" ? item.title : "";
  if (!Number.isFinite(number) || !title) {
    return null;
  }
  const labels = Array.isArray(item.labels)
    ? item.labels
        .map((l) => (l && typeof l === "object" ? (l as Record<string, unknown>).name : undefined))
        .filter((n): n is string => typeof n === "string")
    : [];
  const user =
    item.user && typeof item.user === "object"
      ? ((item.user as Record<string, unknown>).login as string | undefined)
      : undefined;
  const isPr = Boolean(item.pull_request) || item.html_url?.toString().includes("/pull/");
  return {
    number,
    title,
    state: item.state === "closed" ? "closed" : "open",
    kind: isPr ? "pull_request" : "issue",
    labels,
    author: user ?? "unknown",
    createdAt: typeof item.created_at === "string" ? item.created_at : "",
    url: typeof item.html_url === "string" ? item.html_url : `https://github.com/${repoName}/issues/${number}`,
  };
}

function formatIssueError(status: number, body: string, repo: string): string {
  if (status === 403 || status === 429) {
    return `GitHub rate limit reached for ${repo}. Set LMMS_GITHUB_TOKEN to raise the limit, or retry later.`;
  }
  if (status === 404) {
    return `GitHub returned 404 for ${repo} - the repository does not exist or is private (set LMMS_GITHUB_TOKEN to read private repos).`;
  }
  return `GitHub API error ${status} for ${repo}: ${body.slice(0, 300)}`;
}

/**
 * Look up issues/PRs. `deps.fetchFn` is injectable for tests; default is the
 * global fetch. Always read-only.
 */
export async function lookupIssues(
  args: GitHubLookupArgs,
  deps: GitHubDeps = {},
): Promise<{ rows: IssueRow[]; repo: string }> {
  const repo = deps.repo ?? process.env.LMMS_GITHUB_REPO ?? "lmms/lmms";
  const fetchFn = deps.fetchFn ?? globalThis.fetch;
  const headers: Record<string, string> = {
    Accept: "application/vnd.github+json",
    "User-Agent": "lmms-mcp",
    "X-GitHub-Api-Version": "2022-11-28",
  };
  const token = deps.token ?? process.env.LMMS_GITHUB_TOKEN;
  if (token) {
    headers.Authorization = `Bearer ${token}`;
  }
  const limit = Math.min(50, args.limit ?? 10);

  const fetchJson = async (url: string): Promise<Record<string, unknown>> => {
    let res: Response;
    try {
      res = await fetchFn(url, { headers });
    } catch (err) {
      throw new Error(
        `GitHub request failed (network error): ${(err as Error).message}. Check connectivity.`,
      );
    }
    const body = await res.text();
    if (!res.ok) {
      throw new Error(formatIssueError(res.status, body, repo));
    }
    try {
      return JSON.parse(body) as Record<string, unknown>;
    } catch {
      throw new Error(`GitHub returned invalid JSON (HTTP ${res.status}).`);
    }
  };

  if (args.number !== undefined) {
    const item = await fetchJson(`${API}/repos/${repo}/issues/${args.number}`);
    const row = rowFromApiItem(item, repo);
    if (!row) {
      throw new Error(`GitHub returned an unexpected payload for issue #${args.number}.`);
    }
    return { rows: [row], repo };
  }

  if (args.query !== undefined) {
    const stateQ = args.state && args.state !== "all" ? ` state:${args.state}` : "";
    const kindQ =
      args.kind === "pull_request" ? " is:pr" : args.kind === "issue" ? " is:issue" : "";
    const q = encodeURIComponent(`repo:${repo} ${args.query}${stateQ}${kindQ}`.trim());
    const data = await fetchJson(`${API}/search/issues?q=${q}&per_page=${limit}`);
    const items = Array.isArray(data.items) ? (data.items as Record<string, unknown>[]) : [];
    return { rows: items.map((i) => rowFromApiItem(i, repo)).filter((r): r is IssueRow => r !== null), repo };
  }

  const stateQ = args.state && args.state !== "all" ? `state=${args.state}&` : "";
  if (args.kind === "pull_request") {
    const data = await fetchJson(`${API}/repos/${repo}/pulls?${stateQ}per_page=${limit}`);
    const items = Array.isArray(data) ? (data as Record<string, unknown>[]) : [];
    return { rows: items.map((i) => rowFromApiItem(i, repo)).filter((r): r is IssueRow => r !== null), repo };
  }
  const data = await fetchJson(`${API}/repos/${repo}/issues?${stateQ}per_page=${limit}`);
  const items = Array.isArray(data) ? (data as Record<string, unknown>[]) : [];
  const rows = items
    .map((i) => rowFromApiItem(i, repo))
    .filter((r): r is IssueRow => r !== null && r.kind === "issue");
  return { rows, repo };
}
