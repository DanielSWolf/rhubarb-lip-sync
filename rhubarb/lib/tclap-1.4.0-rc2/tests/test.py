#!/usr/bin/python3

import glob
import os
import subprocess
import sys
from typing import Sequence

def test(tests: Sequence[str], expected_fail: Sequence[str]) -> None:
    expected_fail = frozenset(t.split()[0].strip() for t in expected_fail
                              if len(t.split()) > 0)
    passing = 0
    failing = 0
    for t in tests:
        r = subprocess.run(['./' + t],
                           stdout=subprocess.DEVNULL,
                           stderr=subprocess.DEVNULL)
        if r.returncode == 0:
            passing += 1
            if t in expected_fail:
                print('Unexpected PASS: ' + t)
        else:
            failing += 1
            if t not in expected_fail:
                print('Unexpected FAIL: ' + t)
    print('PASS: %d / FAIL: %d' % (passing, failing))

def build():
    cpu_count = os.cpu_count() or 1
    subprocess.run(['make', '-C', '../examples', '-j', str(cpu_count)])

def main():
    script_dir = os.path.dirname(sys.argv[0])
    os.chdir(script_dir or '.')
    build()
    with open('expected-failures.txt') as expected_fail:
        test(glob.glob('test[0-9]*.py'), expected_fail.readlines())

if __name__ == '__main__':
    main()
