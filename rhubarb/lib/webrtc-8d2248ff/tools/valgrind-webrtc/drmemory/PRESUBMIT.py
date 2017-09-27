#!/usr/bin/env python
# Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
#
# Use of this source code is governed by a BSD-style license
# that can be found in the LICENSE file in the root of the source
# tree. An additional intellectual property rights grant can be found
# in the file PATENTS.  All contributing project authors may
# be found in the AUTHORS file in the root of the source tree.

"""
Copied from Chrome's src/tools/valgrind/drmemory/PRESUBMIT.py

See http://dev.chromium.org/developers/how-tos/depottools/presubmit-scripts
for more details on the presubmit API built into gcl.
"""

import os


def CheckChange(input_api, output_api):
  """Checks the DrMemory suppression files for bad suppressions."""

  # Add the path to the Chrome valgrind dir to the import path:
  tools_vg_path = os.path.join(input_api.PresubmitLocalPath(), '..', '..',
                               'valgrind')
  import sys
  old_path = sys.path
  try:
    sys.path = sys.path + [tools_vg_path]
    import suppressions
    return suppressions.PresubmitCheck(input_api, output_api)
  finally:
    sys.path = old_path


def CheckChangeOnUpload(input_api, output_api):
  return CheckChange(input_api, output_api)


def CheckChangeOnCommit(input_api, output_api):
  return CheckChange(input_api, output_api)


def GetPreferredTrySlaves():
  # We don't have any Dr Memory trybots yet, so there's no use for this method.
  # When we have, the slave name(s) should be put into this list.
  return []
