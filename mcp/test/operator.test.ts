import { Client } from "@modelcontextprotocol/sdk/client/index.js";
import { InMemoryTransport } from "@modelcontextprotocol/sdk/inMemory.js";
import net from "node:net";
import { afterEach, describe, expect, it } from "vitest";
import { AgentdClient, LmmsClient, LmmsClientError } from "../src/lmms-client.js";
import { createOperatorServer } from "../src/operator.js";

interface FakeHandler {
  (payload: Record<string, unknown>): Promise<Record<string, unknown>> | Record<string, unknown>;
}

/**
 * Minimal stand-in for the AgentControl TCP server embedded in LMMS
 * (plugins/AgentControl/AgentControl.cpp): newline-delimited JSON, one
 * response per request.
 */
class FakeAgentControl {
  server = net.createServer();
  port = 0;
  requests: Record<string, unknown>[] = [];
  private state = { tempo: 120, track_count: 2, project_file: "" };

  constructor(private handler: FakeHandler) {
    this.server.on("connection", (socket) => {
      let buffer = "";
      socket.on("data", (chunk) => {
        buffer += chunk.toString("utf8");
        let idx: number;
        while ((idx = buffer.indexOf("\n")) >= 0) {
          const line = buffer.slice(0, idx);
          buffer = buffer.slice(idx + 1);
          const payload = JSON.parse(line) as Record<string, unknown>;
          this.requests.push(payload);
          void Promise.resolve(this.handler(payload)).then((response) => {
            socket.write(`${JSON.stringify(response)}\n`);
          });
        }
      });
    });
  }

  listen(): Promise<void> {
    return new Promise((resolve) => {
      this.server.listen(0, "127.0.0.1", () => {
        const address = this.server.address();
        if (address && typeof address === "object") {
          this.port = address.port;
        }
        resolve();
      });
    });
  }

  close(): Promise<void> {
    return new Promise((resolve) => this.server.close(() => resolve()));
  }
}

class FakeAgentd {
  server = net.createServer();
  port = 0;
  requests: Record<string, unknown>[] = [];

  constructor(private handler: FakeHandler) {
    this.server.on("connection", (socket) => {
      let buffer = "";
      socket.on("data", (chunk) => {
        buffer += chunk.toString("utf8");
        let idx: number;
        while ((idx = buffer.indexOf("\n")) >= 0) {
          const line = buffer.slice(0, idx);
          buffer = buffer.slice(idx + 1);
          const payload = JSON.parse(line) as Record<string, unknown>;
          this.requests.push(payload);
          void Promise.resolve(this.handler(payload)).then((response) => {
            socket.write(`${JSON.stringify(response)}\n`);
          });
        }
      });
    });
  }

  listen(): Promise<void> {
    return new Promise((resolve) => {
      this.server.listen(0, "127.0.0.1", () => {
        const address = this.server.address();
        if (address && typeof address === "object") {
          this.port = address.port;
        }
        resolve();
      });
    });
  }

  close(): Promise<void> {
    return new Promise((resolve) => this.server.close(() => resolve()));
  }
}

function okEnvelope(
  result: Record<string, unknown>,
  stateDelta?: Record<string, unknown>,
): Record<string, unknown> {
  return {
    ok: true,
    result,
    state_delta: stateDelta ?? {},
    warnings: [],
    error_code: null,
    error_message: null,
  };
}

const activeServers: (FakeAgentControl | FakeAgentd)[] = [];

afterEach(async () => {
  while (activeServers.length > 0) {
    const s = activeServers.pop();
    await s!.close();
  }
});

