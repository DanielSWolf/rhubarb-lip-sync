#!/usr/bin/python

import simple_test

simple_test.test("test5", ["--aaa", "dilbert", "-b", "asdf", "-c", "fdas", ], expect_fail=True)
