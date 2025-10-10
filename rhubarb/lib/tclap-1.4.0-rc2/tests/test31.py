#!/usr/bin/python

import simple_test

simple_test.test("test3", ["-i=9a", "-i=1", "-s=asdf", "asdf", "asdf", ], expect_fail=True)