describe("lmms-operator over the MCP protocol", () => {
  async function makeClient(fake: FakeAgentControl, agentd?: FakeAgentd) {
    const server = createOperatorServer({
      client: new LmmsClient({ host: "127.0.0.1", port: fake.port, timeoutMs: 3000 }),
      agentd: agentd
        ? new AgentdClient({ host: "127.0.0.1", port: agentd.port, timeoutMs: 3000 })
        : undefined,
    });
    const [clientTransport, serverTransport] = InMemoryTransport.createLinkedPair();
    const client = new Client({ name: "lmms-operator-test", version: "0.0.0" });
    await server.connect(serverTransport);
    await client.connect(clientTransport);
    return { client, server };
  }

  it("advertises the operator tool surface", async () => {
    const fake = new FakeAgentControl(() => okEnvelope({}));
    await fake.listen();
    activeServers.push(fake);
    const { client } = await makeClient(fake);
    const { tools } = await client.listTools();
    const names = tools.map((t) => t.name).sort();
    expect(names).toEqual(
      [
        "lmms_add_arpeggio",
        "lmms_add_channel_effect",
        "lmms_add_chord",
        "lmms_add_effect",
        "lmms_add_notes",
        "lmms_add_rhythm",
        "lmms_add_steps",
        "lmms_automate",
        "lmms_cancel_render",
        "lmms_clear_clip",
        "lmms_clear_loop",
        "lmms_clone_clip",
        "lmms_clone_pattern",
        "lmms_clone_track",
        "lmms_command",
        "lmms_confirm",
        "lmms_connect_controller",
        "lmms_create_automation",
        "lmms_create_channel",
        "lmms_create_clip",
        "lmms_create_controller",
        "lmms_create_pattern",
        "lmms_create_send",
        "lmms_create_track",
        "lmms_delete_channel",
        "lmms_delete_clip",
        "lmms_delete_send",
        "lmms_delete_track",
        "lmms_describe_controllers",
        "lmms_describe_instrument",
        "lmms_describe_song",
        "lmms_diff",
        "lmms_disconnect_controller",
        "lmms_edit_notes",
        "lmms_export_midi",
        "lmms_find_track",
        "lmms_get_effect_params",
        "lmms_get_peak_levels",
        "lmms_get_project_notes",
        "lmms_get_render_progress",
        "lmms_global_automate",
        "lmms_goal",
        "lmms_humanize_clip",
        "lmms_import_audio",
        "lmms_import_hydrogen",
        "lmms_import_midi",
        "lmms_insert_bar",
        "lmms_list_automation",
        "lmms_list_effects",
        "lmms_list_instruments",
        "lmms_list_mixer_channels",
        "lmms_list_patterns",
        "lmms_list_tools",
        "lmms_list_tracks",
        "lmms_load_instrument",
        "lmms_load_instrument_preset",
        "lmms_load_sample",
        "lmms_move_channel",
        "lmms_move_clip",
        "lmms_move_effect",
        "lmms_move_track",
        "lmms_mute_track",
        "lmms_new_project",
        "lmms_open_project",
        "lmms_open_tool",
        "lmms_pause",
        "lmms_play",
        "lmms_play_clip",
        "lmms_play_pattern",
        "lmms_quantize_clip",
        "lmms_read_automation",
        "lmms_record_arm",
        "lmms_record_disarm",
        "lmms_record_start",
        "lmms_record_stop",
        "lmms_remove_automation_node",
        "lmms_remove_bar",
        "lmms_remove_channel_effect",
        "lmms_remove_effect",
        "lmms_remove_notes",
        "lmms_rename_channel",
        "lmms_rename_track",
        "lmms_render_preview",
        "lmms_render_song",
        "lmms_render_tracks",
        "lmms_resize_clip",
        "lmms_reverse_clip",
        "lmms_rollback",
        "lmms_route_track_to_channel",
        "lmms_save_project",
        "lmms_save_project_as",
        "lmms_search_audio",
        "lmms_select_pattern",
        "lmms_select_track",
        "lmms_set_arp",
        "lmms_set_automation_node",
        "lmms_set_automation_progression",
        "lmms_set_automation_tension",
        "lmms_set_channel_mute",
        "lmms_set_channel_solo",
        "lmms_set_channel_volume",
        "lmms_set_clip_color",
        "lmms_set_clip_mute",
        "lmms_set_clip_name",
        "lmms_set_clip_velocity_scale",
        "lmms_set_effect_enabled",
        "lmms_set_effect_param",
        "lmms_set_effect_wetdry",
        "lmms_set_envelope",
        "lmms_set_filter",
        "lmms_set_lfo",
        "lmms_set_lfo_controller",
        "lmms_set_loop",
        "lmms_set_master_pitch",
        "lmms_set_master_volume",
        "lmms_set_metronome",
        "lmms_set_microtuner",
        "lmms_set_note_stacking",
        "lmms_set_play_pos",
        "lmms_set_project_notes",
        "lmms_set_sample_amp",
        "lmms_set_sample_loop",
        "lmms_set_sample_pitch",
        "lmms_set_sample_range",
        "lmms_set_send_amount",
        "lmms_set_sf2_patch",
        "lmms_set_step_velocity",
        "lmms_set_steps",
        "lmms_set_steps_per_bar",
        "lmms_set_stop_behaviour",
        "lmms_set_tempo",
        "lmms_set_time_signature",
        "lmms_set_track_base_note",
        "lmms_set_track_key_range",
        "lmms_set_track_pan",
        "lmms_set_track_pitch",
        "lmms_set_track_volume",
        "lmms_set_vst_param",
        "lmms_set_vst_program",
        "lmms_snapshot",
        "lmms_solo_track",
        "lmms_split_clip",
        "lmms_split_clip_notes",
        "lmms_state",
        "lmms_stop",
        "lmms_track_details",
        "lmms_undo",
      ].sort(),
    );
    for (const t of tools) {
      expect(t.description?.length).toBeGreaterThan(10);
    }
    await client.close();
  });

  it("returns project state through lmms_state", async () => {
    const fake = new FakeAgentControl(() =>
      okEnvelope({ project_file: "", tempo: 140, track_count: 3 }),
    );
    await fake.listen();
    activeServers.push(fake);
    const { client } = await makeClient(fake);
    const res = await client.callTool({ name: "lmms_state", arguments: {} });
    expect(res.isError).toBeFalsy();
    expect(fake.requests).toEqual([{ tool: "getprojectstate", args: {} }]);
    const content = res.structuredContent as Record<string, unknown>;
    expect(content.ok).toBe(true);
    expect((content.result as Record<string, unknown>).tempo).toBe(140);
    await client.close();
  });

  it("passes typed args through for settempo", async () => {
    const fake = new FakeAgentControl(() =>
      okEnvelope({ tempo: 128 }, { tempo_before: 120, tempo_after: 128 }),
    );
    await fake.listen();
    activeServers.push(fake);
    const { client } = await makeClient(fake);
    const res = await client.callTool({
      name: "lmms_set_tempo",
      arguments: { tempo: 128 },
    });
    expect(res.isError).toBeFalsy();
    expect(fake.requests).toEqual([{ tool: "settempo", args: { tempo: 128 } }]);
    const content = res.structuredContent as Record<string, unknown>;
    expect((content.state_delta as Record<string, unknown>).tempo_after).toBe(128);
    await client.close();
  });

  it("forwards note arrays to addnotes", async () => {
    const fake = new FakeAgentControl(() =>
      okEnvelope({ track: "Bass", clip_index: 0, notes_added: 2 }),
    );
    await fake.listen();
    activeServers.push(fake);
    const { client } = await makeClient(fake);
    const res = await client.callTool({
      name: "lmms_add_notes",
      arguments: {
        track: "Bass",
        notes: [
          { key: 36, pos: 0, length: 48, velocity: 110 },
          { key: 43, pos: 96, length: 48, velocity: 90 },
        ],
      },
    });
    expect(res.isError).toBeFalsy();
    expect(fake.requests).toEqual([
      {
        tool: "addnotes",
        args: {
          track: "Bass",
          notes: [
            { key: 36, pos: 0, length: 48, velocity: 110 },
            { key: 43, pos: 96, length: 48, velocity: 90 },
          ],
        },
      },
    ]);
    await client.close();
  });

  it("sends commands through the NL command path", async () => {
    const fake = new FakeAgentControl(() => okEnvelope({ message: "Playing" }));
    await fake.listen();
    activeServers.push(fake);
    const { client } = await makeClient(fake);
    const res = await client.callTool({ name: "lmms_play", arguments: {} });
    expect(res.isError).toBeFalsy();
    expect(fake.requests).toEqual([{ command: "play" }]);
    await client.close();
  });

  it("surfaces the confirmation gate and answers it", async () => {
    let pending = false;
    const fake = new FakeAgentControl((payload) => {
      if (payload.command === "new project") {
        pending = true;
        return okEnvelope({
          message: "Confirmation required: \"new project\". Say \"yes\" to execute or \"cancel\".",
        });
      }
      if (payload.command === "yes") {
        pending = false;
        return okEnvelope({ message: "New project created" });
      }
      return okEnvelope({ message: "ok" });
    });
    await fake.listen();
    activeServers.push(fake);
    const { client } = await makeClient(fake);

    const gate = await client.callTool({ name: "lmms_new_project", arguments: {} });
    expect(gate.isError).toBeFalsy();
    const gateContent = gate.structuredContent as Record<string, unknown>;
    expect(gateContent.needs_confirmation).toBe(true);
    expect(pending).toBe(true);

    const confirm = await client.callTool({
      name: "lmms_confirm",
      arguments: { approve: true },
    });
    expect(confirm.isError).toBeFalsy();
    expect(fake.requests.map((r) => r.command)).toEqual(["new project", "yes"]);
    expect(pending).toBe(false);
    await client.close();
  });

  it("marks plugin failures as tool errors with the envelope intact", async () => {
    const fake = new FakeAgentControl(() => ({
      ok: false,
      result: {},
      warnings: [],
      error_code: "unknown_tool",
      error_message: "Unknown tool: bogus",
    }));
    await fake.listen();
    activeServers.push(fake);
    const { client } = await makeClient(fake);
    const res = await client.callTool({ name: "lmms_undo", arguments: {} });
    expect(res.isError).toBe(true);
    const content = res.structuredContent as Record<string, unknown>;
    expect(content.error_code).toBe("unknown_tool");
    await client.close();
  });

  it("routes goals to the agent daemon with retry metadata", async () => {
    const agentd = new FakeAgentd((payload) => ({
      ok: true,
      request_id: "req_1",
      op: "run_goal",
      timeout_class: "standard",
      attempts: 1,
      replayed: false,
      duration_ms: 42,
      result: { mode: "plan", status: "success", steps: [] },
      meta: { requests_total: 3 },
    }));
    await agentd.listen();
    activeServers.push(agentd);

    const fake = new FakeAgentControl(() => okEnvelope({}));
    await fake.listen();
    activeServers.push(fake);

    const { client } = await makeClient(fake, agentd);
    const res = await client.callTool({
      name: "lmms_goal",
      arguments: {
        goal: "make a 4-bar house loop with an 808 bassline",
        timeout_class: "background",
        idempotency_key: "k1",
      },
    });
    expect(res.isError).toBeFalsy();
    expect(agentd.requests).toEqual([
      {
        op: "run_goal",
        goal: "make a 4-bar house loop with an 808 bassline",
        timeout_class: "background",
        idempotency_key: "k1",
      },
    ]);
    const content = res.structuredContent as Record<string, unknown>;
    expect((content.result as Record<string, unknown>).status).toBe("success");
    await client.close();
  });

  it("returns an actionable error when LMMS is not running", async () => {
    const client = new LmmsClient({ host: "127.0.0.1", port: 1, timeoutMs: 1500 });
    await expect(client.callTool("getprojectstate")).rejects.toThrow(LmmsClientError);
    await expect(client.callTool("getprojectstate")).rejects.toThrow(/not reachable/);
  });

  it("draws automation through lmms_automate", async () => {
    const fake = new FakeAgentControl(() =>
      okEnvelope({ clip: "auto:Lead:0", nodes: 3 }, { automation_clips: 1 }),
    );
    await fake.listen();
    activeServers.push(fake);
    const { client } = await makeClient(fake);
    const res = await client.callTool({
      name: "lmms_automate",
      arguments: {
        address: "track:Lead.volume",
        ticks: [0, 192, 384],
        values: [0, 0.8, 0.2],
      },
    });
    expect(res.isError).toBeFalsy();
    expect(fake.requests).toEqual([
      {
        tool: "automate",
        args: {
          address: "track:Lead.volume",
          ticks: [0, 192, 384],
          values: [0, 0.8, 0.2],
        },
      },
    ]);
    const content = res.structuredContent as Record<string, unknown>;
    expect((content.result as Record<string, unknown>).nodes).toBe(3);
    expect((content.state_delta as Record<string, unknown>).automation_clips).toBe(1);
    await client.close();
  });

  it("rejects mismatched automation ticks and values before reaching LMMS", async () => {
    const fake = new FakeAgentControl(() => okEnvelope({}));
    await fake.listen();
    activeServers.push(fake);
    const { client } = await makeClient(fake);
    const res = await client.callTool({
      name: "lmms_automate",
      arguments: { address: "song.master.volume", ticks: [0, 192], values: [1] },
    });
    expect(res.isError).toBe(true);
    expect(fake.requests).toEqual([]);
    await client.close();
  });

  it("creates a mixer send through lmms_create_send", async () => {
    const fake = new FakeAgentControl(() => okEnvelope({ from: 3, to: 7, amount: 0.3 }));
    await fake.listen();
    activeServers.push(fake);
    const { client } = await makeClient(fake);
    const res = await client.callTool({
      name: "lmms_create_send",
      arguments: { from: 3, to: "Reverb", amount: 0.3 },
    });
    expect(res.isError).toBeFalsy();
    expect(fake.requests).toEqual([
      { tool: "create_send", args: { from: 3, to: "Reverb", amount: 0.3 } },
    ]);
    await client.close();
  });

  it("renders the song through lmms_render_song", async () => {
    const fake = new FakeAgentControl(() => okEnvelope({ render_id: "r1", status: "started" }));
    await fake.listen();
    activeServers.push(fake);
    const { client } = await makeClient(fake);
    const res = await client.callTool({
      name: "lmms_render_song",
      arguments: { path: "/tmp/out.wav", format: "wav", sample_rate: 48000 },
    });
    expect(res.isError).toBeFalsy();
    expect(fake.requests).toEqual([
      {
        tool: "render_song",
        args: { path: "/tmp/out.wav", format: "wav", sample_rate: 48000 },
      },
    ]);
    await client.close();
  });

  it("quantizes a clip through lmms_quantize_clip", async () => {
    const fake = new FakeAgentControl(() =>
      okEnvelope({ track: "Lead", clip_index: 0, notes_quantized: 8 }),
    );
    await fake.listen();
    activeServers.push(fake);
    const { client } = await makeClient(fake);
    const res = await client.callTool({
      name: "lmms_quantize_clip",
      arguments: { track: "Lead", clip_index: 0, resolution: 96 },
    });
    expect(res.isError).toBeFalsy();
    expect(fake.requests).toEqual([
      { tool: "quantize_clip", args: { track: "Lead", clip_index: 0, resolution: 96 } },
    ]);
    await client.close();
  });

  it("sets an effect parameter through lmms_set_effect_param", async () => {
    const fake = new FakeAgentControl(() =>
      okEnvelope({ effect: "ReverbSC", param: "Room Size", value: 0.5 }),
    );
    await fake.listen();
    activeServers.push(fake);
    const { client } = await makeClient(fake);
    const res = await client.callTool({
      name: "lmms_set_effect_param",
      arguments: { track: "Lead", effect: "reverb", param: "Room Size", value: 0.5 },
    });
    expect(res.isError).toBeFalsy();
    expect(fake.requests).toEqual([
      {
        tool: "set_effect_param",
        args: { track: "Lead", effect: "reverb", param: "Room Size", value: 0.5 },
      },
    ]);
    await client.close();
  });

  it("surfaces the confirmation gate for v2 typed tools (lmms_delete_track)", async () => {
    const fake = new FakeAgentControl(() =>
      okEnvelope({
        message: 'Confirmation required: "delete_track". Say "yes" to execute or "cancel".',
      }),
    );
    await fake.listen();
    activeServers.push(fake);
    const { client } = await makeClient(fake);
    const res = await client.callTool({ name: "lmms_delete_track", arguments: { track: "Bass" } });
    expect(res.isError).toBeFalsy();
    const content = res.structuredContent as Record<string, unknown>;
    expect(content.needs_confirmation).toBe(true);
    await client.close();
  });

  it("advertises the v2 resources and prompts", async () => {
    const fake = new FakeAgentControl(() => okEnvelope({}));
    await fake.listen();
    activeServers.push(fake);
    const { client } = await makeClient(fake);

    const resources = await client.listResources();
    const resourceNames = resources.resources.map((r) => r.name).sort();
    expect(resourceNames).toEqual(["da-capabilities", "da-state-schema", "da-workflows"]);

    const prompts = await client.listPrompts();
    const promptNames = prompts.prompts.map((p) => p.name).sort();
    expect(promptNames).toEqual([
      "lmms-arrange-song",
      "lmms-automate-a-sweep",
      "lmms-make-a-beat",
      "lmms-mix-and-master",
      "lmms-render-stems",
    ]);

    const schema = await client.readResource({ uri: "lmms://da/state-schema" });
    const text = (schema.contents[0] as { text: string }).text;
    expect(text).toContain("track:<name>.<param>");
    await client.close();
  });
});
