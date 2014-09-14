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


PRIMITIVE_TYPES = set([
  TypeKind.VOID.value, TypeKind.BOOL.value, TypeKind.CHAR_U.value,
  TypeKind.UCHAR.value, TypeKind.CHAR16.value, TypeKind.CHAR32.value,
  TypeKind.USHORT.value, TypeKind.UINT.value, TypeKind.ULONG.value,
  TypeKind.ULONGLONG.value, TypeKind.UINT128.value, TypeKind.CHAR_S.value,
  TypeKind.SCHAR.value, TypeKind.WCHAR.value, TypeKind.SHORT.value,
  TypeKind.INT.value, TypeKind.LONG.value, TypeKind.LONGLONG.value,
  TypeKind.INT128.value, TypeKind.FLOAT.value, TypeKind.DOUBLE.value,
  TypeKind.LONGDOUBLE.value,
])

PRIMITIVE_TYPE_KINDS_NAME = {
  TypeKind.VOID.value: 'void',
  TypeKind.BOOL.value: 'bool',
  TypeKind.CHAR_U.value: 'char',
  TypeKind.UCHAR.value: 'uchar',
  TypeKind.CHAR16.value: 'char16',
  TypeKind.CHAR32.value: 'char32',
  TypeKind.USHORT.value: 'ushort',
  TypeKind.UINT.value: 'uint',
  TypeKind.ULONG.value: 'ulong',
  TypeKind.ULONGLONG.value: 'ulonglong',
  TypeKind.UINT128.value: 'uint128',
  TypeKind.CHAR_S.value: 'char',
  TypeKind.SCHAR.value: 'schar',
  TypeKind.WCHAR.value: 'wchar',
  TypeKind.SHORT.value: 'short',
  TypeKind.INT.value: 'int',
  TypeKind.LONG.value: 'long',
  TypeKind.LONGLONG.value: 'longlong',
  TypeKind.INT128.value: 'int128',
  TypeKind.FLOAT.value: 'float',
  TypeKind.DOUBLE.value: 'double',
  TypeKind.LONGDOUBLE.value: 'longdouble',
}

PRIMITIVE_TYPE_KINDS_KIND = {
  TypeKind.VOID.value: 'VOID',
  TypeKind.BOOL.value: 'BOOL',
  TypeKind.CHAR_U.value: 'CHAR_U',
  TypeKind.UCHAR.value: 'UCHAR',
  TypeKind.CHAR16.value: 'CHAR16',
  TypeKind.CHAR32.value: 'CHAR32',
  TypeKind.USHORT.value: 'USHORT',
  TypeKind.UINT.value: 'UINT',
  TypeKind.ULONG.value: 'ULONG',
  TypeKind.ULONGLONG.value: 'ULONGLONG',
  TypeKind.UINT128.value: 'UINT128',
  TypeKind.CHAR_S.value: 'CHAR_S',
  TypeKind.SCHAR.value: 'SCHAR',
  TypeKind.WCHAR.value: 'WCHAR',
  TypeKind.SHORT.value: 'SHORT',
  TypeKind.INT.value: 'INT',
  TypeKind.LONG.value: 'LONG',
  TypeKind.LONGLONG.value: 'LONGLONG',
  TypeKind.INT128.value: 'INT128',
  TypeKind.FLOAT.value: 'FLOAT',
  TypeKind.DOUBLE.value: 'DOUBLE',
  TypeKind.LONGDOUBLE.value: 'LONGDOUBLE',
}


def IsQualified(t):
  return (t.is_const_qualified() or t.is_volatile_qualified() or
          t.is_restrict_qualified())

def GetJsQualifiedArg(t):
  a = []
  if t.is_const_qualified():
    a.append('Type.CONST')
  if t.is_volatile_qualified():
    a.append('Type.VOLATILE')
  if t.is_restrict_qualified():
    a.append('Type.RESTRICT')
  return '|'.join(a)

def GetJsQualifiedArgWithComma(t):
  q = GetJsQualifiedArg(t)
  if q:
    return ', ' + q
  return q

anonymous_names = {}

VALID_SPELLING_PARTS = ('struct', 'union', 'enum', 'const', 'volatile',
                        'restrict')
KIND_TO_ANON = {
  TypeKind.ENUM.value: 'enum',
  TypeKind.RECORD.value: 'record'
}
def SpellingBaseName(t):
  if t.get_declaration().spelling == '':
    if t.spelling in anonymous_names:
      return anonymous_names[t.spelling]

    count = len(anonymous_names)
    name = '__anon_%s_%d' % (KIND_TO_ANON[t.kind.value], count)
    anonymous_names[t.spelling] = name
    return name

  spelling = t.spelling

  parts = spelling.split(' ')
  if not all(p in VALID_SPELLING_PARTS for p in parts[:-1]):
    print 'Spelling: %r, Bad parts: %r' % (
        spelling,
        ' '.join(set(parts) - set(VALID_SPELLING_PARTS)))
    assert False
  return parts[-1]


