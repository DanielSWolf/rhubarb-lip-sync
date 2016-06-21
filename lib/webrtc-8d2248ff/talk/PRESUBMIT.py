# libjingle
# Copyright 2013 Google Inc.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
#  1. Redistributions of source code must retain the above copyright notice,
#     this list of conditions and the following disclaimer.
#  2. Redistributions in binary form must reproduce the above copyright notice,
#     this list of conditions and the following disclaimer in the documentation
#     and/or other materials provided with the distribution.
#  3. The name of the author may not be used to endorse or promote products
#     derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
# WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
# EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

def _LicenseHeader(input_api):
  """Returns the license header regexp."""
  # Accept any year number from start of project to the current year
  current_year = int(input_api.time.strftime('%Y'))
  allowed_years = (str(s) for s in reversed(xrange(2004, current_year + 1)))
  years_re = '(' + '|'.join(allowed_years) + ')'
  years_re = '%s(--%s)?' % (years_re, years_re)
  license_header = (
      r'.*? libjingle\n'
      r'.*? Copyright %(year)s Google Inc\..*\n'
      r'.*?\n'
      r'.*? Redistribution and use in source and binary forms, with or without'
        r'\n'
      r'.*? modification, are permitted provided that the following conditions '
        r'are met:\n'
      r'.*?\n'
      r'.*? 1\. Redistributions of source code must retain the above copyright '
        r'notice,\n'
      r'.*?    this list of conditions and the following disclaimer\.\n'
      r'.*? 2\. Redistributions in binary form must reproduce the above '
        r'copyright notice,\n'
      r'.*?    this list of conditions and the following disclaimer in the '
        r'documentation\n'
      r'.*?    and/or other materials provided with the distribution\.\n'
      r'.*? 3\. The name of the author may not be used to endorse or promote '
        r'products\n'
      r'.*?    derived from this software without specific prior written '
        r'permission\.\n'
      r'.*?\n'
      r'.*? THIS SOFTWARE IS PROVIDED BY THE AUTHOR \`\`AS IS\'\' AND ANY '
        r'EXPRESS OR IMPLIED\n'
      r'.*? WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES '
        r'OF\n'
      r'.*? MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE '
        r'DISCLAIMED\. IN NO\n'
      r'.*? EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, '
        r'INCIDENTAL,\n'
      r'.*? SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES \(INCLUDING, '
        r'BUT NOT LIMITED TO,\n'
      r'.*? PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR '
        r'PROFITS;\n'
      r'.*? OR BUSINESS INTERRUPTION\) HOWEVER CAUSED AND ON ANY THEORY OF '
        r'LIABILITY,\n'
      r'.*? WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT \(INCLUDING '
        r'NEGLIGENCE OR\n'
      r'.*? OTHERWISE\) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, '
        r'EVEN IF\n'
      r'.*? ADVISED OF THE POSSIBILITY OF SUCH DAMAGE\.\n'
  ) % {
      'year': years_re,
  }
  return license_header

def _CommonChecks(input_api, output_api):
  """Checks common to both upload and commit."""
  results = []
  results.extend(input_api.canned_checks.CheckLicense(
      input_api, output_api, _LicenseHeader(input_api)))
  return results

def CheckChangeOnUpload(input_api, output_api):
  results = []
  results.extend(_CommonChecks(input_api, output_api))
  return results

def CheckChangeOnCommit(input_api, output_api):
  results = []
  results.extend(_CommonChecks(input_api, output_api))
  return results
