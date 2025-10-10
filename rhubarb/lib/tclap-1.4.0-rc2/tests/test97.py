#!/usr/bin/python

import simple_test

simple_test.test("test32", ["-b", "5", "-s", "foo", ], expect_fail=True)
