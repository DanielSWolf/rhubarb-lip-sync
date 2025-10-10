#!/usr/bin/python

import simple_test

simple_test.test("test12", ["-v", "a 1 0.3", ], expect_fail=True)
