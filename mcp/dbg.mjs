import { Client } from "@modelcontextprotocol/sdk/client/index.js";
import { InMemoryTransport } from "@modelcontextprotocol/sdk/inMemory.js";
import { createServer } from "./dist/server.js";

const server = createServer({ repoRoot: new URL("./test/fixtures/repo", import.meta.url).pathname });
const [clientTransport, serverTransport] = InMemoryTransport.createLinkedPair();
const client = new Client({ name: "t", version: "0" });
client.onerror = (e) => console.error("client onerror:", e);
server.onerror = (e) => console.error("server onerror:", e);
const t0 = Date.now();
try {
  await client.connect(clientTransport);
  console.log("client connected", Date.now() - t0);
} catch (e) { console.error("client connect failed:", e); }
try {
  await server.connect(serverTransport);
  console.log("server connected", Date.now() - t0);
} catch (e) { console.error("server connect failed:", e); }
try {
  const tools = await client.listTools();
  console.log("tools:", tools.tools.length, Date.now() - t0);
} catch (e) { console.error("listTools failed:", e); }
process.exit(0);
