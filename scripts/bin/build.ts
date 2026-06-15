// Builds Rhubarb Lip Sync or part of it, depending on the provided options.

import { join as joinPath, dirname, extname } from 'path';
import lodash from 'lodash';
import { execa } from 'execa';
import { emptyDirSync, ensureDir, ensureDirSync } from 'fs-extra';
import { device, asciidocToHtml, markdownToHtml, throwError } from '../util';
import { glob } from 'glob';
import { copyFile, readFile, writeFile, rm } from 'fs/promises';
import { repoDir } from '../repo-dir';
import { path7z } from '7zip-bin-full';
import { readFileSync } from 'fs';
import { argument, choice, message, object, option, values, withDefault } from '@optique/core';
import { run } from '@optique/run';

const buildDir = joinPath(repoDir, 'cli-build');
const rhubarbBuildDir = joinPath(buildDir, 'rhubarb');
const packageBuildDir = joinPath(buildDir, 'package');

const allCompilers = ['msvc', 'xcode', 'gcc', 'clang'] as const;
type Compiler = (typeof allCompilers)[number];

const allTargets = ['rhubarb', 'spine', 'package'] as const;

const cliParser = object({
  compiler: withDefault(
    option('--compiler', choice(allCompilers, { metavar: 'COMPILER' }), {
      description: message`The compiler to use: ${values(allCompilers)}`,
    }),
    ({ windows: 'msvc', macos: 'xcode', linux: 'gcc' } as const)[device.operatingSystem],
  ),
  test: option('--test', { description: message`Run tests after building.` }),
  target: withDefault(
    argument(choice(allTargets, { metavar: 'TARGET' }), {
      description: message`The target to build: ${values(allTargets)}`,
    }),
    'package' as const,
  ),
});

async function main() {
  const { target, ...buildOptions } = run(cliParser, {
    brief: message`Builds Rhubarb Lip Sync or part of it.`,
    programName: 'npm run build',
    help: 'option',
    showDefault: true,
  });

  ensureDirSync(buildDir);
  const build = { buildRhubarb, buildSpine, buildPackage }[`build${lodash.upperFirst(target)}`];
  await build(buildOptions);
}

interface BuildOptions {
  compiler: Compiler;
  test: boolean;
}

/** Builds the Rhubarb binary. */
async function buildRhubarb({ compiler, test }: BuildOptions) {
  ensureDirSync(rhubarbBuildDir);
  const rhubarbDir = joinPath(repoDir, 'rhubarb');

  // Configure CMake
  const compilerOptions = {
    msvc: ['-G', 'Visual Studio 18 2026', '-A', 'x64'],
    xcode: ['-G', 'Xcode'],
    gcc: [
      ...['-G', 'Unix Makefiles'],
      ...['-D', 'CMAKE_C_COMPILER=gcc'],
      ...['-D', 'CMAKE_CXX_COMPILER=g++'],
    ],
    clang: [
      ...['-G', 'Unix Makefiles'],
      ...['-D', 'CMAKE_C_COMPILER=clang'],
      ...['-D', 'CMAKE_CXX_COMPILER=clang++'],
    ],
  }[compiler];
  await execa(
    'cmake',
    [
      ...compilerOptions,
      ...['-S', rhubarbDir],
      ...['-D', `RHUBARB_ARTIFACT_DIR=${rhubarbBuildDir}`],
    ],
    { cwd: rhubarbBuildDir, stdio: 'inherit' },
  );

  // Build with CMake
  await execa('cmake', ['--build', rhubarbBuildDir, '--config', 'Release', '--parallel'], {
    stdio: 'inherit',
  });

  // Run tests
  if (test) {
    await execa('./runTests', { cwd: rhubarbBuildDir, stdio: 'inherit' });
  }
}

/** Builds Rhubarb for Spine. */
async function buildSpine({ test }: BuildOptions) {
  await execa('./gradlew', ['build', ...(test ? ['test'] : [])], {
    stdio: 'inherit',
    cwd: joinPath(repoDir, 'extras/spine'),
  });
}

