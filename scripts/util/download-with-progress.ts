import { SingleBar, Presets } from 'cli-progress';
import fs from 'fs-extra';
import * as http from 'http';
import * as https from 'https';
import * as path from 'path';
import { getTempPath } from './download-and-extract-with-progress';

/** Downloads a file to the given path, showing a progress bar. */
export const downloadWithProgress = (
  url: string,
  targetPath: string,
  description: string,
): Promise<void> =>
  new Promise((resolve, reject) => {
    const lib = url.startsWith('https:') ? https : http;
    const req = lib.get(url, res => {
      // Redirects
      if (res.statusCode && res.statusCode >= 300 && res.statusCode < 400 && res.headers.location) {
        res.resume();
        const next = new URL(res.headers.location, url).toString();
        downloadWithProgress(next, targetPath, description).then(resolve, reject);
        return;
      }
      if (res.statusCode !== 200) {
        reject(new Error(`HTTP ${res.statusCode} for ${url}`));
        res.resume();
        return;
      }

      const total = Number(res.headers['content-length'] || 0);
      const tempPath = getTempPath('download');
      const out = fs.createWriteStream(tempPath);

      const bar = new SingleBar(
        {
          format:
            total > 0 ?
              `Downloading ${description}. [{bar}] {percentage}% | {value}/{total} MB`
            : `Downloading ${description}. [{bar}] {value} MB`,
          hideCursor: true,
          barsize: 20,
          formatValue: (value, _, type) =>
            type === 'percentage' && typeof value === 'number' ? value.toFixed(1)
            : type === 'value' || type === 'total' ? (value / 1024 / 1024).toFixed(1)
            : String(value),
        },
        Presets.shades_classic,
      );
      bar.start(total, 0);

      let downloaded = 0;

      res.on('data', (chunk: Buffer) => {
        downloaded += chunk.length;
        bar.update(downloaded);
      });

      res.pipe(out);
      out.on('finish', () => {
        out.close(() => {
          bar.update(total > 0 ? total : downloaded);
          bar.stop();
          fs.ensureDirSync(path.dirname(targetPath));
          fs.moveSync(tempPath, targetPath, { overwrite: true });
          resolve(undefined);
        });
      });
      out.on('error', err => {
        bar.stop();
        reject(err);
      });
    });

    req.on('error', reject);
  });
