#!/usr/bin/python

import simple_test

simple_test.test("test8", ["-s", "bbb", "homer", "marge", "bart", "--", "-hv", "two", ], expect_fail=True)
