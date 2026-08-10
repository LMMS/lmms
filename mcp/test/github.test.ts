import { describe, expect, it } from "vitest";
import { lookupIssues } from "../src/github.js";

function stubFetch(
  status: number,
  payload: unknown,
): { fetchFn: typeof fetch; urls: string[] } {
  const urls: string[] = [];
  const fetchFn = (async (url: string | URL | Request) => {
    urls.push(String(url));
    return new Response(JSON.stringify(payload), {
      status,
      headers: { "Content-Type": "application/json" },
    });
  }) as typeof fetch;
  return { fetchFn, urls };
}

const issuePayload = (over: Record<string, unknown> = {}) => ({
  number: 1234,
  title: "Tempo automation is jittery",
  state: "open",
  user: { login: "someuser" },
  labels: [{ name: "bug" }],
  created_at: "2026-01-02T03:04:05Z",
  html_url: "https://github.com/lmms/lmms/issues/1234",
  ...over,
});

describe("lookupIssues", () => {
  it("fetches a single issue by number", async () => {
    const { fetchFn, urls } = stubFetch(200, issuePayload());
    const { rows, repo } = await lookupIssues({ number: 1234 }, { fetchFn });
    expect(urls[0]).toContain("/repos/lmms/lmms/issues/1234");
    expect(repo).toBe("lmms/lmms");
    expect(rows).toHaveLength(1);
    expect(rows[0]).toMatchObject({
      number: 1234,
      title: "Tempo automation is jittery",
      state: "open",
      kind: "issue",
      labels: ["bug"],
      author: "someuser",
    });
    expect(rows[0]!.url).toContain("github.com/lmms/lmms/issues/1234");
  });

  it("classifies an item with pull_request as a PR", async () => {
    const { fetchFn } = stubFetch(200, issuePayload({ pull_request: { url: "x" } }));
    const { rows } = await lookupIssues({ number: 9 }, { fetchFn });
    expect(rows[0]!.kind).toBe("pull_request");
  });

  it("runs a search query against the search endpoint", async () => {
    const { fetchFn, urls } = stubFetch(200, {
      items: [
        issuePayload({ number: 1, title: "hit one" }),
        issuePayload({ number: 2, title: "hit two", pull_request: { url: "x" } }),
      ],
    });
    const { rows } = await lookupIssues(
      { query: "tempo automation", state: "open", kind: "issue" },
      { fetchFn },
    );
    expect(urls[0]).toContain("/search/issues?q=");
    expect(decodeURIComponent(urls[0]!)).toContain("repo:lmms/lmms tempo automation state:open is:issue");
    expect(rows).toHaveLength(2);
    expect(rows.map((r) => r.kind)).toEqual(["issue", "pull_request"]);
  });

  it("lists issues and filters out pull requests", async () => {
    const { fetchFn, urls } = stubFetch(200, [
      issuePayload({ number: 10 }),
      issuePayload({ number: 11, pull_request: { url: "x" } }),
    ]);
    const { rows } = await lookupIssues({ state: "open" }, { fetchFn });
    expect(urls[0]).toContain("/repos/lmms/lmms/issues?state=open&per_page=10");
    expect(rows).toHaveLength(1);
    expect(rows[0]!.number).toBe(10);
  });

  it("lists pull requests from the pulls endpoint", async () => {
    const { fetchFn, urls } = stubFetch(200, [issuePayload({ number: 20 })]);
    const { rows } = await lookupIssues({ kind: "pull_request" }, { fetchFn });
    expect(urls[0]).toContain("/repos/lmms/lmms/pulls?");
    expect(rows).toHaveLength(1);
    expect(rows[0]!.kind).toBe("issue"); // pulls payload lacks pull_request marker
  });

  it("honors the repo override", async () => {
    const { fetchFn, urls } = stubFetch(200, []);
    await lookupIssues({ kind: "pull_request" }, { fetchFn, repo: "saksham-45/lmms" });
    expect(urls[0]).toContain("/repos/saksham-45/lmms/pulls?");
  });

  it("explains rate limits", async () => {
    const { fetchFn } = stubFetch(403, { message: "API rate limit exceeded" });
    await expect(lookupIssues({ number: 1 }, { fetchFn })).rejects.toThrow(
      /rate limit/,
    );
  });

  it("explains 404s with the repo name", async () => {
    const { fetchFn } = stubFetch(404, { message: "Not Found" });
    await expect(
      lookupIssues({ number: 1 }, { fetchFn, repo: "somebody/private-repo" }),
    ).rejects.toThrow(/somebody\/private-repo/);
  });

  it("surfaces network failures", async () => {
    const fetchFn = (async () => {
      throw new TypeError("fetch failed");
    }) as typeof fetch;
    await expect(lookupIssues({ number: 1 }, { fetchFn })).rejects.toThrow(
      /network error/,
    );
  });
});
