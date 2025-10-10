#!/usr/bin/python

import simple_test

simple_test.test("test5", ["--aaa", "asdf", "-c", "fdas", "--fff", "blah", "-i", "one", "-i", "two", "-j", "huh", ], expect_fail=True)