def GetJsInline(t):
  if t.kind.value in PRIMITIVE_TYPES:
    if IsQualified(t):
      return 'Type.Numeric(Type.%s, %s)' % (
          PRIMITIVE_TYPE_KINDS_KIND[t.kind.value], GetJsQualifiedArg(t))
    else:
      return 'Type.%s' % PRIMITIVE_TYPE_KINDS_NAME[t.kind.value]

  value = TYPE_KINDS_JS_INLINE.get(t.kind.value, None)
  if not value:
    raise Error('Unsupported type: %s: %r' % (t.kind, t.spelling))

  if type(value) is str:
    return value
  return value(t)

def GetJsInline_Pointer(t):
  return 'Type.Pointer(%s%s)' % (GetJsInline(t.get_pointee()),
                                 GetJsQualifiedArgWithComma(t))

def GetJsInline_Record(t):
  return 'Tags.%s' % SpellingBaseName(t)

def GetJsInline_FunctionProto(t):
  return 'Type.Function(%s, [%s])' % (
      GetJsInline(t.get_result()),
      ', '.join(GetJsInline(a) for a in t.argument_types()))

def GetJsInline_Typedef(t):
  return 'Types.%s' % SpellingBaseName(t)

def GetJsInline_Enum(t):
  return 'Types.%s' % SpellingBaseName(t)

def GetJsInline_Unexposed(t):
  can = t.get_canonical()
  if can.kind != TypeKind.UNEXPOSED:
    return GetJsInline(can)

  return 'Types.%s' % SpellingBaseName(t)

def GetJsInline_ConstantArray(t):
  return 'Type.Array(%s, %d%s)' % (
      GetJsInline(t.get_array_element_type()), t.get_array_size(),
      GetJsQualifiedArgWithComma(t))

def GetJsInline_IncompleteArray(t):
  return 'Type.Array(%s%s)' % (GetJsInline(t.get_array_element_type()),
                               GetJsQualifiedArgWithComma(t))

TYPE_KINDS_JS_INLINE = {
  TypeKind.POINTER.value: GetJsInline_Pointer,
  TypeKind.RECORD.value: GetJsInline_Record,
  TypeKind.FUNCTIONPROTO.value: GetJsInline_FunctionProto,
  TypeKind.TYPEDEF.value: GetJsInline_Typedef,
  TypeKind.ENUM.value: GetJsInline_Enum,
  TypeKind.UNEXPOSED.value: GetJsInline_Unexposed,
  TypeKind.CONSTANTARRAY.value: GetJsInline_ConstantArray,
  TypeKind.INCOMPLETEARRAY.value: GetJsInline_IncompleteArray,
}


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


def Mangle(t, canonical=True):
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
    if canonical:
      ret += Mangle(t.get_canonical())
    else:
      ret += MangleName(SpellingBaseName(t))
  elif t.kind in (TypeKind.RECORD, TypeKind.ENUM):
    ret += MangleName(SpellingBaseName(t))
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
    if canonical or t.get_canonical().kind != TypeKind.UNEXPOSED:
      ret += Mangle(t.get_canonical())
    else:
      ret += MangleName(SpellingBaseName(t))
  else:
    print t.kind, t.spelling
    import pdb; pdb.set_trace()
    assert False
  return ret


class Type(object):
  def __init__(self, t):
    self.type = t
    self.js_inline = GetJsInline(t)
    self.js_mangle = Mangle(t, canonical=False)

  def IsPrimitive(self):
    return self.type.kind.value in PRIMITIVE_TYPES

  def IsTagType(self):
    return self.type.kind.value in (TypeKind.RECORD.value, TypeKind.ENUM.value)

  def GetName(self):
    if self.type.kind.value in (TypeKind.TYPEDEF.value, TypeKind.RECORD.value,
                                TypeKind.ENUM.value):
      return SpellingBaseName(self.type)
    raise Error('Don\'t know how to get name of type %s' % self.type.spelling)

  def argument_types(self):
    for t in self.type.argument_types():
      yield Type(t)

  def fields(self):
    for f in self.type.get_declaration().get_children():
      yield f.spelling, Type(f.type), self.type.get_offset(f.spelling) / 8

  def __getattr__(self, name):
    val = getattr(self.type, name)
    if callable(val):
      def wrapper(*args, **kwargs):
        result = val(*args, **kwargs)
        if isinstance(result, clang.cindex.Type):
          return Type(result)
        return result
      return wrapper
    return val

  def __eq__(self, other):
    if type(self) != type(other):
      return False

    t1 = self.type
    t2 = other.type

    if t1.kind != t2.kind:
      return False

    if self.IsPrimitive():
      return True
    return t1 == t2

    # They must be different, but how?
    print '%s:%r %s:%r' % (t1.kind, t1.spelling, t2.kind, t2.spelling)
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
    t = f.type.get_canonical()
    if t not in self.function_types:
      self.function_types[t] = []
    self.function_types[t].append(f)

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
    key = lambda f: Mangle(f, canonical=False)
    for fn_type in sorted(self.function_types.keys(), key=key):
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
  parser.add_option('-t', '--template')
  options, args = parser.parse_args(args)
  if options.verbose:
    logging.getLogger().setLevel(logging.INFO)

  if not options.template:
    parser.error('--template argument required')

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

  with open(options.template) as f:
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
