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

import base64
import optparse
import os
import subprocess
import sys
import urllib2

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
NACL_SDK_DIR = os.path.join(SCRIPT_DIR, '..', '..', 'out', 'nacl_sdk')
FILES = ('cindex.py', 'enumerations.py', '__index__.py')
URL = ('https://chromium.googlesource.com/native_client/pnacl-clang.git/+/' +
    '%s/bindings/python/clang/%s')

NACL_SDK_ROOT = None
NACL_CONFIG = None
PNACL_CLANG = None


def InitializeDirectories(bundle_name):
  global NACL_SDK_ROOT
  global NACL_CONFIG
  global PNACL_CLANG
  NACL_SDK_ROOT = os.path.join(NACL_SDK_DIR, bundle_name)
  NACL_CONFIG = os.path.join(NACL_SDK_ROOT, 'tools', 'nacl_config.py')
  PNACL_CLANG = RunNaClConfig('-t', 'pnacl', '--tool', 'clang')


def RunNaClConfig(*args):
  cmd = [sys.executable, NACL_CONFIG] + list(args)
  p = subprocess.Popen(cmd, stdout=subprocess.PIPE)
  stdout, _ = p.communicate()
  return stdout.strip()


def RunClang(*args):
  cmd = [PNACL_CLANG] + list(args)
  print cmd
  p = subprocess.Popen(cmd, stdout=subprocess.PIPE)
  stdout, _ = p.communicate()
  return stdout


def main(args):
  parser = optparse.OptionParser()
  parser.add_option('-v', '--verbose', help='verbose output')
  parser.add_option('-b', '--bundle', help='nacl_sdk bundle name',
                    default='pepper_canary')
  options, args = parser.parse_args(args)
  InitializeDirectories(options.bundle)
  print RunClang('--version')
  return 0

if __name__ == '__main__':
  sys.exit(main(sys.argv[1:]))
