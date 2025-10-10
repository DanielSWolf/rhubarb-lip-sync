#!/usr/bin/python

import simple_test

simple_test.test("test2", ["-i", "2.1", "-f", "4.2", "-s", "asdf", "asdf", ], expect_fail=True)
