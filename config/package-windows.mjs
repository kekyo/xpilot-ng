import { mkdir, readdir, readFile, stat, writeFile } from 'node:fs/promises';
import { dirname, isAbsolute, join, relative, resolve, sep } from 'node:path';
import { fileURLToPath } from 'node:url';

const minimumZipDate = new Date(Date.UTC(1980, 0, 1, 0, 0, 0));
const maximumZipEntryCount = 0xffff;
const maximumZipValue = 0xffffffff;
const utf8FileNameFlag = 0x0800;

const crc32Table = new Uint32Array(256);

for (let value = 0; value < crc32Table.length; value += 1) {
  let crc = value;

  for (let bit = 0; bit < 8; bit += 1) {
    crc = (crc & 1) === 1 ? 0xedb88320 ^ (crc >>> 1) : crc >>> 1;
  }

  crc32Table[value] = crc >>> 0;
}

const crc32 = (buffer) => {
  let crc = 0xffffffff;

  for (const byte of buffer) {
    crc = crc32Table[(crc ^ byte) & 0xff] ^ (crc >>> 8);
  }

  return (crc ^ 0xffffffff) >>> 0;
};

const toDosDateTime = (date) => {
  const normalizedDate = date < minimumZipDate ? minimumZipDate : date;
  const year = normalizedDate.getUTCFullYear();
  const month = normalizedDate.getUTCMonth() + 1;
  const day = normalizedDate.getUTCDate();
  const hours = normalizedDate.getUTCHours();
  const minutes = normalizedDate.getUTCMinutes();
  const seconds = Math.floor(normalizedDate.getUTCSeconds() / 2);

  return {
    date: ((year - 1980) << 9) | (month << 5) | day,
    time: (hours << 11) | (minutes << 5) | seconds,
  };
};

const writeUInt16 = (buffer, value, offset) => {
  buffer.writeUInt16LE(value, offset);
};

const writeUInt32 = (buffer, value, offset) => {
  buffer.writeUInt32LE(value >>> 0, offset);
};

const assertClassicZipValue = (value, description) => {
  if (!Number.isSafeInteger(value) || value < 0 || value > maximumZipValue) {
    throw new Error(`${description} exceeds the classic ZIP limit.`);
  }
};

const createLocalFileHeader = (entry) => {
  const header = Buffer.alloc(30);

  writeUInt32(header, 0x04034b50, 0);
  writeUInt16(header, 10, 4);
  writeUInt16(header, utf8FileNameFlag, 6);
  writeUInt16(header, 0, 8);
  writeUInt16(header, entry.modified.time, 10);
  writeUInt16(header, entry.modified.date, 12);
  writeUInt32(header, entry.crc, 14);
  writeUInt32(header, entry.content.length, 18);
  writeUInt32(header, entry.content.length, 22);
  writeUInt16(header, entry.fileName.length, 26);
  writeUInt16(header, 0, 28);

  return Buffer.concat([header, entry.fileName]);
};

const createCentralDirectoryHeader = (entry) => {
  const header = Buffer.alloc(46);

  writeUInt32(header, 0x02014b50, 0);
  writeUInt16(header, 20, 4);
  writeUInt16(header, 10, 6);
  writeUInt16(header, utf8FileNameFlag, 8);
  writeUInt16(header, 0, 10);
  writeUInt16(header, entry.modified.time, 12);
  writeUInt16(header, entry.modified.date, 14);
  writeUInt32(header, entry.crc, 16);
  writeUInt32(header, entry.content.length, 20);
  writeUInt32(header, entry.content.length, 24);
  writeUInt16(header, entry.fileName.length, 28);
  writeUInt16(header, 0, 30);
  writeUInt16(header, 0, 32);
  writeUInt16(header, 0, 34);
  writeUInt16(header, 0, 36);
  writeUInt32(header, 0, 38);
  writeUInt32(header, entry.localHeaderOffset, 42);

  return Buffer.concat([header, entry.fileName]);
};

const createEndOfCentralDirectory = ({
  entryCount,
  centralDirectorySize,
  centralDirectoryOffset,
}) => {
  const header = Buffer.alloc(22);

  writeUInt32(header, 0x06054b50, 0);
  writeUInt16(header, 0, 4);
  writeUInt16(header, 0, 6);
  writeUInt16(header, entryCount, 8);
  writeUInt16(header, entryCount, 10);
  writeUInt32(header, centralDirectorySize, 12);
  writeUInt32(header, centralDirectoryOffset, 16);
  writeUInt16(header, 0, 20);

  return header;
};

const findArchiveInputs = async (rootDirectory, directory, prefix) => {
  const directoryEntries = await readdir(directory, { withFileTypes: true });
  directoryEntries.sort((left, right) =>
    left.name < right.name ? -1 : left.name > right.name ? 1 : 0
  );

  const inputs = [];
  for (const directoryEntry of directoryEntries) {
    const path = join(directory, directoryEntry.name);
    const name = prefix
      ? `${prefix}/${directoryEntry.name}`
      : directoryEntry.name;

    if (directoryEntry.isDirectory()) {
      inputs.push(...(await findArchiveInputs(rootDirectory, path, name)));
    } else if (directoryEntry.isFile()) {
      inputs.push({ name, path });
    } else {
      throw new Error(`Unsupported package entry: ${relative(rootDirectory, path)}`);
    }
  }

  return inputs;
};

