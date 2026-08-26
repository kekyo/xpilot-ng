import assert from 'node:assert/strict';
import {
  mkdir,
  mkdtemp,
  readFile,
  rm,
  writeFile,
} from 'node:fs/promises';
import { tmpdir } from 'node:os';
import { join, resolve } from 'node:path';
import { pathToFileURL } from 'node:url';

const readUInt16 = (buffer, offset) => buffer.readUInt16LE(offset);
const readUInt32 = (buffer, offset) => buffer.readUInt32LE(offset);

const readZipEntries = async (archivePath) => {
  const buffer = await readFile(archivePath);
  let endOfCentralDirectory = -1;

  for (let index = buffer.length - 22; index >= 0; index -= 1) {
    if (readUInt32(buffer, index) === 0x06054b50) {
      endOfCentralDirectory = index;
      break;
    }
  }

  assert.ok(endOfCentralDirectory >= 0, 'end of central directory is missing');

  const entryCount = readUInt16(buffer, endOfCentralDirectory + 10);
  const centralDirectoryOffset = readUInt32(
    buffer,
    endOfCentralDirectory + 16
  );
  const entries = new Map();
  let offset = centralDirectoryOffset;

  for (let index = 0; index < entryCount; index += 1) {
    assert.equal(readUInt32(buffer, offset), 0x02014b50);

    const compressionMethod = readUInt16(buffer, offset + 10);
    const compressedSize = readUInt32(buffer, offset + 20);
    const fileNameLength = readUInt16(buffer, offset + 28);
    const extraLength = readUInt16(buffer, offset + 30);
    const commentLength = readUInt16(buffer, offset + 32);
    const localHeaderOffset = readUInt32(buffer, offset + 42);
    const fileNameOffset = offset + 46;
    const name = buffer
      .subarray(fileNameOffset, fileNameOffset + fileNameLength)
      .toString('utf8');

    assert.equal(compressionMethod, 0);
    assert.equal(readUInt32(buffer, localHeaderOffset), 0x04034b50);

    const localFileNameLength = readUInt16(buffer, localHeaderOffset + 26);
    const localExtraLength = readUInt16(buffer, localHeaderOffset + 28);
    const contentOffset =
      localHeaderOffset + 30 + localFileNameLength + localExtraLength;
    const content = buffer.subarray(contentOffset, contentOffset + compressedSize);

    entries.set(name, content);
    offset += 46 + fileNameLength + extraLength + commentLength;
  }

  return entries;
};

const archiveModulePath = process.argv[2];
assert.ok(archiveModulePath, 'archive module path is required');

const { createWindowsArchive } = await import(
  pathToFileURL(resolve(archiveModulePath)).href
);
assert.equal(typeof createWindowsArchive, 'function');

const temporaryDirectory = await mkdtemp(
  join(tmpdir(), 'xpilot-windows-archive-')
);

try {
  const packageDirectory = join(temporaryDirectory, 'package');
  const firstArchive = join(temporaryDirectory, 'first.zip');
  const secondArchive = join(temporaryDirectory, 'second.zip');

  await mkdir(join(packageDirectory, 'lib', 'maps'), { recursive: true });
  await writeFile(join(packageDirectory, 'COPYING'), 'license text');
  await writeFile(join(packageDirectory, 'xpilot-ng-server.exe'), 'server');
  await writeFile(join(packageDirectory, 'xpilot-ng-sdl.exe'), 'client');
  await writeFile(join(packageDirectory, 'lib.txt'), 'top-level data');
  await writeFile(join(packageDirectory, 'lib', 'defaults.txt'), 'defaults');
  await writeFile(join(packageDirectory, 'lib', 'maps', 'ndh.xp2'), 'map');

  await createWindowsArchive({
    inputDirectory: packageDirectory,
    outputPath: firstArchive,
  });
  await createWindowsArchive({
    inputDirectory: packageDirectory,
    outputPath: secondArchive,
  });

  assert.deepEqual(
    await readFile(firstArchive),
    await readFile(secondArchive),
    'identical package trees must produce byte-identical archives'
  );

  const entries = await readZipEntries(firstArchive);
  assert.deepEqual([...entries.keys()], [
    'COPYING',
    'lib.txt',
    'lib/defaults.txt',
    'lib/maps/ndh.xp2',
    'xpilot-ng-sdl.exe',
    'xpilot-ng-server.exe',
  ]);
  assert.equal(entries.get('COPYING').toString('utf8'), 'license text');
  assert.equal(entries.get('lib/maps/ndh.xp2').toString('utf8'), 'map');
  assert.equal(entries.get('xpilot-ng-sdl.exe').toString('utf8'), 'client');

  process.stdout.write('Deterministic Windows archive generation passed\n');
} finally {
  await rm(temporaryDirectory, { recursive: true, force: true });
}
