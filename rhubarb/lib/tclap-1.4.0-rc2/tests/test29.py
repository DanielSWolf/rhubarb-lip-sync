#!/usr/bin/python

import simple_test

simple_test.test("test2", ["-i", "0xA", "-f", "4.2", "-s", "asdf", "asdf", ], expect_fail=True)
