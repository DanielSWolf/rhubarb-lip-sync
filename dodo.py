"""Collection of tasks. Run using `doit <task>`."""

from pathlib import Path
import subprocess
from functools import cache
from gitignore_parser import parse_gitignore
from typing import Dict, Optional, List
from enum import Enum


root_dir = Path(__file__).parent
rhubarb_dir = root_dir / 'rhubarb'


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
            subprocess.run(
                ['clang-format', '--dry-run' if check_only else '-i', '--Werror', *files],
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


@cache
def get_files_by_formatters() -> Dict[Formatter, List[Path]]:
    """Returns a dict with all formattable code files grouped by formatter."""

    is_gitignored = parse_gitignore(root_dir / '.gitignore')

    def is_hidden(path: Path):
        return path.name.startswith('.')

    def is_third_party(path: Path):
        return path.is_relative_to(rhubarb_dir / 'lib') or path.name == 'gradle'

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
