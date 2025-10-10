#!/usr/bin/python

import simple_test

simple_test.test("test9", ["-VVV", "-N", "--noise", "-rr", ], expect_fail=True)
