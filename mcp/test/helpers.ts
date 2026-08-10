import { fileURLToPath } from "node:url";

/** Absolute path to the fixture mini-repo used across the suite. */
export const FIXTURE = fileURLToPath(new URL("./fixtures/repo", import.meta.url));
