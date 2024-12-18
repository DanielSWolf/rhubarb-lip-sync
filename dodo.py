"""Collection of tasks. Run using `doit <task>`."""

from pathlib import Path
import subprocess
from functools import cache
from gitignore_parser import parse_gitignore
from typing import Dict, Optional, List
from enum import Enum
from shutil import rmtree, copy, copytree, make_archive
import platform
import tomllib
import re

root_dir = Path(__file__).parent
rhubarb_dir = root_dir / 'rhubarb'
rhubarb_build_dir = rhubarb_dir / 'build'
extras_dir = root_dir / 'extras'


def task_format():
    """Format source files"""

    files_by_formatters = get_files_by_formatters()
    for formatter, files in files_by_formatters.items():
        yield {
            'name': formatter.value,
            'actions': [(format, [files, formatter])],
            'file_dep': files,
        }


def task_check_formatted():
    """Fails unless source files are formatted"""

    files_by_formatters = get_files_by_formatters()
    for formatter, files in files_by_formatters.items():
        yield {
            'basename': 'check-formatted',
            'name': formatter.value,
            'actions': [(format, [files, formatter], {'check_only': True})],
        }


class Formatter(Enum):
    """A source code formatter."""

    CLANG_FORMAT = 'clang-format'
    GERSEMI = 'gersemi'
    PRETTIER = 'prettier'
    RUFF = 'ruff'


def format(files: List[Path], formatter: Formatter, *, check_only: bool = False):
    match formatter:
        case Formatter.CLANG_FORMAT:
            # Pass relative paths to avoid exceeding the maximum command line length
            relative_paths = [file.relative_to(root_dir) for file in files]
            subprocess.run(
                ['clang-format', '--dry-run' if check_only else '-i', '--Werror', *relative_paths],
                cwd=root_dir,
                check=True,
            )
        case Formatter.GERSEMI:
            subprocess.run(['gersemi', '--check' if check_only else '-i', *files], check=True)
        case Formatter.PRETTIER:
            subprocess.run(
                [
                    *['deno', 'run', '-A', 'npm:prettier@3.4.2'],
                    *['--check' if check_only else '--write', '--log-level', 'warn', *files],
                ],
                check=True,
            )
        case Formatter.RUFF:
            subprocess.run(
                ['ruff', '--quiet', 'format', *(['--check'] if check_only else []), *files],
                check=True,
            )
        case _:
            raise ValueError(f'Unknown formatter: {formatter}')


def task_configure_rhubarb():
    """Configure CMake for the Rhubarb binary"""

    def configure_rhubarb():
        ensure_dir(rhubarb_build_dir)
        subprocess.run(['cmake', '..'], cwd=rhubarb_build_dir, check=True)

    return {'basename': 'configure-rhubarb', 'actions': [configure_rhubarb]}


def task_build_rhubarb():
    """Build the Rhubarb binary"""

    def build_rhubarb():
        subprocess.run(
            ['cmake', '--build', '.', '--config', 'Release'], cwd=rhubarb_build_dir, check=True
        )

    return {'basename': 'build-rhubarb', 'actions': [build_rhubarb]}


def task_build_spine():
    """Build Rhubarb for Spine"""

    def build_spine():
        onWindows = platform.system() == 'Windows'
        subprocess.run(
            ['gradlew.bat' if onWindows else './gradlew', 'build'],
            cwd=extras_dir / 'esoteric-software-spine',
            check=True,
            shell=onWindows,
        )

    return {'basename': 'build-spine', 'actions': [build_spine]}


