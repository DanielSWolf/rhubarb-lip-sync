import { execa, Options as ExecaOptions, ResultPromise as ExecaResultPromise } from 'execa';
import { extname, join as joinPaths } from 'path';
import os from 'os';
import fs from 'fs';
import { ArchiveKind, downloadAndExtractWithProgress } from './download-and-extract-with-progress';
import { device } from './device';
import { chmod } from 'fs/promises';
import { downloadWithProgress } from './download-with-progress';

/** An external command-line tool offering one or more commands. */
export class ExternalCli {
  constructor(readonly spec: ExternalCliSpec) {}

  async run<TOptions extends ExecaOptions = {}>(
    file: string,
    args: string[],
    options?: TOptions,
  ): Promise<ExecaResultPromise<{} & TOptions>> {
    const { spec } = this;
    const { platform } = device;
    const cliPath = joinPaths(
      os.tmpdir(),
      'external-clis',
      `${spec.identifier}-${spec.version}-${platform}`,
    );
    if (!fs.existsSync(cliPath)) {
      const url = spec.getDownloadUrl(spec.version);
      if (spec.archiveKind === 'uncompressed') {
        const extension = extname(decodeURIComponent(new URL(url).pathname));
        const targetPath = joinPaths(cliPath, `${spec.identifier}${extension}`);
        await downloadWithProgress(url, targetPath, spec.prettyName);
      } else {
        await downloadAndExtractWithProgress(url, cliPath, {
          description: spec.prettyName,
          archiveKind: spec.archiveKind,
        });
      }
    }

    const binPath = joinPaths(cliPath, spec.binDir ?? '', file);
    if (device.operatingSystem !== 'windows') await chmod(binPath, 0o755); // Make executable
    return execa<TOptions>(binPath, args, { stdio: 'inherit', ...(options ?? {}) } as TOptions);
  }
}

export interface ExternalCliSpec {
  /** The official, pretty name of the tool. */
  prettyName: string;

  /** The kebab-case, identifier-style name of the tool. */
  identifier: string;

  /** The tool version, according to the tool's official versioning. */
  version: string;

  /**
   * The path of the bin directory, relative to the archive file.
   * Defaults to empty.
   */
  binDir?: string;

  /** The kind of the archive. If not given, it is inferred from the URL. */
  archiveKind?: ArchiveKind | 'uncompressed';

  /** Returns the URL for the portable binary release for the given version and platform. */
  getDownloadUrl: (version: string) => string;
}
