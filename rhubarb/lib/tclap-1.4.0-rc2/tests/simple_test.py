#!/usr/bin/python3

import os
import sys
import subprocess
import difflib
import platform

def isWindows():
    return platform.system() == 'Windows'

def test(target, args, head=None, expect_fail=False):
    test_name = os.path.basename(sys.argv[0])[:-3]
    example_dir = os.path.join('..', 'examples')
    test_bin = os.path.join(example_dir, target)
    if isWindows():
        test_bin += '.exe'
    test = subprocess.Popen([test_bin] + args,
                            stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT,
                            universal_newlines=True)
    (stdout, _) = test.communicate()
    if expect_fail:
        if test.returncode == 0:
            print('FAIL')
            print('Expected non-zero return code, got %d' % test.returncode)
            sys.exit(1)
    elif test.returncode != 0:
        print('FAIL')
        print('Got non-zero return code %d' % test.returncode)
        sys.exit(1)

    got = stdout.split('\n')[:head]
    with open(os.path.join(os.path.dirname(sys.argv[0]), test_name) + '.out') as inp:
        want = inp.read().split('\n')

    if got == want:
        print('OK')
        sys.exit(0)

    diff = difflib.unified_diff(got, want, fromfile='got', tofile='want')
    result = '\n'.join(diff)
    print('FAIL')
    print(result)
    sys.exit(1)

def main():
    (cmd, py3, test) = sys.argv
    script = os.path.join(os.path.dirname(cmd), test + '.py')
    sys.exit(subprocess.call([py3, script]))

if __name__ == '__main__':
    main()
