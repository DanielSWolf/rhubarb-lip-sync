/// <reference types="npm:@types/node" />
import { basename, dirname, join } from 'jsr:@std/path';
// @deno-types="npm:@types/fs-extra"
import { ensureDir } from 'fs-extra';
import ky from 'ky';
// @deno-types="npm:@types/decompress"
import decompress from 'decompress';

export async function downloadAndExtractWithProgress(
  url: string,
  targetDir: string
): Promise<void> {
  // Download archive to memory buffer
  const archiveFileName = basename(url);
  const logDownloadProgress = createProgressLogger(`Downloading ${archiveFileName}`);
  logDownloadProgress(0);
  const response = await ky.get(url, {
    onDownloadProgress: progress => logDownloadProgress(progress.percent),
  });
  const buffer = Buffer; // await response.arrayBuffer();

  // Extract archive to temporary directory
  const workingDir = dirname(targetDir);
  const tempDir = join(workingDir, `~${basename(targetDir)}`);
  await ensureDir(tempDir);
  await decompress(buffer, tempDir);
}

/** Returns a function that can be used as a callback for logging progress. */
function createProgressLogger(message: string) {
  const minInterval = 2000; // ms

  let lastLoggedPercentage = -1;
  let lastLoggingTimestamp = 0;
  return (percentage: number) => {
    const percentageInt = Math.trunc(percentage);
    const timestamp = Date.now();

    if (percentageInt === lastLoggedPercentage) return;
    if (timestamp - lastLoggingTimestamp < minInterval && percentageInt < 100) return;

    console.log(`${message} [${percentageInt}%]`);
    lastLoggedPercentage = percentageInt;
    lastLoggingTimestamp = timestamp;
  };
}
