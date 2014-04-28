#!/usr/bin/env python
# Copyright 2014 Ben Smith. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Generate C binding code from an idl file.
"""

import os
import optparse
import sys


class Error(Exception):
  pass


def Log(msg):
  if Log.verbose:
    sys.stderr.write(str(msg) + '\n')
Log.verbose = False


def main(argv):
  usage = 'Usage: %prog [options] <file.json>'
  parser = optparse.OptionParser(usage, description=__doc__)
  parser.add_option('-v', '--verbose', action='store_true',
                    help='Verbose output')

  options, args = parser.parse_args(argv)
  if not args:
    parser.error('No json file specified.')


if __name__ == '__main__':
  try:
    sys.exit(main(sys.argv[1:]))
  except Error as e:
    sys.stderr.write(str(e) + '\n')
    sys.exit(1)
