/**
 * Reusable MCP prompts for common LMMS development workflows. Each prompt
 * carries a zod argument schema (validated by the protocol) and renders
 * instructions the client agent can execute; all are read-only guidance,
 * never state-changing operations.
 */
import { z } from "zod";

export interface PromptDef {
  name: string;
  description: string;
  schema: z.ZodRawShape;
  render: (args: Record<string, unknown>) => string;
}

const newPluginShape = {
  name: z
    .string()
    .regex(/^[A-Za-z][A-Za-z0-9]*$/, "plugin name must be an identifier")
    .describe("Plugin name, e.g. MySynth"),
  kind: z
    .enum(["instrument", "effect", "tool"])
    .default("instrument")
    .describe("instrument, effect, or tool"),
} satisfies z.ZodRawShape;

const buildSetupShape = {
  target: z.string().optional().describe("Optional build target to compile instead of the full build"),
} satisfies z.ZodRawShape;

const bugInvestigationShape = {
  symptom: z.string().min(1).describe("Reported bug symptom or behavior"),
} satisfies z.ZodRawShape;

const codeReviewShape = {
  scope: z.string().optional().describe("Optional scope of the review (files, branch, or PR number)"),
} satisfies z.ZodRawShape;

export const PROMPTS: PromptDef[] = [
  {
    name: "lmms-new-plugin",
    description:
      "Scaffold a new LMMS plugin: create the plugin directory, class skeleton, CMake wiring, and build verification steps.",
    schema: newPluginShape,
    render: (args) => {
      const parsed = z.object(newPluginShape).parse(args);
      const base = parsed.kind === "instrument" ? "Instrument" : parsed.kind === "effect" ? "Effect" : "Plugin";
      return [
        `Scaffold a new LMMS ${parsed.kind} plugin named ${parsed.name}.`,
        "",
        "1. Read doc/wiki/Plugin-development.md if present, and read the CMakeLists.txt and source layout of an existing sibling plugin (e.g. plugins/TripleOscillator for an instrument, plugins/ReverbSC for an effect) as the pattern to follow.",
        `2. Create plugins/${parsed.name}/ with ${parsed.name}.h and ${parsed.name}.cpp declaring \`class ${parsed.name} : public ${base}\`, following the sibling's constructor and model setup.`,
        "3. Add plugins/${parsed.name}/CMakeLists.txt matching the sibling's plugin build pattern.",
        "4. Register the plugin in plugins/CMakeLists.txt or cmake/modules/PluginList.cmake where sibling plugins are listed.",
        "5. Verify: configure a build with the plugin enabled, build the ${parsed.name} target, and check the plugin appears in the LMMS plugin browser when run.",
        "",
        "Do not skip the registration step; an unregistered plugin builds but never appears.",
      ].join("\n");
    },
  },
  {
    name: "lmms-build-setup",
    description:
      "Configure and build LMMS for development, choosing sensible options for the platform.",
    schema: buildSetupShape,
    render: (args) => {
      const parsed = z.object(buildSetupShape).parse(args);
      const targetLine = parsed.target
        ? `Build the target: \`cmake --build build --target ${parsed.target}\``
        : "Build: `cmake --build build -j$(nproc)` (or the equivalent for the platform)";
      return [
        "Set up a development build of LMMS:",
        "",
        "1. Check the lmms://build resource for the CMake minimum version and option switches available in this checkout.",
        "2. Configure an out-of-tree debug build with the common development options enabled (sanitizers, debug symbols). Recommended:",
        "   cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DWANT_DEBUG_ASAN=ON",
        `3. ${targetLine}.`,
        "4. Note the Qt requirement (Qt6 support is experimental; use the Qt5 default unless the checkout defaults otherwise).",
        "",
        "Report the exact configure and build commands used and any missing dependencies.",
      ].join("\n");
    },
  },
  {
    name: "lmms-bug-investigation",
    description:
      "Investigate a reported LMMS bug: locate the relevant code, trace the failure, and produce a reproduction plan.",
    schema: bugInvestigationShape,
    render: (args) => {
      const parsed = z.object(bugInvestigationShape).parse(args);
      return [
        `Investigate this LMMS bug: ${parsed.symptom}`,
        "",
        "1. Search the codebase for the feature area named by the symptom (use lmms_search and lmms_symbol_lookup on the relevant classes).",
        "2. Read the surrounding implementation in src/core, src/gui, or plugins/ and identify the code path that produces the observed behavior.",
        "3. Check open GitHub issues for duplicates (lmms_issue_lookup).",
        "4. Produce a reproduction plan: exact steps, expected vs actual behavior, and the files/lines most likely responsible.",
        "5. If a fix is authorized, implement it and add a regression test following the conventions in tests/ (scripted tests for behavior, unit tests for pure logic).",
      ].join("\n");
    },
  },
  {
    name: "lmms-code-review",
    description:
      "Review an LMMS change against the project's conventions and common C++/Qt pitfalls.",
    schema: codeReviewShape,
    render: (args) => {
      const parsed = z.object(codeReviewShape).parse(args);
      const scopeLine = parsed.scope ? `Review scope: ${parsed.scope}\n` : "";
      return [
        "Review the LMMS change for correctness and project fit.",
        "",
        scopeLine,
        "1. Check the lmms://coding-conventions resource and align with the tooling files (.clang-format, .clang-tidy) present in the checkout.",
        "2. Look for: raw pointers that should be RAII or Qt parent-owned, thread-safety around audio processing (never allocate or lock in the audio thread), float vs double correctness in DSP code, and signal/slot connection lifetimes.",
        "3. Verify plugin changes register in PluginList and follow the sibling-plugin pattern.",
        "4. Verify tests exist for behavioral changes (tests/ conventions) and that the change builds cleanly.",
        "5. Summarize findings by severity with file:line references.",
      ].join("\n");
    },
  },
];
