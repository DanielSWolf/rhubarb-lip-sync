#!/usr/bin/python

import simple_test

simple_test.test("test3", ["-f=9", "-f=1.0.0", "-s=asdf", "asdf", "asdf", ], expect_fail=True)
