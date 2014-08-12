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

import logging
import optparse
import os
import platform
import re
import subprocess
import sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
ROOT_DIR = os.path.dirname(SCRIPT_DIR)
PYTHON_BINDINGS_DIR = os.path.join(ROOT_DIR, 'third_party', 'clang', 'bindings',
                                   'python')
NACL_SDK_DIR = os.path.join(ROOT_DIR, 'out', 'nacl_sdk')
BUNDLE_NAME = 'pepper_canary'
NACL_SDK_ROOT = os.path.join(NACL_SDK_DIR, BUNDLE_NAME)

FILTER_ARGS = ('-cc1', '-main-file-name', '-v', '-triple', '-mrelocation-model',
    '-mdisable-fp-elim', '-mconstructor-aliases', '-target-linker-version',
    '-coverage-file', '-nostdsysteminc', '-fdebug-compilation-dir',
    '-ferror-limit', '-fmessage-length', '-emit-llvm-bc', '-fdeprecated-macro',
    '-E')

SEVERITY_MAP = {2: 'warning', 3: 'error', 4: 'fatal'}


def GetHostDir():
  if platform.architecture()[0] == '32bit':
    return 'host_x86_32'
  else:
    return 'host_x86_64'

def Run(cmd, env=None):
  cmd_msg = ' '.join(cmd)
  logging.info('Running %s' % cmd_msg)
  p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                       env=env)
  stdout, stderr = p.communicate()
  if p.returncode != 0:
    e = RunError('Command %r failed with error %d' % (cmd_msg, p.returncode),
                 stdout, stderr)
    raise e
  return stdout, stderr

def RunNaClConfig(*args):
  cmd = [sys.executable, NACL_CONFIG] + list(args)
  return Run(cmd)[0].strip()


NACL_CONFIG = os.path.join(NACL_SDK_ROOT, 'tools', 'nacl_config.py')
PNACL_CLANG = RunNaClConfig('-t', 'pnacl', '--tool', 'clang')
PNACL_ROOT = os.path.dirname(os.path.dirname(PNACL_CLANG))
PNACL_LIB = os.path.join(PNACL_ROOT, GetHostDir(), 'lib')

sys.path.append(PYTHON_BINDINGS_DIR)

import clang.cindex
from clang.cindex import Index, CursorKind, TypeKind, TranslationUnit, Config

Config.set_library_path(PNACL_LIB)


class Error(Exception):
  pass


class RunError(Error):
  def __init__(self, msg, stdout, stderr):
    Error.__init__(self, msg)
    self.stdout = stdout
    self.stderr = stderr


def CreateTranslationUnit(args):
  new_args = GetIndexParseArgs(args)
  logging.info('index.parse(None, %r)' % new_args)
  tu = Index.create().parse(
      None, new_args, options=TranslationUnit.PARSE_DETAILED_PROCESSING_RECORD)

  PrintTranslationUnitDiagnostics(tu)
  if TranslationUnitHasErrors(tu):
    return None
  
  return tu


def TranslationUnitHasErrors(tu):
  return any(d.severity >= 3 for d in tu.diagnostics)


def PrintTranslationUnitDiagnostics(tu):
  errors = 0
  for d in tu.diagnostics:
    m = ''
    if d.location.file:
      m += '%s:%d:%d: ' % (d.location.file, d.location.line, d.location.column)
    m += '%s: ' % SEVERITY_MAP.get(d.severity, '???')
    if d.severity >= 3:
      errors += 1

    m += d.spelling
    sys.stderr.write(m + '\n')

  if errors > 0:
    sys.stderr.write('%d error%s generated.\n' % (
        errors, 's' if errors != 1 else ''))


def GetIndexParseArgs(args):
  new_args = RunClangForArgs(args)

  # First arg is the compiler. Remove.
  new_args = new_args[1:]
  # Last arg is always the filename to compile (the code errors out earlier if
  # there are 0 or >2 files).
  filename = new_args[-1]

  # Remove filename arg.
  new_args = new_args[:-1]

  parsed_args = ParseClangArgs(new_args)
  parsed_args = FilterParsedArgs(parsed_args, FILTER_ARGS)
  parsed_args = ExtendParsedClangArgs(parsed_args)
  new_args = UnparseClangArgs(parsed_args)

  # Add the filename back in to pass to Index.parse
  new_args.append(filename)
  return new_args


def RunClangForArgs(args):
  run_args = args[:]
  # Turn on verbosity, preprocess only, and output to /dev/null.
  run_args.extend(('-v', '-E', '-o', '/dev/null'))
  try:
    cmd = [PNACL_CLANG] + run_args
    stdout, stderr = Run(cmd)
    e = RunError('Command %s failed.' % ' '.join(cmd), stdout, stderr)
  except RunError as e:
    stderr = e.stderr

  if 'Unrecognized file type' in stderr:
    # Try again, defaulting to parsing as C
    return RunClangForArgs(['-x', 'c'] + args)

  # TODO(binji): It's cheesy to rely on line 4 being the command line to parse.
  # Find a better way.
  # If there are < 4 lines, the driver failed. Just forward the RunError
  # through.
  stderr_lines = stderr.splitlines()
  if len(stderr_lines) < 4:
    raise e

  # TODO(binji): handle quoted args with spaces
  new_args = stderr_lines[4].split()
  if new_args[0] == 'clang:':
    # Clang failure; it couldn't generate the commandline. Output everything
    # except for the verbose stuff.
    e.stderr = '\n'.join(stderr_lines[4:])
    raise e

  return new_args


def ParseClangArgs(args):
  result = []
  i = 0
  while i < len(args):
    arg = args[i]
    if (arg.startswith('-') and
        i + 1 < len(args) and not args[i + 1].startswith('-')):
      result.append((arg, args[i + 1]))
      i += 2
    else:
      result.append((arg, None))
      i += 1
  return result


def FilterParsedArgs(args, to_filter_out):
  return filter(lambda a: a[0] not in to_filter_out, args)


def ExtendParsedClangArgs(args):
  defines = RunClangForDefines()
  new_args = [('-undef', None)]
  new_args += [('-D%s=%s' % (n, v), None) for n, v in defines]
  new_args += args
  return new_args


def RunClangForDefines():
  cmd = [PNACL_CLANG] + ['-dM', '-E', '-x', 'c', '/dev/null']
  stdout, _ = Run(cmd)
  stdout_lines = stdout.splitlines()
  result = []
  for line in stdout_lines:
    # Lines look like:
    # #define MACRO_NAME SOME MACRO VALUE
    split_line = line.split(None, 2)
    if len(split_line) == 2:
      name, value = split_line[1], None
    else:
      name, value = split_line[1:]

    result.append((name, value))
  return result


def UnparseClangArgs(args):
  result = []
  for arg in args:
    if arg[1] is not None:
      result.extend(arg)
    else:
      result.append(arg[0])
  return result


def main(args):
  logging.basicConfig(format='%(levelname)s: %(message)s')
  parser = optparse.OptionParser()
  parser.add_option('-v', '--verbose', action='store_true',
                    help='verbose output')
  options, args = parser.parse_args(args)
  if options.verbose:
    logging.getLogger().setLevel(logging.INFO)

  tu = CreateTranslationUnit(args)
  if not tu:
    return 1

  return 0

if __name__ == '__main__':
  try:
    sys.exit(main(sys.argv[1:]))
  except RunError as e:
    logging.error(e)
    logging.error('stderr:\n' + e.stderr)
    sys.exit(1)
  except Error as e:
    logging.error(e)
    sys.exit(1)