/** Builds the Rhubarb release package. */
async function buildPackage(options: BuildOptions) {
  await Promise.all([buildRhubarb(options), buildSpine(options)]);

  emptyDirSync(packageBuildDir);

  // Copy files to package directory

  // ... global files
  await copyAsHtml(joinPath(repoDir, 'CHANGELOG.md'), joinPath(packageBuildDir, 'CHANGELOG.html'));
  await copyAsHtml(joinPath(repoDir, 'LICENSE.md'), joinPath(packageBuildDir, 'LICENSE.html'));
  await copyAsHtml(joinPath(repoDir, 'README.adoc'), joinPath(packageBuildDir, 'README.html'));
  await copy(repoDir, packageBuildDir, 'img/*');
  await copy(rhubarbBuildDir, packageBuildDir, 'rhubarb?(.exe)');

  // ... Rhubarb for After Effects
  await copyAsHtml(
    joinPath(repoDir, 'extras/after-effects/README.adoc'),
    joinPath(packageBuildDir, 'extras/after-effects/README.html'),
  );
  await copy(
    joinPath(repoDir, 'extras/after-effects'),
    joinPath(packageBuildDir, 'extras/after-effects'),
    '{*.jsx,demo/**/*}',
  );

  // ... Rhubarb for Spine
  await copyAsHtml(
    joinPath(repoDir, 'extras/spine/README.adoc'),
    joinPath(packageBuildDir, 'extras/spine/README.html'),
  );
  await copy(
    joinPath(repoDir, 'extras/spine'),
    joinPath(packageBuildDir, 'extras/spine'),
    'demo/**/*',
  );
  await copy(
    joinPath(repoDir, 'extras/spine/build/libs'),
    joinPath(packageBuildDir, 'extras/spine'),
    'rhubarb-for-spine.jar',
  );

  // ... Rhubarb for Vegas Pro
  await copy(
    joinPath(repoDir, 'extras/vegas-pro'),
    joinPath(packageBuildDir, 'extras/vegas-pro'),
    '{*.cs,*.cs.config}',
  );
  await copyAsHtml(
    joinPath(repoDir, 'extras/vegas-pro/README.adoc'),
    joinPath(packageBuildDir, 'extras/vegas-pro/README.html'),
  );

  // Create archive
  const version = getAppVersion();
  const osString = { windows: 'Windows', macos: 'macOS', linux: 'Linux' }[device.operatingSystem];
  const extension = device.operatingSystem === 'linux' ? '.tar.xz' : '.zip';
  const archivePath = joinPath(buildDir, `Rhubarb-Lip-Sync-${version}-${osString}${extension}`);
  await compressDir(packageBuildDir, archivePath);
}

function getAppVersion(): string {
  // Read version from package.json
  const json = JSON.parse(readFileSync(joinPath(repoDir, 'package.json'), 'utf-8'));
  return json.version;
}

async function copy(sourceDir: string, targetDir: string, pattern: string = '**/*') {
  const relativePaths = await glob(pattern, { cwd: sourceDir, nodir: true });
  for (const relativePath of relativePaths) {
    const sourcePath = joinPath(sourceDir, relativePath);
    const targetPath = joinPath(targetDir, relativePath);
    await ensureDir(dirname(targetPath));
    await copyFile(sourcePath, targetPath);
  }
}

async function copyAsHtml(sourcePath: string, targetPath: string) {
  const markup = await readFile(sourcePath, 'utf-8');
  const extension = extname(sourcePath).toLowerCase();
  const html =
    extension === '.md' ? markdownToHtml(markup)
    : extension === '.adoc' ? asciidocToHtml(markup)
    : extension === '.html' ? markup
    : throwError(`Unsupported markup format: ${sourcePath}`);
  await ensureDir(dirname(targetPath));
  await writeFile(targetPath, html);
}

async function compressDir(sourceDir: string, targetArchivePath: string): Promise<void> {
  await rm(targetArchivePath, { force: true });
  if (/\.zip$/i.test(targetArchivePath)) {
    await execa(path7z, ['a', targetArchivePath, `${sourceDir}/*`], {
      stdio: 'inherit',
    });
  } else if (/\.tar\.xz$/i.test(targetArchivePath)) {
    await execa(path7z, ['a', 'dummy.tar', '-so', `${sourceDir}/*`], {
      stderr: 'inherit',
    }).pipe(path7z, ['a', '-sidummy.tar', targetArchivePath], { stderr: 'inherit' });
  } else {
    throw new Error(`Unsupported archive format: ${targetArchivePath}`);
  }
}

main();
