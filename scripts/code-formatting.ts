import { ExternalCli, device, throwError } from './util';
import { glob } from 'glob';
import { execa } from 'execa';
import { repoDir } from './repo-dir';

const customDependencies = require('../package.json').customDependencies as Record<string, string>;

type Action = 'check' | 'fix';

const allRunners = [runClangFormat, runPrettier, runGersemi];

/** Checks whether all code in the repo is formatted; throws otherwise. */
export async function checkCodeFormatting() {
  let failed = false;
  await Promise.all(
    allRunners
      .map(runner => runner('check'))
      .map(promise =>
        promise.catch(error => {
          console.error(error.message);
          failed = true;
        }),
      ),
  );
  if (failed)
    throw new Error('Some files are not properly formatted. Run `npm run fix` to fix them.');
}

/** Formats all code in the repo. */
export async function fixCodeFormatting() {
  await Promise.all(allRunners.map(runner => runner('fix')));
}

/** Runs ClangFormat, a C/C++/... code formatter. */
async function runClangFormat(action: Action) {
  const files = await glob('rhubarb/{src,tests}/**/*.{c,h,cpp,hpp}', { cwd: repoDir, nodir: true });
  await clangFormat.run(
    'clang-format',
    [action === 'check' ? '--dry-run' : '-i', '--Werror', ...files],
    { cwd: repoDir },
  );
}

/** Runs Prettier, a JavaScript/TypeScript/... code formatter. */
async function runPrettier(action: Action) {
  await execa(
    'node',
    ['node_modules/prettier/bin/prettier.cjs', action === 'check' ? '--check' : '--write', '.'],
    {
      cwd: repoDir,
      stdio: 'inherit',
    },
  );
}

/** Runs gersemi, a CMake code formatter. */
async function runGersemi(action: Action) {
  const files = await glob('{,rhubarb/,extras/**/}{CMakeLists.txt,*.cmake}', {
    cwd: repoDir,
    nodir: true,
  });
  await uv.run(
    'uvx',
    [
      `gersemi@${customDependencies.gersemi}`,
      action === 'check' ? '--check' : '--in-place',
      ...files,
    ],
    { cwd: repoDir },
  );
}

const clangFormat = new ExternalCli({
  prettyName: 'ClangFormat',
  identifier: 'clang-format',
  version: customDependencies.clangFormat,
  getDownloadUrl: version => {
    const [clangVersion, release] = version.split('@');
    const basename =
      {
        'windows-amd64': `clang-format-${clangVersion}_windows-amd64.exe`,
        'windows-arm64': null,
        'macos-amd64': `clang-format-${clangVersion}_macosx-amd64`,
        'macos-arm64': `clang-format-${clangVersion}_macos-arm-arm64`,
        'linux-amd64': `clang-format-${clangVersion}_linux-amd64`,
        'linux-arm64': null,
      }[device.platform] ??
      throwError(`No clang-format binary found for platform ${device.platform}.`);
    const releasesUrl = 'https://github.com/muttleyxd/clang-tools-static-binaries/releases';
    return `${releasesUrl}/download/${release}/${basename}`;
  },
  archiveKind: 'uncompressed',
});

const uv = new ExternalCli({
  prettyName: 'uv Python package manager',
  identifier: 'uv',
  version: customDependencies.uv,
  getDownloadUrl: version => {
    const basename = {
      'windows-amd64': 'uv-x86_64-pc-windows-msvc.zip',
      'windows-arm64': 'uv-aarch64-pc-windows-msvc.zip',
      'macos-amd64': 'uv-x86_64-apple-darwin.tar.gz',
      'macos-arm64': 'uv-aarch64-apple-darwin.tar.gz',
      'linux-amd64': 'uv-x86_64-unknown-linux-gnu.tar.gz',
      'linux-arm64': 'uv-aarch64-unknown-linux-gnu.tar.gz',
    }[device.platform];
    const releasesUrl = 'https://github.com/astral-sh/uv/releases';
    return `${releasesUrl}/download/${version}/${basename}`;
  },
});
