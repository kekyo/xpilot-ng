import { spawn } from 'node:child_process';
import {
  chmod,
  copyFile,
  mkdir,
  mkdtemp,
  readdir,
  rm,
  stat,
  utimes,
} from 'node:fs/promises';
import { tmpdir } from 'node:os';
import { dirname, isAbsolute, join, relative, resolve, sep } from 'node:path';
import { fileURLToPath } from 'node:url';

const fixedZipDate = new Date(Date.UTC(1980, 0, 1, 0, 0, 0));

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

const stageArchiveInputs = async (stagingDirectory, inputs) => {
  for (const input of inputs) {
    if (/[\r\n]/u.test(input.name)) {
      throw new Error(`Archive file name contains a newline: ${input.name}`);
    }
    const stagedPath = join(stagingDirectory, input.name);
    await mkdir(dirname(stagedPath), { recursive: true });
    await copyFile(input.path, stagedPath);
    await chmod(stagedPath, 0o644);
    await utimes(stagedPath, fixedZipDate, fixedZipDate);
  }
};

const runZip = async ({ outputPath, stagingDirectory, inputNames }) => {
  const zipProgram = 'zip';
  const zipEnvironment = { ...process.env, TZ: 'UTC' };

  // Info-ZIP interprets ZIP and ZIPOPT as implicit command-line options.
  // Ignoring them prevents host settings from changing the output or metadata.
  delete zipEnvironment.ZIP;
  delete zipEnvironment.ZIPOPT;

  await new Promise((resolveProcess, rejectProcess) => {
    const standardError = [];
    let inputError;
    const zipProcess = spawn(
      zipProgram,
      ['-9', '-X', '-D', '-q', outputPath, '-@'],
      {
        cwd: stagingDirectory,
        env: zipEnvironment,
        stdio: ['pipe', 'ignore', 'pipe'],
      }
    );

    zipProcess.stderr.on('data', (chunk) => {
      standardError.push(chunk);
    });
    zipProcess.stdin.on('error', (error) => {
      inputError = error;
    });
    zipProcess.once('error', (error) => {
      rejectProcess(new Error(`Unable to run ${zipProgram}: ${error.message}`));
    });
    zipProcess.once('close', (code) => {
      if (inputError && inputError.code !== 'EPIPE') {
        rejectProcess(
          new Error(`Unable to provide the ZIP input list: ${inputError.message}`)
        );
        return;
      }
      if (code !== 0) {
        const detail = Buffer.concat(standardError).toString('utf8').trim();
        rejectProcess(
          new Error(
            detail
              ? `${zipProgram} failed with exit code ${code}: ${detail}`
              : `${zipProgram} failed with exit code ${code}.`
          )
        );
        return;
      }
      resolveProcess();
    });

    zipProcess.stdin.end(`${inputNames.join('\n')}\n`, 'utf8');
  });
};

const createZipArchive = async (outputPath, inputs) => {
  await mkdir(dirname(outputPath), { recursive: true });
  await rm(outputPath, { force: true });

  const stagingDirectory = await mkdtemp(
    join(tmpdir(), 'xpilot-windows-archive-')
  );
  try {
    await stageArchiveInputs(stagingDirectory, inputs);
    await runZip({
      inputNames: inputs.map((input) => input.name),
      outputPath,
      stagingDirectory,
    });
    let outputMetadata;
    try {
      outputMetadata = await stat(outputPath);
    } catch {
      throw new Error(`ZIP output was not created: ${outputPath}`);
    }
    if (!outputMetadata.isFile()) {
      throw new Error(`ZIP output is not a file: ${outputPath}`);
    }
  } catch (error) {
    await rm(outputPath, { force: true });
    throw error;
  } finally {
    await rm(stagingDirectory, { recursive: true, force: true });
  }
};

/**
 * Creates a deterministic ZIP archive from a Windows package directory.
 *
 * @param {{inputDirectory: string, outputPath: string}} options Packaging paths.
 * @returns {Promise<string>} The absolute path of the generated archive.
 * @remarks Info-ZIP `zip` is run at maximum compression. Symbolic links and
 * non-file package entries are rejected. File order, timestamps, permissions,
 * and extra metadata are normalized so identical inputs remain reproducible.
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
