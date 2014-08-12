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


def CreateTranslationUnit(args, detailed=False):
  new_args = GetIndexParseArgs(args)
  logging.info('index.parse(None, %r)' % new_args)
  options = (TranslationUnit.PARSE_INCOMPLETE |
             TranslationUnit.PARSE_SKIP_FUNCTION_BODIES)
  if detailed:
    options = TranslationUnit.PARSE_DETAILED_PROCESSING_RECORD
  tu = Index.create().parse(
      None, new_args, options=options)

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
  def __init__(self, wl_files, wl_syms, bl_files, bl_syms, default):
    self.wl_files = wl_files
    self.wl_syms = wl_syms
    self.bl_files = bl_files
    self.bl_syms = bl_syms
    self.default = default

  def Accept(self, filename, name):
    # Always accept if filename or symbol are in the whitelist.
    for wl_file in self.wl_files:
      if re.search(wl_file, filename):
        return True
    for wl_sym in self.wl_syms:
      if re.search(wl_sym, name):
        return True

    # Always reject if filename or symbol are in the blacklist
    for bl_file in self.bl_files:
      if re.search(bl_file, filename):
        return False
    for bl_sym in self.bl_syms:
      if re.search(bl_sym, name):
        return False

    # Otherwise, perform default action.
    return self.default


PRIMITIVE_TYPE_KINDS_MANGLE = {
  TypeKind.VOID.value: 'v',
  TypeKind.BOOL.value: 'b',
  TypeKind.CHAR_U.value: 'c',
  TypeKind.UCHAR.value: 'h',
  TypeKind.CHAR16.value: 'Ds',
  TypeKind.CHAR32.value: 'Di',
  TypeKind.USHORT.value: 't',
  TypeKind.UINT.value: 'j',
  TypeKind.ULONG.value: 'm',
  TypeKind.ULONGLONG.value: 'y',
  TypeKind.UINT128.value: 'o',
  TypeKind.CHAR_S.value: 'c',
  TypeKind.SCHAR.value: 'a',
  TypeKind.WCHAR.value: 'w',
  TypeKind.SHORT.value: 's',
  TypeKind.INT.value: 'i',
  TypeKind.LONG.value: 'l',
  TypeKind.LONGLONG.value: 'x',
  TypeKind.INT128.value: 'n',
  TypeKind.FLOAT.value: 'f',
  TypeKind.DOUBLE.value: 'd',
  TypeKind.LONGDOUBLE.value: 'e',
}


def MangleName(s):
  return '%d%s' % (len(s), s)


def Mangle(t):
  ret = ''
  if t.is_restrict_qualified():
    ret += 'r'
  if t.is_volatile_qualified():
    ret += 'v'
  if t.is_const_qualified():
    ret += 'K'

  if t.kind.value in PRIMITIVE_TYPE_KINDS_MANGLE:
    ret += PRIMITIVE_TYPE_KINDS_MANGLE[t.kind.value]
  elif t.kind == TypeKind.POINTER:
    ret += 'P' + Mangle(t.get_pointee())
  elif t.kind == TypeKind.TYPEDEF:
    ret += Mangle(t.get_canonical())
  elif t.kind in (TypeKind.RECORD, TypeKind.ENUM):
    # Find last space, everything else should be
    # const/volatile/restrict/struct/enum.
    s = t.spelling.split(' ')[-1]
    ret += MangleName(s)
  elif t.kind == TypeKind.FUNCTIONPROTO:
    ret += 'F'
    ret += Mangle(t.get_result())
    args = list(t.argument_types())
    if len(args) == 0:
      ret += 'v'
    else:
      ret += ''.join(Mangle(a) for a in t.argument_types())
    if t.is_function_variadic():
      ret += 'z'
    ret += 'E'
  elif t.kind == TypeKind.INCOMPLETEARRAY:
    ret += 'P%s' % Mangle(t.get_array_element_type())
  elif t.kind == TypeKind.CONSTANTARRAY:
    ret += 'A%d_%s' % (t.get_array_size(), Mangle(t.get_array_element_type()))
  elif t.kind == TypeKind.UNEXPOSED:
    ret += Mangle(t.get_canonical())
  else:
    print t.kind, t.spelling
    import pdb; pdb.set_trace()
    assert False
  return ret


