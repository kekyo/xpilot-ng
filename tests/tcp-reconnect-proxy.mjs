/*
 * TCP proxy fixture for the gameplay reconnection E2E scenario.
 *
 * The contact reply is rewritten to point at a gameplay proxy.  Once the
 * caller creates the trigger file, the first gameplay stream is severed and
 * new gameplay connections are held off briefly before forwarding resumes.
 */

import fs from "node:fs";
import net from "node:net";

const [actualHost, proxyHost, contactPortText, triggerFile, stateFile] =
  process.argv.slice(2);
const contactPort = Number(contactPortText);

if (
  !actualHost ||
  !proxyHost ||
  !Number.isInteger(contactPort) ||
  contactPort <= 0 ||
  contactPort > 65535 ||
  !triggerFile ||
  !stateFile
) {
  throw new Error(
    "usage: tcp-reconnect-proxy.mjs ACTUAL_HOST PROXY_HOST CONTACT_PORT " +
      "TRIGGER_FILE STATE_FILE",
  );
}

const sockets = new Set();
let gameplayServer;
let actualGameplayPort;
let gameplayConnections = 0;
let reconnectAllowedAt = 0;
let triggerTimer;

const recordState = (state) => {
  fs.appendFileSync(stateFile, `${state}\n`);
};

const trackSocket = (socket) => {
  sockets.add(socket);
  socket.on("close", () => sockets.delete(socket));
  socket.on("error", () => socket.destroy());
  return socket;
};

const relayGameplay = (client) => {
  trackSocket(client);
  gameplayConnections += 1;

  if (gameplayConnections > 1 && Date.now() < reconnectAllowedAt) {
    client.destroy();
    return;
  }

  const upstream = trackSocket(
    net.createConnection({
      host: actualHost,
      port: actualGameplayPort,
      localAddress: proxyHost,
    }),
  );
  upstream.on("connect", () => {
    if (gameplayConnections > 1) recordState("resumed");
    client.pipe(upstream);
    upstream.pipe(client);
  });

  if (gameplayConnections !== 1) return;
  triggerTimer = setInterval(() => {
    if (!fs.existsSync(triggerFile)) return;
    clearInterval(triggerTimer);
    triggerTimer = undefined;
    reconnectAllowedAt = Date.now() + 2000;
    client.destroy();
    upstream.destroy();
    recordState("dropped");
  }, 25);
};

const processContactReply = (replyRecord, contactClient) => {
  const payloadLength = replyRecord.readUInt16BE(0);
  if (payloadLength < 6 || replyRecord.length !== payloadLength + 2) {
    contactClient.destroy(new Error("invalid framed contact reply"));
    return;
  }
  const replyType = replyRecord[6];
  const status = replyRecord[7];
  if (replyType !== 0 || status !== 0) {
    contactClient.end(replyRecord);
    return;
  }
  if (payloadLength < 8) {
    contactClient.destroy(new Error("incomplete enter-game reply"));
    return;
  }
  actualGameplayPort = replyRecord.readUInt16BE(8);
  gameplayServer = net.createServer(relayGameplay);
  gameplayServer.on("error", (error) => {
    process.stderr.write(`${error.stack}\n`);
    process.exitCode = 1;
  });
  gameplayServer.listen(0, proxyHost, () => {
    const address = gameplayServer.address();
    if (typeof address === "string" || address === null) {
      contactClient.destroy(new Error("gameplay proxy has no TCP address"));
      return;
    }
    replyRecord.writeUInt16BE(address.port, 8);
    contactClient.end(replyRecord);
    recordState("ready");
  });
};

const contactServer = net.createServer((contactClient) => {
  trackSocket(contactClient);
  const upstream = trackSocket(
    net.createConnection({
      host: actualHost,
      port: contactPort,
      localAddress: proxyHost,
    }),
  );
  contactClient.pipe(upstream);

  let response = Buffer.alloc(0);
  upstream.on("data", (chunk) => {
    response = Buffer.concat([response, chunk]);
    if (response.length < 2) return;
    const recordLength = response.readUInt16BE(0) + 2;
    if (response.length < recordLength) return;
    upstream.destroy();
    processContactReply(Buffer.from(response.subarray(0, recordLength)), contactClient);
  });
});

contactServer.on("error", (error) => {
  process.stderr.write(`${error.stack}\n`);
  process.exitCode = 1;
});
contactServer.listen(contactPort, proxyHost, () => recordState("contact-ready"));

const stop = () => {
  if (triggerTimer !== undefined) clearInterval(triggerTimer);
  for (const socket of sockets) socket.destroy();
  if (gameplayServer !== undefined) gameplayServer.close();
  contactServer.close(() => process.exit(process.exitCode ?? 0));
};

process.on("SIGTERM", stop);
process.on("SIGINT", stop);