def task_package():
    """Package all artifacts into an archive file"""

    with open(root_dir / 'app-info.toml', 'rb') as file:
        appInfo = tomllib.load(file)

    os_name = 'macOS' if platform.system() == 'Darwin' else platform.system()
    file_name = f"{appInfo['appName'].replace(' ', '-')}-{appInfo['appVersion']}-{os_name}"

    artifacts_dir = ensure_empty_dir(root_dir / 'artifacts')
    tree_dir = ensure_dir(artifacts_dir.joinpath(file_name))

    def collect_artifacts():
        # Misc. files
        asciidoc_to_html(root_dir / 'README.adoc', tree_dir / 'README.html')
        markdown_to_html(root_dir / 'LICENSE.md', tree_dir / 'LICENSE.html')
        markdown_to_html(root_dir / 'CHANGELOG.md', tree_dir / 'CHANGELOG.html')
        copytree(root_dir / 'img', tree_dir / 'img')

        # Rhubarb
        subprocess.run(
            ['cmake', '--install', '.', '--prefix', tree_dir], cwd=rhubarb_build_dir, check=True
        )

        # Adobe After Effects script
        src = extras_dir / 'adobe-after-effects'
        dst_extras_dir = ensure_dir(tree_dir / 'extras')
        dst = ensure_dir(dst_extras_dir / 'adobe-after-effects')
        asciidoc_to_html(src / 'README.adoc', dst / 'README.html')
        copy(src / 'Rhubarb Lip Sync.jsx', dst)

        # Rhubarb for Spine
        src = extras_dir / 'esoteric-software-spine'
        dst = ensure_dir(dst_extras_dir / 'esoteric-software-spine')
        asciidoc_to_html(src / 'README.adoc', dst / 'README.html')
        for file in (src / 'build' / 'libs').iterdir():
            copy(file, dst)

        # Magix Vegas
        src = extras_dir / 'magix-vegas'
        dst = ensure_dir(dst_extras_dir / 'magix-vegas')
        asciidoc_to_html(src / 'README.adoc', dst / 'README.html')
        copy(src / 'Debug Rhubarb.cs', dst)
        copy(src / 'Debug Rhubarb.cs.config', dst)
        copy(src / 'Import Rhubarb.cs', dst)
        copy(src / 'Import Rhubarb.cs.config', dst)

    def pack_artifacts():
        zip_base_name = tree_dir
        format = 'gztar' if platform.system() == 'Linux' else 'zip'
        make_archive(zip_base_name, format, tree_dir)

    return {
        'actions': [collect_artifacts, pack_artifacts],
        'task_dep': ['build-rhubarb', 'build-spine'],
    }


@cache
def get_files_by_formatters() -> Dict[Formatter, List[Path]]:
    """Returns a dict with all formattable code files grouped by formatter."""

    is_gitignored = parse_gitignore(root_dir / '.gitignore')

    def is_hidden(path: Path):
        return path.name.startswith('.')

    def is_third_party(path: Path):
        return path.name == 'lib' or path.name == 'gradle'

    result = {formatter: [] for formatter in Formatter}

    def visit(dir: Path):
        for path in dir.iterdir():
            if is_gitignored(path) or is_hidden(path) or is_third_party(path):
                continue

            if path.is_file():
                formatter = get_formatter(path)
                if formatter is not None:
                    result[formatter].append(path)
            else:
                visit(path)

    visit(root_dir)
    return result


def get_formatter(path: Path) -> Optional[Formatter]:
    """Returns the formatter to use for the given code file, if any."""

    match path.suffix.lower():
        case '.c' | '.cpp' | '.h':
            return Formatter.CLANG_FORMAT
        case '.cmake':
            return Formatter.GERSEMI
        case _ if path.name.lower() == 'cmakelists.txt':
            return Formatter.GERSEMI
        case '.js' | '.jsx' | '.ts':
            return Formatter.PRETTIER
        case '.py':
            return Formatter.RUFF


def asciidoc_to_html(src: Path, dst: Path):
    subprocess.run(
        ['deno', 'run', '-A', 'npm:asciidoctor@3.0.0', '-a', 'toc=left', '-o', dst, src], check=True
    )


def markdown_to_html(src: Path, dst: Path):
    tmp = dst.parent.joinpath(f'{src.stem}-tmp.adoc')
    try:
        markdown_to_asciidoc(src, tmp)
        asciidoc_to_html(tmp, dst)
    finally:
        tmp.unlink()


def markdown_to_asciidoc(src: Path, dst: Path):
    """Cheap best-effort heuristics for converting Markdown to AsciiDoc"""

    markup = src.read_text()

    # Convert headings
    markup = re.sub('^#+', lambda match: '=' * len(match[0]), markup, flags=re.MULTILINE)

    # Convert italics
    markup = re.sub(
        r'(?<![*_])[*_]((?!\s)[^*_\n]+(?<!\s))[*_](?![*_])', lambda match: f'_{match[1]}_', markup
    )

    # Convert boldface
    markup = re.sub(
        r'(?<![*_])[*_]{2}((?!\s)[^*_\n]+(?<!\s))[*_]{2}(?![*_])',
        lambda match: f'*{match[1]}*',
        markup,
    )

    # Convert links
    markup = re.sub(r'\[([^\]]+)\]\(([^)]+)\)', lambda match: f'{match[2]}[{match[1]}]', markup)

    # Convert continuations
    markup = re.sub(r'\n\n\s+', '\n+\n', markup)

    dst.write_text(markup)


def ensure_dir(dir: Path) -> Path:
    """Makes sure the given directory exists."""

    if not dir.exists():
        dir.mkdir()
    return dir


def ensure_empty_dir(dir: Path) -> Path:
    """Makes sure the given directory exists and is empty."""

    if dir.exists():
        rmtree(dir)
    return ensure_dir(dir)
