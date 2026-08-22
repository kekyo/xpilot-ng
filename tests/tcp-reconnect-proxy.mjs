/*
 * UDP contact and TCP gameplay proxy for the reconnection E2E scenario.
 *
 * The datagram contact reply is rewritten to point at a TCP gameplay proxy.
 * Once the caller creates the trigger file, the first gameplay stream is
 * severed and new gameplay connections are held off briefly before
 * forwarding resumes.
 */

import dgram from "node:dgram";
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
let proxyGameplayPort;
let gameplayConnections = 0;
let reconnectAllowedAt = 0;
let triggerTimer;
let contactPeer;
let pendingContactReply;

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

const sendContactReply = (reply) => {
  if (contactPeer === undefined) return;
  contactSocket.send(reply, contactPeer.port, contactPeer.address);
};

const processContactReply = (replyRecord) => {
  if (replyRecord.length < 6) {
    throw new Error("invalid datagram contact reply");
  }
  const replyType = replyRecord[4];
  const status = replyRecord[5];
  if (replyType !== 0 || status !== 0) {
    sendContactReply(replyRecord);
    return;
  }
  if (replyRecord.length < 8) {
    throw new Error("incomplete enter-game reply");
  }
  actualGameplayPort = replyRecord.readUInt16BE(6);
  if (proxyGameplayPort !== undefined) {
    replyRecord.writeUInt16BE(proxyGameplayPort, 6);
    sendContactReply(replyRecord);
    return;
  }
  pendingContactReply = replyRecord;
  if (gameplayServer !== undefined) return;
  gameplayServer = net.createServer(relayGameplay);
  gameplayServer.on("error", (error) => {
    process.stderr.write(`${error.stack}\n`);
    process.exitCode = 1;
  });
  gameplayServer.listen(0, proxyHost, () => {
    const address = gameplayServer.address();
    if (typeof address === "string" || address === null) {
      throw new Error("gameplay proxy has no TCP address");
    }
    proxyGameplayPort = address.port;
    pendingContactReply.writeUInt16BE(proxyGameplayPort, 6);
    sendContactReply(pendingContactReply);
    pendingContactReply = undefined;
    recordState("ready");
  });
};

const contactSocket = dgram.createSocket("udp4");
contactSocket.on("message", (message, remote) => {
  if (remote.address === actualHost && remote.port === contactPort) {
    processContactReply(Buffer.from(message));
    return;
  }
  contactPeer = remote;
  contactSocket.send(message, contactPort, actualHost);
});
contactSocket.on("error", (error) => {
  process.stderr.write(`${error.stack}\n`);
  process.exitCode = 1;
});
contactSocket.bind(contactPort, proxyHost, () => recordState("contact-ready"));

const stop = () => {
  if (triggerTimer !== undefined) clearInterval(triggerTimer);
  for (const socket of sockets) socket.destroy();
  if (gameplayServer !== undefined) gameplayServer.close();
  contactSocket.close(() => process.exit(process.exitCode ?? 0));
};

process.on("SIGTERM", stop);
process.on("SIGINT", stop);
