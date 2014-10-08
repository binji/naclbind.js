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

import easy_template
import logging
import optparse
import os
import platform
import re
import subprocess
import sys

import gen_types

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
ROOT_DIR = os.path.dirname(SCRIPT_DIR)
PYTHON_BINDINGS_DIR = os.path.join(ROOT_DIR, 'third_party', 'clang', 'bindings',
                                   'python')
NACL_SDK_ROOT = os.getenv('NACL_SDK_ROOT')

RENAME_ARGS = {'-triple': '-target'}

FILTER_ARGS = ('-cc1', '-main-file-name', '-v', '-mrelocation-model',
    '-mdisable-fp-elim', '-mconstructor-aliases', '-target-linker-version',
    '-coverage-file', '-nostdsysteminc', '-fdebug-compilation-dir',
    '-ferror-limit', '-fmessage-length', '-emit-llvm-bc', '-fdeprecated-macro',
    '-E')

SEVERITY_MAP = {2: 'warning', 3: 'error', 4: 'fatal'}

class OptionParser(optparse.OptionParser):
  def __init__(self, *args, **kwargs):
    self.ignore_error = False
    if 'ignore_error' in kwargs:
      self.ignore_error = True
      del kwargs['ignore_error']
    optparse.OptionParser.__init__(self, *args, **kwargs)

    self.add_option('-v', '--verbose', action='count')
    self.add_option('-w', '--whitelist-file', action='append', default=[])
    self.add_option('-W', '--whitelist-symbol', action='append', default=[])
    self.add_option('-b', '--blacklist-file', action='append', default=[])
    self.add_option('-B', '--blacklist-symbol', action='append', default=[])
    self.add_option('-t', '--template')
    self.add_option('-m', '--module-name', default='naclbind_gen')
    self.add_option('-o', '--output')
    self.add_option('--max-int-varargs', default=6)
    self.add_option('--max-double-varargs', default=2)

  def error(self, msg):
    if self.ignore_error:
      logging.info('Ignoring option error: %s' % msg)
    else:
      optparse.OptionParser.error(self, msg)


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
# PNACL_LIB = os.path.join(PNACL_ROOT, GetHostDir(), 'lib')
PNACL_LIB = os.path.join(PNACL_ROOT, 'lib')

sys.path.append(PYTHON_BINDINGS_DIR)

import clang.cindex
from clang.cindex import Index, CursorKind, TypeKind, TranslationUnit, Config
from clang.cindex import TokenKind

Config.set_library_path(PNACL_LIB)


class Error(Exception):
  pass


class RunError(Error):
  def __init__(self, msg, stdout, stderr):
    Error.__init__(self, msg)
    self.stdout = stdout
    self.stderr = stderr


def CreateTranslationUnit(args, detailed=False):
  new_args = GetIndexParseArgs(args)
  filename = new_args[-1]
  logging.info('index.parse(None, %r)' % new_args)
  options = (TranslationUnit.PARSE_INCOMPLETE |
             TranslationUnit.PARSE_SKIP_FUNCTION_BODIES)
  if detailed:
    options = TranslationUnit.PARSE_DETAILED_PROCESSING_RECORD
  tu = Index.create().parse(
      None, new_args, options=options)

  PrintTranslationUnitDiagnostics(tu)
  if TranslationUnitHasErrors(tu):
    return None, filename

  return tu, filename


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

  logging.info('Args = %r' % new_args)

  parsed_args = ParseClangArgs(new_args)
  parsed_args = RenameParsedArgs(parsed_args, RENAME_ARGS)
  parsed_args = FilterParsedArgs(parsed_args, FILTER_ARGS)
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


def RenameParsedArgs(args, to_rename):
  result = []
  for arg in args:
    new_name = to_rename.get(arg[0])
    if new_name:
      result.append((new_name, arg[1]))
    else:
      result.append(arg)
  return result


def FilterParsedArgs(args, to_filter_out):
  return filter(lambda a: a[0] not in to_filter_out, args)


def UnparseClangArgs(args):
  result = []
  for arg in args:
    if arg[1] is not None:
      result.extend(arg)
    else:
      result.append(arg[0])
  return result


def ExtendOptionsFromTranslationUnit(tu, options):
  comment_re = re.compile(r'/[/*]*\s*naclbind-gen:\s*(.*)?(?:[*/]*/)?',
                          re.DOTALL)
  for t in tu.cursor.get_tokens():
    if t.kind != TokenKind.COMMENT:
      continue
    m = comment_re.match(t.spelling)
    if not m:
      continue

    text = m.group(1).replace('\r', '').replace('\n', '')
    logging.info('Got naclbind-gen args: %r' % text)
    parser = OptionParser(add_help_option=False, ignore_error=True)
    new_options, _ = parser.parse_args(text.split(' '))

    for key in dir(new_options):
      old_value = getattr(options, key)
      if type(old_value) is not list:
        continue
      new_value = getattr(new_options, key)
      logging.info('Extending %s with %r' % (key, new_value))
      old_value.extend(new_value)


def CollectCursors(root, fn):
  def Helper(cursor):
    if fn(cursor):
      yield cursor
    for child in cursor.get_children():
      for c in Helper(child):
        yield c
  for c in Helper(root):
    yield c


