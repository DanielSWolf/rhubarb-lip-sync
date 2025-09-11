import * as path from 'path';
import * as os from 'os';
import fs from 'fs-extra';
import moize from 'moize';
import { path7z } from '7zip-bin-full';
import { execa } from 'execa';
import { downloadWithProgress } from './download-with-progress';

/**
 * Downloads an archive from the given URL and extracts it into the given target directory,
 * flattening single-child directory hierarchies.
 * Shows a progress bar during download.
 */
export const downloadAndExtractWithProgress = moize(
  async (url: string, targetDir: string, options: DownloadExtractOptions = {}): Promise<void> => {
    const description = options.description ?? 'archive file';
    const tempArchivePath = path.join(os.tmpdir(), encodeURIComponent(url));
    if (!fs.existsSync(tempArchivePath)) {
      await downloadWithProgress(url, tempArchivePath, description);
    }

    console.log(`Extracting ${description}.`);
    const archiveKind = options.archiveKind ?? detectArchiveKindFromUrl(url);
    fs.emptyDirSync(targetDir);
    await extractArchive(tempArchivePath, archiveKind, targetDir);

    flattenSingleChildDirectories(targetDir);
  },
  { maxSize: 1000, isDeepEqual: true },
);

export interface DownloadExtractOptions {
  /** File description used in progress messages. Default: "archive file" */
  description?: string;

  /** The kind of the archive. If not given, it is inferred from the URL. */
  archiveKind?: ArchiveKind;
}

const allArchiveKinds = ['tar.gz', 'tgz', 'tar.xz', 'tar.bz2', 'tar', 'zip', '7z'] as const;

export type ArchiveKind = (typeof allArchiveKinds)[number];

function detectArchiveKindFromUrl(url: string): ArchiveKind {
  const pathname = decodeURIComponent(new URL(url).pathname.toLowerCase());
  const kind = allArchiveKinds.find(kind => pathname.endsWith(`.${kind}`));
  if (!kind) throw new Error(`Unsupported archive type in URL: ${url}`);
  return kind;
}

/** Extracts the given archive file into the given output directory. */
async function extractArchive(
  archivePath: string,
  archiveKind: ArchiveKind,
  outputDirectory: string,
): Promise<void> {
  // Deactivate symlink security checks in 7-Zip, as they prevent extracting valid archives
  // https://sourceforge.net/p/sevenzip/discussion/45797/thread/da14cd780b/#:~:text=Command%20line%20switch-,%2Dsnld20,-can%20be%20used
  const allowSymlinks = '-snld20';

  // Extract single-stage archives
  const singleStageKinds = new Set<ArchiveKind>(['tar', 'zip', '7z']);
  if (singleStageKinds.has(archiveKind)) {
    await execa(path7z, ['x', archivePath, allowSymlinks, '-y', `-o${outputDirectory}`], {
      stdio: 'inherit',
    });
    return;
  }

  // Extract multi-stage archives into a temp dir first
  const stageDir = getTempPath('stage');
  try {
    await execa(path7z, ['x', archivePath, '-y', `-o${stageDir}`], { stdio: 'inherit' });

    const entries = await fs.promises.readdir(stageDir, { withFileTypes: true });
    const tar = entries
      .filter(e => e.isFile() && e.name.toLowerCase().endsWith('.tar'))
      .map(e => path.join(stageDir, e.name))[0];
    if (!tar) throw new Error(`No .tar file found in ${stageDir}.`);

    await execa(path7z, ['x', tar, '-y', allowSymlinks, `-o${outputDirectory}`], {
      stdio: 'inherit',
    });
  } finally {
    await fs.promises.rm(stageDir, { recursive: true, force: true });
  }
}

function flattenSingleChildDirectories(dir: string): void {
  while (true) {
    const children = fs.readdirSync(dir, { withFileTypes: true });
    if (children.length !== 1) return;

    const child = children[0];
    if (!child.isDirectory()) return;

    const tmpDir = dir + '.tmp';
    fs.moveSync(path.join(dir, child.name), tmpDir, { overwrite: true });
    fs.removeSync(dir);
    fs.moveSync(tmpDir, dir);
  }
}

export function getTempPath(prefix: string): string {
  return path.join(os.tmpdir(), `${prefix}-${Date.now()}-${Math.random().toString(36).slice(2)}`);
}
