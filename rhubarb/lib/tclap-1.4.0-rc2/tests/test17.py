#!/usr/bin/python

import simple_test

simple_test.test("test3", ["--stringTest=one", "homer", "-B", ], expect_fail=True)