const createZipArchive = async (outputPath, inputs) => {
  if (inputs.length > maximumZipEntryCount) {
    throw new Error('The package contains too many files for a classic ZIP archive.');
  }

  const localParts = [];
  const centralParts = [];
  const entries = [];
  let offset = 0;

  for (const input of inputs) {
    const content = await readFile(input.path);
    const fileName = Buffer.from(input.name, 'utf8');

    if (fileName.length > maximumZipEntryCount) {
      throw new Error(`Archive file name is too long: ${input.name}`);
    }
    assertClassicZipValue(content.length, `Archive entry ${input.name}`);
    assertClassicZipValue(offset, 'Archive offset');

    const entry = {
      content,
      crc: crc32(content),
      fileName,
      localHeaderOffset: offset,
      modified: toDosDateTime(minimumZipDate),
    };
    const localHeader = createLocalFileHeader(entry);

    localParts.push(localHeader, content);
    offset += localHeader.length + content.length;
    entries.push(entry);
  }

  const centralDirectoryOffset = offset;
  assertClassicZipValue(centralDirectoryOffset, 'Central directory offset');

  for (const entry of entries) {
    const centralHeader = createCentralDirectoryHeader(entry);
    centralParts.push(centralHeader);
    offset += centralHeader.length;
  }

  const centralDirectorySize = offset - centralDirectoryOffset;
  assertClassicZipValue(centralDirectorySize, 'Central directory size');
  const endOfCentralDirectory = createEndOfCentralDirectory({
    centralDirectoryOffset,
    centralDirectorySize,
    entryCount: entries.length,
  });

  await mkdir(dirname(outputPath), { recursive: true });
  await writeFile(
    outputPath,
    Buffer.concat([...localParts, ...centralParts, endOfCentralDirectory])
  );
};

/**
 * Creates a deterministic ZIP archive from a Windows package directory.
 *
 * @param {{inputDirectory: string, outputPath: string}} options Packaging paths.
 * @returns {Promise<string>} The absolute path of the generated archive.
 * @remarks Symbolic links and non-file package entries are rejected. ZIP64 is
 * not emitted because XPilot Infinity distribution packages are expected to remain
 * within classic ZIP limits.
 */
export const createWindowsArchive = async ({ inputDirectory, outputPath }) => {
  const resolvedInputDirectory = resolve(inputDirectory);
  const resolvedOutputPath = resolve(outputPath);
  const inputMetadata = await stat(resolvedInputDirectory);

  if (!inputMetadata.isDirectory()) {
    throw new Error(`Package input is not a directory: ${resolvedInputDirectory}`);
  }

  const outputRelativeToInput = relative(
    resolvedInputDirectory,
    resolvedOutputPath
  );
  if (
    outputRelativeToInput === '' ||
    (!outputRelativeToInput.startsWith(`..${sep}`) &&
      outputRelativeToInput !== '..' &&
      !isAbsolute(outputRelativeToInput))
  ) {
    throw new Error('Archive output must be outside the package directory.');
  }

  const inputs = await findArchiveInputs(
    resolvedInputDirectory,
    resolvedInputDirectory,
    ''
  );
  inputs.sort((left, right) =>
    left.name < right.name ? -1 : left.name > right.name ? 1 : 0
  );
  if (inputs.length === 0) {
    throw new Error('The Windows package directory is empty.');
  }

  await createZipArchive(resolvedOutputPath, inputs);
  return resolvedOutputPath;
};

const usage = () => {
  process.stdout.write(
    'Usage: node config/package-windows.mjs --input PATH --output PATH\n'
  );
};

const parseArguments = (argumentsToParse) => {
  let inputDirectory;
  let outputPath;

  for (let index = 0; index < argumentsToParse.length; index += 1) {
    const argument = argumentsToParse[index];
    if (argument === '--help') {
      usage();
      process.exit(0);
    } else if (argument === '--input') {
      index += 1;
      inputDirectory = argumentsToParse[index];
    } else if (argument.startsWith('--input=')) {
      inputDirectory = argument.slice('--input='.length);
    } else if (argument === '--output') {
      index += 1;
      outputPath = argumentsToParse[index];
    } else if (argument.startsWith('--output=')) {
      outputPath = argument.slice('--output='.length);
    } else {
      throw new Error(`Unknown option: ${argument}`);
    }
  }

  if (!inputDirectory || !outputPath) {
    throw new Error('--input and --output are required.');
  }

  return { inputDirectory, outputPath };
};

const main = async () => {
  const options = parseArguments(process.argv.slice(2));
  const archivePath = await createWindowsArchive(options);
  process.stdout.write(`Created ${archivePath}\n`);
};

const modulePath = fileURLToPath(import.meta.url);
if (process.argv[1] && resolve(process.argv[1]) === modulePath) {
  await main();
}
