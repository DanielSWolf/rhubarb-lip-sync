#!/usr/bin/python

import simple_test

simple_test.test("test2", ["-i", "2", "-f", "4.0.2", "-s", "asdf", "asdf", ], expect_fail=True)
