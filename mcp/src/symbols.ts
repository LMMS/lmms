/**
 * C++-aware symbol lookup over the LMMS source tree. Definitions are found
 * through declaration patterns (class/struct/enum/namespace and `Name::`
 * member definitions); references are plain word matches.
 */
import { searchRepo, type SearchMatch, type SearchRunner } from "./search.js";

export type SymbolKind = "definitions" | "references" | "all";

export type SymbolLookupResult = {
  symbol: string;
  kind: SymbolKind;
  definitions: SearchMatch[];
  references: SearchMatch[];
};

/** Only safe identifier characters are accepted as symbol names. */
const SYMBOL_RE = /^[A-Za-z_][A-Za-z0-9_:~]*$/;

const DEFINITION_PATTERN = (
  escaped: string,
) => `(?m)^\\s*(class|struct|enum|namespace|enum\\s+class)\\s+${escaped}\\b`;

const MEMBER_PATTERN = (escaped: string) => `${escaped}::`;

const REFERENCE_PATTERN = (escaped: string) => `\\b${escaped}\\b`;

function escapeRegex(s: string): string {
  return s.replace(/[.*+?^${}()|[\]\\]/g, "\\$&");
}

function dedupe(matches: SearchMatch[]): SearchMatch[] {
  const seen = new Set<string>();
  const out: SearchMatch[] = [];
  for (const m of matches) {
    const key = `${m.file}:${m.line}:${m.text}`;
    if (!seen.has(key)) {
      seen.add(key);
      out.push(m);
    }
  }
  return out;
}

/**
 * Look up a symbol across the repository. `runner` is injectable for tests;
 * defaults to the same rg-with-fallback runner as searchRepo.
 */
export async function lookupSymbol(
  root: string,
  symbol: string,
  kind: SymbolKind = "all",
  runner?: SearchRunner,
): Promise<SymbolLookupResult> {
  if (!SYMBOL_RE.test(symbol)) {
    throw new Error(
      `invalid symbol name: ${symbol} (expected an identifier like Instrument or Track::setName)`,
    );
  }
  const escaped = escapeRegex(symbol);
  const definitions: SearchMatch[] = [];
  const references: SearchMatch[] = [];

  if (kind === "definitions" || kind === "all") {
    const decls = await searchRepo(
      root,
      { pattern: DEFINITION_PATTERN(escaped), maxResults: 50, caseSensitive: true },
      runner,
    );
    definitions.push(...decls.matches);
    const members = await searchRepo(
      root,
      { pattern: MEMBER_PATTERN(escaped), maxResults: 50, caseSensitive: true },
      runner,
    );
    definitions.push(...members.matches);
  }
  if (kind === "references" || kind === "all") {
    const refs = await searchRepo(
      root,
      { pattern: REFERENCE_PATTERN(escaped), maxResults: 100, caseSensitive: true },
      runner,
    );
    references.push(...refs.matches);
  }
  return {
    symbol,
    kind,
    definitions: dedupe(definitions),
    references: dedupe(references),
  };
}
