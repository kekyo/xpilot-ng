/*
 * TCP stream proxy for fixed-session reconnection E2E scenarios.
 *
 * The proxy is transport-agnostic: it forwards both length-prefixed TCP
 * sessions and WebSocket sessions without interpreting their records. Once
 * the caller creates the trigger file, the first stream is severed and new
 * connections are rejected briefly before forwarding resumes. For TCP
 * sessions it also submits one deliberately corrupted resumption token.
 */

import fs from "node:fs";
import net from "node:net";

const [actualHost, proxyHost, portText, triggerFile, stateFile, transport] =
  process.argv.slice(2);
const port = Number(portText);

if (
  !actualHost ||
  !proxyHost ||
  !Number.isInteger(port) ||
  port <= 0 ||
  port > 65535 ||
  !triggerFile ||
  !stateFile ||
  !["tcp", "websocket"].includes(transport)
) {
  throw new Error(
    "usage: session-reconnect-proxy.mjs ACTUAL_HOST PROXY_HOST PORT " +
      "TRIGGER_FILE STATE_FILE TRANSPORT",
  );
}

const sockets = new Set();
let reconnectAllowedAt = 0;
let resumeToken = null;
let resumed = false;
let triggered = false;
let invalidProbeTimer = null;

const recordState = (state) => {
  fs.appendFileSync(stateFile, `${state}\n`);
};

const trackSocket = (socket) => {
  sockets.add(socket);
  socket.on("close", () => sockets.delete(socket));
  socket.on("error", () => socket.destroy());
  return socket;
};

const inspectTcpServerRecords = () => {
  let buffered = Buffer.alloc(0);

  return (chunk) => {
    buffered = Buffer.concat([buffered, chunk]);
    while (buffered.length >= 2) {
      const payloadLength = buffered.readUInt16BE(0);
      const frameLength = payloadLength + 2;

      if (buffered.length < frameLength) return;
      const payload = buffered.subarray(2, frameLength);
      buffered = buffered.subarray(frameLength);

      // PKT_RELIABLE has an eleven-byte header. The initial reliable stream
      // then carries PKT_MAGIC plus its u32 value before PKT_SESSION_TOKEN.
      if (
        resumeToken === null &&
        payload.length >= 33 &&
        payload[0] === 42 &&
        payload[11] === 41 &&
        payload[16] === 64
      ) {
        resumeToken = Buffer.from(payload.subarray(17, 33));
        recordState("token-captured");
      }
    }
  };
};

const probeInvalidResume = () => {
  if (resumeToken === null) {
    recordState("invalid-probe-missing-token");
    return;
  }

  const invalidToken = Buffer.from(resumeToken);
  const payload = Buffer.alloc(21);
  const frame = Buffer.alloc(23);
  let buffered = Buffer.alloc(0);
  let finished = false;

  invalidToken[0] ^= 1;
  payload.writeUInt16BE(0xf4ed, 0);
  payload[2] = 2;
  payload[3] = 1;
  payload[4] = 3;
  invalidToken.copy(payload, 5);
  frame.writeUInt16BE(payload.length, 0);
  payload.copy(frame, 2);

  const probe = net.createConnection({
    host: actualHost,
    port,
    localAddress: proxyHost,
  });
  sockets.add(probe);
  probe.on("close", () => sockets.delete(probe));
  probe.on("connect", () => probe.write(frame));
  probe.on("data", (chunk) => {
    buffered = Buffer.concat([buffered, chunk]);
    while (buffered.length >= 2) {
      const payloadLength = buffered.readUInt16BE(0);
      const frameLength = payloadLength + 2;

      if (buffered.length < frameLength) return;
      const reply = buffered.subarray(2, frameLength);
      buffered = buffered.subarray(frameLength);
      if (
        reply.length >= 6 &&
        reply.readUInt16BE(0) === 0xf4ed &&
        reply[2] === 2 &&
        reply[3] === 4 &&
        reply[4] === 7
      ) {
        finished = true;
        recordState("invalid-rejected");
        probe.end();
        return;
      }
      finished = true;
      recordState("invalid-probe-bad-reply");
      probe.destroy();
      return;
    }
  });
  probe.on("timeout", () => {
    if (!finished) recordState("invalid-probe-timeout");
    probe.destroy();
  });
  probe.on("error", (error) => {
    if (!finished) {
      recordState("invalid-probe-error");
      process.stderr.write(`${error.stack}\n`);
    }
  });
  probe.setTimeout(1500);
};

const relaySession = (client) => {
  trackSocket(client);

  if (triggered && Date.now() < reconnectAllowedAt) {
    client.destroy();
    return;
  }

  const upstream = trackSocket(
    net.createConnection({
      host: actualHost,
      port,
      localAddress: proxyHost,
    }),
  );
  upstream.on("connect", () => {
    if (triggered && !resumed) {
      resumed = true;
      recordState("resumed");
    }
    client.pipe(upstream);
    upstream.pipe(client);
  });
  if (transport === "tcp")
    upstream.on("data", inspectTcpServerRecords());
};

const sessionServer = net.createServer(relaySession);
sessionServer.on("error", (error) => {
  process.stderr.write(`${error.stack}\n`);
  process.exitCode = 1;
});
sessionServer.listen(port, proxyHost, () => recordState("ready"));

const triggerTimer = setInterval(() => {
  if (triggered || !fs.existsSync(triggerFile)) return;
  triggered = true;
  reconnectAllowedAt = Date.now() + 2000;
  for (const socket of sockets) socket.destroy();
  recordState("dropped");
  if (transport === "tcp")
    invalidProbeTimer = setTimeout(probeInvalidResume, 100);
}, 25);

const stop = () => {
  clearInterval(triggerTimer);
  if (invalidProbeTimer !== null) clearTimeout(invalidProbeTimer);
  for (const socket of sockets) socket.destroy();
  sessionServer.close(() => process.exit(process.exitCode ?? 0));
};

process.on("SIGTERM", stop);
process.on("SIGINT", stop);
