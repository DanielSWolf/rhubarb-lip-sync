#!/usr/bin/python

import simple_test

simple_test.test("test8", ["-s=one", "homer", "-B", ], expect_fail=True)
