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

import copy
import logging
import optparse
import os
import platform
import re
import subprocess
import sys

import easy_template
import gen_types

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
ROOT_DIR = os.path.dirname(SCRIPT_DIR)
PYTHON_BINDINGS_DIR = os.path.join(ROOT_DIR, 'third_party', 'clang', 'bindings',
                                   'python')
BUILTINS_H = os.path.join(ROOT_DIR, 'src', 'c', 'builtins.h')
NACL_SDK_ROOT = os.getenv('NACL_SDK_ROOT')

if not NACL_SDK_ROOT:
  sys.stderr.write(
      'NACL_SDK_ROOT environment variable is not set.\n' +
      'Please set NACL_SDK_ROOT to the location of your Native Client SDK.\n')
  sys.exit(1)

RENAME_ARGS = {'-triple': '-target'}

FILTER_ARGS = ('-cc1', '-main-file-name', '-v', '-mrelocation-model',
    '-mdisable-fp-elim', '-mconstructor-aliases', '-target-linker-version',
    '-coverage-file', '-nostdsysteminc', '-fdebug-compilation-dir',
    '-ferror-limit', '-fmessage-length', '-emit-llvm-bc', '-fdeprecated-macro',
    '-E', '-disable-free', '-fno-math-builtin')

SEVERITY_MAP = {2: 'warning', 3: 'error', 4: 'fatal'}


def ParseRemapOption(option, opt_str, value, parser):
  try:
    sym_from, sym_to = value.split('=')
  except:
    raise optparse.OptionValueError('%s requires format FROM=TO.' % opt_str)

  if sym_from in parser.values.remap:
    raise optparse.OptionValueError('Symbol %r already remapped to %r.' % (
        sym_from, parser.values.remap[sym_from]))

  parser.values.remap[sym_from] = sym_to


class OptionParser(optparse.OptionParser):
  def __init__(self, *args, **kwargs):
    self.ignore_error = False
    if 'ignore_error' in kwargs:
      self.ignore_error = True
      del kwargs['ignore_error']
    optparse.OptionParser.__init__(self, *args, **kwargs)

    self.add_option('-v', '--verbose', action='count')
    self.add_option('-w', '--whitelist-file', metavar='RE', action='append',
                    default=[])
    self.add_option('-W', '--whitelist-symbol', metavar='RE',
                    action='append', default=[])
    self.add_option('-b', '--blacklist-file', metavar='RE', action='append',
                    default=[])
    self.add_option('-B', '--blacklist-symbol', metavar='RE',
                    action='append', default=[])
    self.add_option('-t', '--template', action='append', default=[])
    self.add_option('-m', '--module-name', default='naclbind_gen')
    self.add_option('-o', '--output', metavar='FILE', action='append',
                    default=[])
    self.add_option('-r', '--remap', action='callback', metavar='FROM=TO',
                    callback=ParseRemapOption, type='string', nargs=1,
                    default={})
    self.add_option('--builtins', action='store_true')
    self.add_option('--max-int-varargs', metavar='NUM', type='int', default=6)
    self.add_option('--max-double-varargs', metavar='NUM', type='int',
                    default=2)
    self.add_option('--function-pointer-count', metavar='NUM', type='int',
                    default=4)
    self.add_option('--no-include', action='store_false', dest='include',
                    default=True)

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

  # Look for a line starting with a single space:
  stderr_lines = stderr.splitlines()
  for line in stderr_lines:
    if line[0] == ' ':
      if '-cc1' not in line:
        continue

      # TODO(binji): handle quoted args with spaces
      new_args = line.split()
      if new_args[0] == 'clang:':
        # Clang failure; it couldn't generate the commandline. Output everything
        # except for the verbose stuff.
        e.stderr = '\n'.join(line)
        raise e

      return new_args

  # Couldn't find it, must be an error. Just forward the RunError through.
  raise e


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
  comment_c_re = re.compile(r'/\*\s*naclbind-gen:\s*(.*)\*/', re.DOTALL)
  comment_cpp_re = re.compile(r'//\s*naclbind-gen:\s*(.*)', re.DOTALL)
  for t in tu.cursor.get_tokens():
    if t.kind != TokenKind.COMMENT:
      continue
    m = comment_c_re.match(t.spelling)
    if not m:
      m = comment_cpp_re.match(t.spelling)
      if not m:
        continue

    text = m.group(1).replace('\r', '').replace('\n', '')
    logging.info('Got naclbind-gen args: %r' % text)
    parser = OptionParser(add_help_option=False, ignore_error=True)
    new_options, _ = parser.parse_args(text.split(' '))

    for opt_str in dir(new_options):
      # TODO(binji): maybe this should be opt-in instead of opt-out?
      # Don't allow extending output or templates.
      if opt_str in ('output', 'template'):
        continue

      old_value = getattr(options, opt_str)
      if type(old_value) is list:
        old_list = old_value
        new_list = getattr(new_options, opt_str)
        logging.info('Extending flag %r with %r' % (opt_str, new_list))
        old_list.extend(new_list)
      elif type(old_value) is dict:
        old_dict = old_value
        new_dict = getattr(new_options, opt_str)
        logging.info('Extending flag %r with %r' % (opt_str, new_dict))
        for key in new_dict.iterkeys():
          if key in old_dict:
            logging.warning(
                'While extending flag %r from comments:\n'
                '  Overwriting key %r with %r (old value %r)' % (
                opt_str, key, new_dict[key], old_dict[key]))
          old_dict[key] = new_dict[key]


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
  def __init__(self):
    self.types = set()
    self.types_topo = []
    self.functions = []
    self.functions_remapped = {}
    self.function_types = {}
    self.enums = {}
    self.next_id = 0

  def Collect(self, tu, acceptor, remap):
    for cindex_cursor in gen_types.Iter(tu.cursor):
      file_name = cindex_cursor.location.file.name
      spelling = cindex_cursor.spelling
      if acceptor.Accept(file_name, spelling):
        logging.debug('ACCEPTED %s' % spelling)
        if cindex_cursor.kind == CursorKind.FUNCTION_DECL:
          fn = gen_types.FunctionDecl(cindex_cursor)
          self._VisitFunction(fn, remap)
        elif cindex_cursor.kind == CursorKind.ENUM_DECL:
          enum = gen_types.EnumDecl(cindex_cursor)
          self._VisitEnum(enum)
        else:
          raise Error('Unexpected cursor type: %r' % cindex_cursor.type)
      else:
        logging.debug('REJECTED %s' % spelling)

  def _VisitFunction(self, fn, remap):
    fn.VisitTypes(self)

    fn.fn_id = self.next_id
    self.next_id += 1

    remapped_name = remap.get(fn.spelling, fn.spelling)
    fn_type = fn.type.canonical

    self.functions.append(fn)
    self.functions_remapped.setdefault(remapped_name, []).append(fn)
    self.function_types.setdefault(fn_type, []).append(fn)

  def _VisitEnum(self, enum):
    if enum.spelling in self.enums:
      return
    self.enums[enum.spelling] = enum

  def EnterType(self, t):
    t = t.Unqualified()
    if t in self.types:
      return False
    self.types.add(t)
    return True

  def ExitType(self, t):
    self.types_topo.append(t.Unqualified())

  def SortedFunctionTypes(self):
    key = lambda f: f.mangled
    for fn_type in sorted(self.function_types.keys(), key=key):
      yield fn_type, self.function_types[fn_type]

  def SortedRemappedFunctions(self):
    for fn_name in sorted(self.functions_remapped.keys()):
      yield fn_name, self.functions_remapped[fn_name]

  def SortedEnums(self):
    for name in sorted(self.enums.keys()):
      yield name, self.enums[name]


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