class Acceptor(object):
  def __init__(self, tu_file, wl_files, wl_syms, bl_files, bl_syms, default):
    self.tu_file = tu_file
    self.wl_files = wl_files
    self.wl_syms = wl_syms
    self.bl_files = bl_files
    self.bl_syms = bl_syms
    self.default = default

  def Accept(self, filename, name):
    # Always accept if filename or symbol are in the whitelist.
    for wl_file in self.wl_files:
      if re.match(wl_file, filename):
        return True
    for wl_sym in self.wl_syms:
      if re.match(wl_sym, name):
        return True

    # Always reject if filename or symbol are in the blacklist
    for bl_file in self.bl_files:
      if re.match(bl_file, filename):
        return False
    for bl_sym in self.bl_syms:
      if re.match(bl_sym, name):
        return False

    # Unless otherwise blacklisted above, always include symbols that are
    # specified in the given file. This makes it so you don't have to
    # explicitly whitelist them in case you whitelist symbols from an #include.
    if filename == self.tu_file:
      return True

    # Otherwise, perform default action.
    return self.default


class Collector(object):
  def __init__(self, acceptor, tu):
    self.acceptor = acceptor
    self.tu = tu
    self.types = set()
    self.types_topo = []
    self.functions = []
    self.function_types = {}

  def Collect(self):
    for fn in gen_types.IterFunctions(self.tu.cursor):
      if self.acceptor.Accept(fn.file_name, fn.spelling):
        logging.debug('ACCEPTED %s' % fn.spelling)
        self._VisitFunction(fn)
      else:
        logging.debug('REJECTED %s' % fn.spelling)

  def SortedFunctionTypes(self):
    key = lambda f: f.mangled
    for fn_type in sorted(self.function_types.keys(), key=key):
      yield fn_type, self.function_types[fn_type]

  def _VisitFunction(self, fn):
    fn.VisitTypes(self)

    self.functions.append(fn)
    fn_type = fn.type.canonical
    if fn_type not in self.function_types:
      self.function_types[fn_type] = []
    self.function_types[fn_type].append(fn)

  def EnterType(self, t):
    t = t.Unqualified()
    if t in self.types:
      return False
    self.types.add(t)
    return True

  def ExitType(self, t):
    self.types_topo.append(t.Unqualified())


def StripCopyright(text):
  # Assume that the first C-style comment is the copyright
  m = re.match(r'/\*.*?\*/[\r\n]*(.*)', text, re.DOTALL)
  if not m:
    return text

  return m.group(1)


def StripRegions(text):
  start_re = re.compile(r'^/\*+ STRIP_START \*+/$', re.MULTILINE)
  end_re = re.compile(r'^/\*+ STRIP_END \*+/$', re.MULTILINE)
  start_pos = 0
  result = ''
  while True:
    sm = start_re.search(text, start_pos)
    if not sm:
      break
    em = end_re.search(text, sm.end())
    if not em:
      raise Error('Found start marker but not end.')

    result += text[start_pos:sm.start()]
    start_pos = em.end()

  result += text[start_pos:]
  return result


def IncludeFile(name):
  # Search in src
  path = os.path.join(ROOT_DIR, 'src', name)
  rel_path = os.path.relpath(path, ROOT_DIR)

  try:
    f = open(path)
  except IOError as e:
    raise Error('Failed to open file: %s: %s' % (rel_path, e))

  try:
    # Strip the copyright, it's already included at the top of the template.
    text = StripCopyright(f.read())
    text = StripRegions(text)
    return """\
/* +++ INCLUDE %(path)s +++ */
%(text)s
/* --- INCLUDE %(path)s --- */""" % {'path': rel_path, 'text': text}
  finally:
    f.close()


def main(args):
  logging.basicConfig(format='%(levelname)s: %(message)s')
  parser = OptionParser()
  options, args = parser.parse_args(args)
  if options.verbose >= 2:
    logging.getLogger().setLevel(logging.DEBUG)
  elif options.verbose >= 1:
    logging.getLogger().setLevel(logging.INFO)

  if not options.template:
    parser.error('--template argument required')

  tu, filename = CreateTranslationUnit(args)
  if not tu:
    return 1

  ExtendOptionsFromTranslationUnit(tu, options)

  # By default, accept everything. If there is an explicit whitelist, default
  # to accepting nothing.
  if options.whitelist_file or options.whitelist_symbol:
    accept_default = False
  else:
    accept_default = True

  acceptor = Acceptor(filename,
                      options.whitelist_file, options.whitelist_symbol,
                      options.blacklist_file, options.blacklist_symbol,
                      accept_default)

  collector = Collector(acceptor, tu)
  collector.Collect()

  with open(options.template) as f:
    template = f.read()

  # See http://stackoverflow.com/a/14620633
  class AttrDict(dict):
    def __init__(self, *args, **kwargs):
      super(AttrDict, self).__init__(*args, **kwargs)
      self.__dict__ = self

  template_dict = AttrDict()
  template_dict.TypeKind = gen_types.TypeKind
  template_dict.collector = collector
  template_dict.filename = filename
  template_dict.module_name = options.module_name
  template_dict.IncludeFile = IncludeFile
  template_dict.Error = Error
  template_dict.MAX_INT_VARARGS = options.max_int_varargs
  template_dict.MAX_DBL_VARARGS = options.max_double_varargs

  out_text = easy_template.RunTemplateString(template, template_dict)

  if options.output:
    outdir = os.path.abspath(os.path.dirname(options.output))
    if not os.path.exists(outdir):
      os.makedirs(outdir)
    with open(options.output, 'w') as outf:
      outf.write(out_text)
  else:
    sys.stdout.write(out_text)

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
