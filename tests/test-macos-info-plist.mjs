import assert from 'node:assert/strict';
import { readFile } from 'node:fs/promises';

const infoPlistPath = process.argv[2];
assert.ok(infoPlistPath, 'InfoPlist.strings path is required');

const contents = await readFile(infoPlistPath);
assert.ok(contents.length >= 2, 'InfoPlist.strings must not be empty');
assert.equal(
  contents.readUInt16BE(0),
  0xfeff,
  'InfoPlist.strings must use UTF-16 big-endian encoding'
);

const decoded = new TextDecoder('utf-16be', { fatal: true }).decode(contents);
const bundleName = /^CFBundleName\s*=\s*"([^"]+)";/m.exec(decoded);

assert.ok(bundleName, 'InfoPlist.strings must define CFBundleName');
assert.equal(
  bundleName[1],
  'XPilotInfinity',
  'the localized bundle name must match the XPilot Infinity application bundle'
);

process.stdout.write('macOS bundle name is XPilotInfinity\n');