def CollectFromHeader(collector, compile_args, options):
  tu, filename = CreateTranslationUnit(compile_args)
  if not tu:
    raise Error('Creating translation unit failed.')

  options = copy.copy(options)
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

  collector.Collect(tu, acceptor, options.remap)
  return filename


# See http://stackoverflow.com/a/14620633
class AttrDict(dict):
  def __init__(self, *args, **kwargs):
    super(AttrDict, self).__init__(*args, **kwargs)
    self.__dict__ = self


def OutputForTemplate(template, output, collector, tu_filename, options):
  with open(template) as f:
    template = f.read()

  template_dict = AttrDict()
  template_dict.TypeKind = gen_types.TypeKind
  template_dict.collector = collector
  template_dict.filename = tu_filename
  template_dict.module_name = options.module_name
  template_dict.IncludeFile = IncludeFile
  template_dict.Error = Error
  template_dict.builtins = options.builtins
  template_dict.BUILTINS_H = BUILTINS_H
  template_dict.MAX_INT_VARARGS = options.max_int_varargs
  template_dict.MAX_DBL_VARARGS = options.max_double_varargs
  template_dict.FUNCTION_POINTER_COUNT = options.function_pointer_count
  template_dict.INCLUDE_FILES = options.include

  out_text = easy_template.RunTemplateString(template, template_dict)

  if output:
    outdir = os.path.abspath(os.path.dirname(output))
    if not os.path.exists(outdir):
      os.makedirs(outdir)
    with open(output, 'w') as outf:
      outf.write(out_text)
  else:
    sys.stdout.write(out_text)


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

  if len(options.template) != len(options.output):
    # It's OK if there is only one template, and no output. In that case, write
    # to stdout.
    if len(options.template) == 1 and len(options.output) == 0:
      options.output = [None]
    else:
      parser.error('Need to have same number of --template and --output')

  collector = Collector()
  if options.builtins:
    CollectFromHeader(collector, [BUILTINS_H], options)
  filename = CollectFromHeader(collector, args, options)

  for template, output in zip(options.template, options.output):
    OutputForTemplate(template, output, collector, filename, options)

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
