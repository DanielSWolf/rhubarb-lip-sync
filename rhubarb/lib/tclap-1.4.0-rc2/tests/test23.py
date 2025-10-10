#!/usr/bin/python

import simple_test

simple_test.test("test5", ["-d", "junk", "-c", "fdas", ], expect_fail=True)