class Type(object):
  def __init__(self, t):
    self.type = t
    # Generate C typedefs for function prototypes.
    self.c_typedef = None
    if t.kind == TypeKind.FUNCTIONPROTO:
      self.c_typedef = 'TYPE_%s' % (Mangle(t))

  def __getattr__(self, name):
    return getattr(self.type, name)

  def __eq__(self, other):
    if type(self) != type(other):
      return False

    t1 = self.type
    t2 = other.type

    if t1 == t2:
      return True

    # The types still may be equal as far as we're concerned, even if clang
    # doesn't think so.
    if t1.spelling != t2.spelling:
      return False

    t1typedef = t1.kind == TypeKind.TYPEDEF
    t2typedef = t2.kind == TypeKind.TYPEDEF
    t1can = t1.get_canonical()
    t2can = t2.get_canonical()

    # It may be that one is an anonymous struct/enum and the other is a typedef
    # of it. It seems that libclang gives them the same spelling in this case.
    # We want to distinguish these two types, so in this case return not equal.
    if (t1typedef and t1can == t2) or (t2typedef and t2can == t1):
      return False

    # If the canonical types are the same, and neither are typedefs then we'll
    # consider the types identical.
    if not t1typedef and not t2typedef and t1can == t2can:
      return True

    # They must be different, but how?
    assert False
    return False

  def __ne__(self, other):
    return not self.__eq__(other)

  def __hash__(self):
    return hash(self.type.spelling)


class Function(object):
  def __init__(self, fn):
    self.fn = fn
    self.type = Type(fn.type)

  def __getattr__(self, name):
    return getattr(self.fn, name)

  def __eq__(self, other):
    if type(self) != type(other):
      return False

    return self.fn.spelling == other.fn.spelling

  def __ne__(self, other):
    return not self.__eq__(other)

  def __hash__(self):
    return hash(self.fn.spelling)


class Collector(object):
  def __init__(self, acceptor, tu):
    self.acceptor = acceptor
    self.tu = tu
    self.types = set()
    self.types_topo = []
    self.functions = []
    self.function_types = {}

  def _VisitFunctionDecl(self, f):
    for child in f.get_children():
      if child.kind == CursorKind.PARM_DECL:
        self._VisitType(child.type)

    f = Function(f)
    self.functions.append(f)
    if f.type not in self.function_types:
      self.function_types[f.type] = []
    self.function_types[f.type].append(f)

  def _VisitType(self, t):
    t = Type(t)
    if t in self.types:
      return

    deps = []
    if t.kind == TypeKind.TYPEDEF:
      deps.append(t.get_canonical())
    elif t.kind == TypeKind.POINTER:
      deps.append(t.get_pointee())
    elif t.kind == TypeKind.FUNCTIONPROTO:
      deps.append(t.get_result())
      deps.extend(list(t.argument_types()))
    elif t.kind == TypeKind.RECORD:
      deps.extend(c.type for c in t.get_declaration().get_children())

    self.types.add(t)

    for dep in deps:
      self._VisitType(dep)

    self.types_topo.append(t)

  def Collect(self):
    is_fn_decl = lambda c: c.kind == CursorKind.FUNCTION_DECL
    for fn_decl in CollectCursors(self.tu.cursor, is_fn_decl):
      if self.acceptor.Accept(fn_decl.location.file.name, fn_decl.spelling):
        self._VisitFunctionDecl(fn_decl)

  def SortedFunctionTypes(self):
    for fn_type in sorted(self.function_types.keys(), key=lambda f: f.spelling):
      yield fn_type, self.function_types[fn_type]


def IncludeCFile(name):
  # Search in src/c/.
  try:
    path = os.path.join(ROOT_DIR, 'src', 'c', name)
    rel_path = os.path.relpath(path, ROOT_DIR)
    f = open(path)
  except IOError as e:
    raise Error('Failed to open file: %s: %s' % (rel_path, e))

  try:
    text = f.read()
    return """\
/* +++ INCLUDE %(path)s +++ */
%(text)s
/* --- INCLUDE %(path)s --- */""" % {'path': rel_path, 'text': text}
  finally:
    f.close()


def main(args):
  logging.basicConfig(format='%(levelname)s: %(message)s')
  parser = optparse.OptionParser()
  parser.add_option('-v', '--verbose', action='store_true',
                    help='verbose output')
  parser.add_option('-w', '--whitelist-file', action='append', default=[])
  parser.add_option('-W', '--whitelist-symbol', action='append', default=[])
  parser.add_option('-b', '--blacklist-file', action='append', default=[])
  parser.add_option('-B', '--blacklist-symbol', action='append', default=[])
  options, args = parser.parse_args(args)
  if options.verbose:
    logging.getLogger().setLevel(logging.INFO)

  tu = CreateTranslationUnit(args)
  if not tu:
    return 1

  # By default, accept everything. If there is an explicit whitelist, default
  # to accepting nothing.
  if options.whitelist_file or options.whitelist_symbol:
    accept_default = False
  else:
    accept_default = True

  acceptor = Acceptor(options.whitelist_file, options.whitelist_symbol,
                      options.blacklist_file, options.blacklist_symbol,
                      accept_default)

  collector = Collector(acceptor, tu)
  collector.Collect()

  with open('../templates/glue.c') as f:
    template = f.read()

  # See http://stackoverflow.com/a/14620633
  class AttrDict(dict):
    def __init__(self, *args, **kwargs):
      super(AttrDict, self).__init__(*args, **kwargs)
      self.__dict__ = self

  template_dict = AttrDict()
  template_dict.TypeKind = TypeKind
  template_dict.collector = collector
  print easy_template.RunTemplateString(template, template_dict)

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
