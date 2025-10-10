#!/usr/bin/python

import simple_test

simple_test.test("test3", ["--stringTest", "bbb", "homer", "marge", "bart", "--", "-hv", "two", ], expect_fail=True)
