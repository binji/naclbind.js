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
import json
import logging
import optparse
import os
import platform
import re
import subprocess
import sys
import urllib2

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
NACL_SDK_DIR = os.path.join(SCRIPT_DIR, '..', '..', 'out', 'nacl_sdk')
URL_ROOT = 'https://chromium.googlesource.com/native_client/pnacl-clang.git/+/%s/'
BLOB_URL = URL_ROOT + '%s?format=TEXT'
TREE_URL = URL_ROOT + '%s?format=JSON'
BINDINGS_PATH = 'bindings/python'
BINDINGS_TESTS_PATH = os.path.join(BINDINGS_PATH, 'tests')
CLANG_VERSION_RE = r'\((https:\S+)\s*(\S+)\)'

NACL_SDK_ROOT = None
NACL_CONFIG = None
PNACL_CLANG = None


class Error(Exception):
  pass


class RunError(Error):
  pass


def InitializeDirectories(bundle_name):
  global NACL_SDK_ROOT
  global NACL_CONFIG
  global PNACL_CLANG
  NACL_SDK_ROOT = os.getenv('NACL_SDK_ROOT',
                            os.path.join(NACL_SDK_DIR, bundle_name))
  NACL_CONFIG = os.path.join(NACL_SDK_ROOT, 'tools', 'nacl_config.py')
  PNACL_CLANG = RunNaClConfig('-t', 'pnacl', '--tool', 'clang')


def Run(cmd, env=None):
  cmd_msg = ' '.join(cmd)
  logging.info('Running %s' % cmd_msg)
  p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                       env=env)
  stdout, stderr = p.communicate()
  if p.returncode != 0:
    e = RunError('Command %r failed with error %d' % (cmd_msg, p.returncode))
    e.stdout = stdout
    e.stderr = stderr
    raise e
  return stdout, stderr


def RunNaClConfig(*args):
  cmd = [sys.executable, NACL_CONFIG] + list(args)
  return Run(cmd)[0].strip()


def RunClang(*args):
  cmd = [PNACL_CLANG] + list(args)
  return Run(cmd)[0]


def GetClangSha():
  version_stdout = RunClang('--version')
  url_shas = re.findall(CLANG_VERSION_RE, version_stdout)
  for url, sha in url_shas:
    if 'clang' in url:
      return sha
  return None


def ReadURL(url):
  logging.info('Downloading %s' % url)
  try:
    f = urllib2.urlopen(url)
    try:
      data = f.read()
    finally:
      f.close()
  except urllib2.URLError as e:
    raise Error('Error downloading %s: %s' % (url, e))
  return data


def DownloadFile(sha, name):
  try:
    data = ReadURL(BLOB_URL % (sha, name))
  except Error:
    # Annoyingly, this can fail if the file has zero size.
    # Let's assume that's what happened here.
    data = ''

  out_path = os.path.join(SCRIPT_DIR, name)
  out_dir = os.path.dirname(out_path)
  try:
    if not os.path.exists(out_dir):
      logging.info('Making directory %s' % out_dir)
      os.makedirs(out_dir)
  except OSError:
    pass
  logging.info('Writing file %s' % out_path)
  with open(out_path, 'w') as f:
    f.write(base64.b64decode(data))


def DownloadFiles(sha, root):
  url = TREE_URL % (sha, root)
  data = ReadURL(url)
  try:
    # The first line is )]}', strip everything until the first {
    brace = data.find('{')
    if brace == -1:
      raise ValueError('Missing "{"')
    data = data[brace:]
    data = json.loads(data)
  except ValueError as e:
    raise Error('Error decoding JSON from %s: %s' % (url, e))

  blobs = []
  trees = []
  entries = data.get('entries', [])
  for entry in entries:
    if entry['type'] == 'blob':
      blobs.append(entry['name'])
    elif entry['type'] == 'tree':
      trees.append(entry['name'])

  for filename in blobs:
    DownloadFile(sha, '%s/%s' % (root, filename))
  for tree in trees:
    DownloadFiles(sha, '%s/%s' % (root, tree))


def RunTests():
  env = dict(os.environ)
  pnacl_dir = os.path.dirname(os.path.dirname(PNACL_CLANG))
  if platform.architecture()[0] == '32bit':
    host_dir = 'host_x86_32'
  else:
    host_dir = 'host_x86_64'

  lib_dir = os.path.join(pnacl_dir, host_dir, 'lib')
  env['LD_LIBRARY_PATH'] = lib_dir
  cmd = ['nosetests', BINDINGS_TESTS_PATH]
  try:
    Run(cmd, env)
  except RunError as e:
    sys.stderr.write('Error running tests:\n' + e.stderr)
    raise


def main(args):
  logging.basicConfig(format='%(levelname)s: %(message)s')
  parser = optparse.OptionParser()
  parser.add_option('-v', '--verbose', action='store_true',
                    help='verbose output')
  parser.add_option('-b', '--bundle', help='nacl_sdk bundle name',
                    default='pepper_canary')
  parser.add_option('--test', action='store_true',
                    help='run python binding tests')
  options, args = parser.parse_args(args)
  if options.verbose:
    logging.getLogger().setLevel(logging.INFO)

  InitializeDirectories(options.bundle)
  sha = GetClangSha()
  DownloadFiles(sha, BINDINGS_PATH)
  if options.test:
    RunTests()
  sys.stderr.write('OK\n')
  return 0

if __name__ == '__main__':
  try:
    sys.exit(main(sys.argv[1:]))
  except Error as e:
    logging.error(e)
    sys.exit(1)
