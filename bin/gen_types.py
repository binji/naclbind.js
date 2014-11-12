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

import collections
import copy
import logging
import os
import sys

import gen

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
ROOT_DIR = os.path.dirname(SCRIPT_DIR)
PYTHON_BINDINGS_DIR = os.path.join(ROOT_DIR, 'third_party', 'clang', 'bindings',
                                   'python')

sys.path.append(PYTHON_BINDINGS_DIR)

import clang.cindex
from clang.cindex import CursorKind

CindexTypeKind = clang.cindex.TypeKind


class TypeKind(object):
  def __init__(self, kind_name, js_name, c_name=None):
    self.value = getattr(CindexTypeKind, kind_name).value
    self.kind_name = kind_name
    self.js_name = js_name
    self.c_name = c_name

  def __eq__(self, other):
    if type(other) is int:
      return self.value == other
    if not isinstance(other, TypeKind):
      return False
    return self.value == other.value

  def __ne__(self, other):
    return not self.__eq__(other)

  def __cmp__(self, other):
    if type(other) is int:
      return cmp(self.value, other)
    return cmp(self.value, other.value)

  def __hash__(self):
    return hash(self.value)

  def __str__(self):
    return self.kind_name

  values = {}

  @classmethod
  def FromValue(cls, value):
    if cls.values:
      return cls.values[value]
    for name in dir(cls):
      attr = getattr(cls, name)
      if isinstance(attr, TypeKind):
        cls.values[attr.value] = attr
    return cls.values[value]


TypeKind.VOID = TypeKind('VOID', 'void', 'void')
TypeKind.BOOL = TypeKind('BOOL', 'bool', 'bool')
TypeKind.CHAR_U = TypeKind('CHAR_U', 'char', 'char')
TypeKind.UCHAR = TypeKind('UCHAR', 'uchar', 'unsigned char')
TypeKind.USHORT = TypeKind('USHORT', 'ushort', 'unsigned short')
TypeKind.UINT = TypeKind('UINT', 'uint', 'unsigned int')
TypeKind.ULONG = TypeKind('ULONG', 'ulong', 'unsigned long')
TypeKind.ULONGLONG = TypeKind('ULONGLONG', 'ulonglong', 'unsigned long long')
TypeKind.CHAR_S = TypeKind('CHAR_S', 'char', 'char')
TypeKind.SCHAR = TypeKind('SCHAR', 'schar', 'signed char')
TypeKind.WCHAR = TypeKind('WCHAR', 'wchar', 'wchar_t')
TypeKind.SHORT = TypeKind('SHORT', 'short', 'short')
TypeKind.INT = TypeKind('INT', 'int', 'int')
TypeKind.LONG = TypeKind('LONG', 'long', 'long')
TypeKind.LONGLONG = TypeKind('LONGLONG', 'longlong', 'long long')
TypeKind.FLOAT = TypeKind('FLOAT', 'float', 'float')
TypeKind.DOUBLE = TypeKind('DOUBLE', 'double', 'double')
TypeKind.LONGDOUBLE = TypeKind('LONGDOUBLE', 'longdouble', 'long double')
TypeKind.POINTER = TypeKind('POINTER', 'Pointer')
TypeKind.RECORD = TypeKind('RECORD', 'Record')
TypeKind.ENUM = TypeKind('ENUM', 'Enum')
TypeKind.TYPEDEF = TypeKind('TYPEDEF', 'Typedef')
TypeKind.FUNCTIONNOPROTO = TypeKind('FUNCTIONNOPROTO', 'FunctionNoproto')
TypeKind.FUNCTIONPROTO = TypeKind('FUNCTIONPROTO', 'Function')
TypeKind.CONSTANTARRAY = TypeKind('CONSTANTARRAY', 'Array')
TypeKind.INCOMPLETEARRAY = TypeKind('INCOMPLETEARRAY', 'IncompleteArray')


MANGLED_PRIMITIVE = {
  TypeKind.VOID: 'v',
  TypeKind.BOOL: 'b',
  TypeKind.CHAR_U: 'c',
  TypeKind.UCHAR: 'h',
  TypeKind.USHORT: 't',
  TypeKind.UINT: 'j',
  TypeKind.ULONG: 'm',
  TypeKind.ULONGLONG: 'y',
  TypeKind.CHAR_S: 'c',
  TypeKind.SCHAR: 'a',
  TypeKind.WCHAR: 'w',
  TypeKind.SHORT: 's',
  TypeKind.INT: 'i',
  TypeKind.LONG: 'l',
  TypeKind.LONGLONG: 'x',
  TypeKind.FLOAT: 'f',
  TypeKind.DOUBLE: 'd',
  TypeKind.LONGDOUBLE: 'e',
}

anonymous_names = {}
def _GetAnonymousName(kind, cindex_spelling):
  if cindex_spelling in anonymous_names:
    return anonymous_names[cindex_spelling]

  name = '__anon_%s_%d' % (kind.kind_name.lower(), len(anonymous_names))
  anonymous_names[cindex_spelling] = name
  return name


def _MangleName(name):
  return '%d%s' % (len(name), name)


def _LStripSpelling(spelling, prefixes):
  i = 0
  parts = spelling.split(' ')
  for i, part in enumerate(parts):
    if part not in prefixes:
      break
  return ' '.join(parts[i:])


def _BaseCindexSpelling(spelling):
  PREFIXES = ('struct', 'union', 'enum', 'const', 'volatile', 'restrict')
  return _LStripSpelling(spelling, PREFIXES)


class CindexTypeMemo(object):
  def __init__(self):
    self.memo = {}

  @staticmethod
  def _Key(cindex_type):
    # Given the following C code:
    #     typedef struct { ... } Foo;
    # Cindex "helpfully" names both the anonymous struct and the typedef Foo.
    # We want these types to be distinct, so we always add the type kind as
    # part of the key for the memo.
    return '%s%s' % (cindex_type.kind, CindexTypeMemo._GetSpelling(cindex_type))

  @staticmethod
  def _GetSpelling(cindex_type):
    if cindex_type.kind != CindexTypeKind.FUNCTIONPROTO:
      return cindex_type.spelling

    # Function prototype spellings don't always match what would be generated
    # from the component types, e.g.
    #     cindex_type.spelling => int (char*, const char*, int*)
    #     list(cindex_type.argument_types())[2].spelling => __gnu_va_list
    s = cindex_type.get_result().spelling
    s += ' ('
    arg_types = list(cindex_type.argument_types())
    if arg_types:
      s += ', '.join(a.spelling for a in arg_types)
    else:
      s += 'void'
    if cindex_type.is_function_variadic():
      s += ', ...'
    s += ')'
    return s

  def __contains__(self, cindex_type):
    assert isinstance(cindex_type, clang.cindex.Type)
    key = CindexTypeMemo._Key(cindex_type) 
    return key in self.memo

  def __getitem__(self, cindex_type):
    assert isinstance(cindex_type, clang.cindex.Type)
    key = CindexTypeMemo._Key(cindex_type) 
    return self.memo[key]

  def __setitem__(self, cindex_type, t):
    assert isinstance(cindex_type, clang.cindex.Type)
    key = CindexTypeMemo._Key(cindex_type) 
    self.memo[key] = t

  def Set(self, cindex_type, t):
    self.__setitem__(cindex_type, t)
    return t


class Qualifiers(object):
  def __init__(self, const, volatile, restrict):
    self.const = const
    self.volatile = volatile
    self.restrict = restrict
    self.c_spelling = self.GetCSpelling()
    self.js_spelling = self.GetJsSpelling()
    self.js_spelling_with_comma = self.GetJsSpelling(True)
    self.mangled = self.GetMangled()

  @staticmethod
  def FromCindexType(cindex_type):
    return Qualifiers(cindex_type.is_const_qualified(),
                      cindex_type.is_volatile_qualified(),
                      cindex_type.is_restrict_qualified())

  def GetCSpelling(self):
    s = ''
    if self.const:    s += 'const '
    if self.volatile: s += 'volatile '
    if self.restrict: s += 'restrict '
    return s

  def GetJsSpelling(self, with_comma=False):
    if not self.__nonzero__():
      return ''

    l = []
    if self.const:    l.append('type.CONST')
    if self.volatile: l.append('type.VOLATILE')
    if self.restrict: l.append('type.RESTRICT')

    if with_comma:
      return ', ' + '|'.join(l)
    return '|'.join(l)

  def GetMangled(self):
    ret = ''
    if self.restrict: ret += 'r'
    if self.volatile: ret += 'v'
    if self.const:    ret += 'K'
    return ret

  def __eq__(self, other):
    if not isinstance(other, Qualifiers):
      return False
    return (self.const == other.const and
            self.volatile == other.volatile and
            self.restrict == other.restrict)

  def __ne__(self, other):
    return not self.__eq__(other)

  def __hash__(self):
    return hash((self.const, self.volatile, self.restrict))

  def __nonzero__(self):
    return self.const or self.volatile or self.restrict

  def __str__(self):
    return self.c_spelling


class Type(object):
  def __init__(self, cindex_type):
    self.is_primitive = False
    self.qualifiers = Qualifiers.FromCindexType(cindex_type)
    self.kind = TypeKind.FromValue(cindex_type.kind.value)
    self.kind_name = self.kind.kind_name
    self.c_name = self.kind.c_name

    # Keep only as a backdoor for getting canonical type.
    # TODO(binji): remove this
    self._cindex_type = cindex_type

  def _PostInit(self, cindex_type, memo):
    return self

  global_memo = CindexTypeMemo()

  @staticmethod
  def FromCindexType(cindex_type, memo=None):
    had_memo = memo is not None
    if not memo:
      memo = Type.global_memo

    logging.debug('Processing type: %s %s' % (cindex_type.kind, cindex_type.spelling))
    if cindex_type.kind.value == TypeKind.VOID:
      ret = VoidType._DoFromCindexType(cindex_type, memo)
    elif TypeKind.BOOL <= cindex_type.kind.value <= TypeKind.LONGDOUBLE:
      ret = NumericType._DoFromCindexType(cindex_type, memo)
    elif cindex_type.kind.value == TypeKind.POINTER:
      ret = PointerType._DoFromCindexType(cindex_type, memo)
    elif cindex_type.kind.value == TypeKind.RECORD:
      ret = RecordType._DoFromCindexType(cindex_type, memo)
    elif cindex_type.kind.value == TypeKind.ENUM:
      ret = EnumType._DoFromCindexType(cindex_type, memo)
    elif cindex_type.kind.value == TypeKind.TYPEDEF:
      ret = TypedefType._DoFromCindexType(cindex_type, memo)
    elif cindex_type.kind.value == TypeKind.FUNCTIONNOPROTO:
      ret = FunctionNoprotoType._DoFromCindexType(cindex_type, memo)
    elif cindex_type.kind.value == TypeKind.FUNCTIONPROTO:
      ret = FunctionType._DoFromCindexType(cindex_type, memo)
    elif cindex_type.kind.value == TypeKind.CONSTANTARRAY:
      ret = ConstantArrayType._DoFromCindexType(cindex_type, memo)
    elif cindex_type.kind.value == TypeKind.INCOMPLETEARRAY:
      ret = IncompleteArrayType._DoFromCindexType(cindex_type, memo)
    elif cindex_type.kind == CindexTypeKind.UNEXPOSED:
      ret = Type.FromCindexType(cindex_type.get_canonical(), memo)
    else:
      raise gen.Error('Unsupported type kind: %s' % cindex_type.kind)

    return ret

  @classmethod
  def _DoFromCindexType(cls, cindex_type, memo):
    if cindex_type in memo:
      logging.debug('  %s in memo, skipping.' % cindex_type.spelling)
      return memo[cindex_type]

    ret = memo.Set(cindex_type, cls(cindex_type))
    return ret._PostInit(cindex_type, memo)

  SPELLING_PRECEDENCE = {
    TypeKind.POINTER: 1,
    TypeKind.CONSTANTARRAY: 2,
    TypeKind.INCOMPLETEARRAY: 3,
    TypeKind.FUNCTIONPROTO: 3,
    TypeKind.FUNCTIONNOPROTO: 3,
  }

  def GetCSpelling(self, name=None, last_kind=None):
    spelling = str(self.qualifiers)

    if self.is_primitive:
      spelling += self.c_name
      if name:
        spelling += ' ' + name
      return spelling

    name = name or ''
    prec = Type.SPELLING_PRECEDENCE.get(self.kind)
    last_prec = Type.SPELLING_PRECEDENCE.get(last_kind)

    if prec and last_prec and prec > last_prec:
      name = '(' + name + ')'

    if self.kind == TypeKind.TYPEDEF:
      spelling += self.name
      if name:
        spelling += ' ' + name
    elif self.kind == TypeKind.POINTER:
      name = '*' + spelling + name
      spelling = self.pointee.GetCSpelling(name, self.kind)
    elif self.kind == TypeKind.ENUM:
      if not self.is_anonymous:
        spelling += 'enum '
      spelling += self.c_tag
      if name:
        spelling += ' ' + name
    elif self.kind == TypeKind.RECORD:
      if not self.is_anonymous:
        if self.is_union:
          spelling += 'union '
        else:
          spelling += 'struct '
      spelling += self.c_tag
      if name:
        spelling += ' ' + name
    elif self.kind == TypeKind.CONSTANTARRAY:
      name += '[%d]' % self.array_size
      spelling = self.element_type.GetCSpelling(name, self.kind)
    elif self.kind == TypeKind.INCOMPLETEARRAY:
      name += '[]'
      self.element_type.GetCSpelling(name, self.kind)
    elif self.kind == TypeKind.FUNCTIONPROTO:
      name += '('
      if len(self.arg_types) > 0:
        args_spelling = [arg.GetCSpelling() for arg in self.arg_types]
        if self.is_variadic:
          args_spelling.append('...')
        name += ', '.join(args_spelling)
      else:
        name += 'void'
      name += ')'
      spelling = self.result_type.GetCSpelling(name, self.kind)
    elif self.kind == TypeKind.FUNCTIONNOPROTO:
      name += '()'
      spelling = self.result_type.GetCSpelling(name, self.kind)
    else:
      raise gen.Error('Unsupported type kind: %s' % self.kind_name)

    return spelling

  def __eq__(self, other):
    if not isinstance(other, Type):
      return False
    if self.kind != other.kind:
      return False
    if self.qualifiers != other.qualifiers:
      return False
    return True

  def __ne__(self, other):
    return not (self == other)

  def __str__(self):
    return self.c_spelling

  def __hash__(self):
    return hash((self.kind, self.qualifiers))

  def Unqualified(self):
    new = copy.copy(self)
    new.qualifiers = Qualifiers(False, False, False)
    return new

  def GetMangled(self):
    return self.qualifiers.mangled

  def VisitTypes(self, visitor):
    if visitor.EnterType(self):
      visitor.ExitType(self)

  @property
  def c_spelling(self):
    return self.GetCSpelling()

  @property
  def js_spelling(self):
    return self.GetJsSpelling()

  @property
  def mangled(self):
    return self.GetMangled()

  @property
  def canonical(self):
    return Type.FromCindexType(self._cindex_type.get_canonical())


class PrimitiveType(Type):
  def __init__(self, cindex_type):
    Type.__init__(self, cindex_type)
    self.is_primitive = True

  def GetJsSpelling(self):
    js_name = self.kind.js_name
    if self.qualifiers:
      return 'type.%s.$qualify(%s)' % (js_name, self.qualifiers.js_spelling)
    return 'type.%s' % js_name

  def GetMangled(self):
    return '%s%s' % (Type.GetMangled(self), MANGLED_PRIMITIVE[self.kind])


class VoidType(PrimitiveType):
  def __init__(self, cindex_type):
    PrimitiveType.__init__(self, cindex_type)


class NumericType(PrimitiveType):
  def __init__(self, cindex_type):
    PrimitiveType.__init__(self, cindex_type)


class PointerType(Type):
  def __init__(self, cindex_type):
    Type.__init__(self, cindex_type)
    self.pointee = None

  def _PostInit(self, cindex_type, memo):
    self.pointee = Type.FromCindexType(cindex_type.get_pointee(), memo)
    Type._PostInit(self, cindex_type, memo)
    return self

  def __eq__(self, other):
    if not Type.__eq__(self, other):
      return False
    return self.pointee == other.pointee

  def __hash__(self):
    return hash((Type.__hash__(self), self.pointee))

  def GetJsSpelling(self):
    return 'type.Pointer(%s%s)' % (self.pointee.js_spelling,
                                   self.qualifiers.js_spelling_with_comma)

  def GetMangled(self):
    return '%sP%s' % (Type.GetMangled(self), self.pointee.mangled)

  def VisitTypes(self, visitor):
    if visitor.EnterType(self):
      self.pointee.VisitTypes(visitor)
      visitor.ExitType(self)


class TagType(Type):
  def __init__(self, cindex_type):
    Type.__init__(self, cindex_type)

    # Don't use this. It is only used for uniquely determining this type.
    self._spelling = cindex_type.spelling
    self.is_anonymous = cindex_type.get_declaration().spelling == ''

    unqualified_spelling = _BaseCindexSpelling(self._spelling)

    if self.is_anonymous:
      self.js_tag = _GetAnonymousName(self.kind, unqualified_spelling)
      self.c_tag = unqualified_spelling
    else:
      self.js_tag = _BaseCindexSpelling(unqualified_spelling)
      self.c_tag = self.js_tag

  def __eq__(self, other):
    if not Type.__eq__(self, other):
      return False
    return self._spelling == other._spelling

  def __hash__(self):
    return hash((Type.__hash__(self), self._spelling))

  def GetJsSpelling(self):
    if self.qualifiers:
      return 'tags.%s.$qualify(%s)' % (self.js_tag, self.qualifiers.js_spelling)
    return 'tags.%s' % self.js_tag

  def GetMangled(self):
    return Type.GetMangled(self) + _MangleName(self.js_tag)


RecordField = collections.namedtuple(
    'RecordField',
    ['name', 'type', 'offset'])


class RecordType(TagType):
  def __init__(self, cindex_type):
    TagType.__init__(self, cindex_type)
    self.size = cindex_type.get_size()
    self.fields = []
    self.is_union = cindex_type.get_declaration().kind == CursorKind.UNION_DECL
    # Nested records that are named and don't have an associated field. They
    # will not be automatically embedded, so they are actually considered
    # top-level definitions.
    self.nested_named_records = []

  def _PostInit(self, cindex_type, memo):
    anon_records = set()

    for field in cindex_type.get_declaration().get_children():
      if field.kind == CursorKind.FIELD_DECL:
        field_name = field.spelling
        field_type = Type.FromCindexType(field.type, memo)
        field_offset = cindex_type.get_offset(field.spelling) / 8
        self.fields.append(RecordField(field_name, field_type, field_offset))
        if field_type.kind == TypeKind.RECORD and field_type.is_anonymous:
          anon_records.add(field_type)

    for field in cindex_type.get_declaration().get_children():
      if field.kind not in (CursorKind.STRUCT_DECL, CursorKind.UNION_DECL):
        continue

      nested = Type.FromCindexType(field.type, memo)
      if field.spelling != '':
        self.nested_named_records.append(nested)
        continue

      if nested in anon_records:
        continue

      # Unnamed nested struct, embed the fields directly.
      for nested_field in nested.fields:
        nested_field_name = nested_field.name
        nested_field_type = nested_field.type
        nested_field_offset = cindex_type.get_offset(nested_field_name) / 8
        assert nested_field_offset >= 0
        self.fields.append(RecordField(
            nested_field_name, nested_field_type, nested_field_offset))

    TagType._PostInit(self, cindex_type, memo)
    return self

  def VisitTypes(self, visitor):
    if visitor.EnterType(self):
      for field in self.fields:
        field.type.VisitTypes(visitor)
      for nested in self.nested_named_records:
        nested.VisitTypes(visitor)
      visitor.ExitType(self)


class EnumType(TagType):
  def __init__(self, cindex_type):
    TagType.__init__(self, cindex_type)


class TypedefType(Type):
  def __init__(self, cindex_type):
    Type.__init__(self, cindex_type)
    self.alias_type = None
    self.name = _BaseCindexSpelling(cindex_type.spelling)

  def _PostInit(self, cindex_type, memo):
    underlying_type = cindex_type.get_declaration().underlying_typedef_type
    self.alias_type = Type.FromCindexType(underlying_type, memo)
    Type._PostInit(self, cindex_type, memo)
    return self

  def __eq__(self, other):
    if not Type.__eq__(self, other):
      return False
    return (self.name == other.name and
            self.alias_type == other.alias_type)

  def __hash__(self):
    return hash((Type.__hash__(self), self.name, self.alias_type))

  def GetJsSpelling(self):
    if self.qualifiers:
      return 'types.%s.$qualify(%s)' % (self.name, self.qualifiers.js_spelling)
    return 'types.%s' % self.name

  def GetMangled(self):
    return Type.GetMangled(self) + _MangleName(self.name)

  def VisitTypes(self, visitor):
    if visitor.EnterType(self):
      self.alias_type.VisitTypes(visitor)
      visitor.ExitType(self)


class FunctionNoprotoType(Type):
  def __init__(self, cindex_type):
    Type.__init__(self, cindex_type)
    self.result_type = None

  def _PostInit(self, cindex_type, memo):
    self.result_type = Type.FromCindexType(cindex_type.get_result(), memo)
    Type._PostInit(self, cindex_type, memo)
    return self

  def __eq__(self, other):
    if not Type.__eq__(self, other):
      return False
    return self.result_type == other.result_type

  def __hash__(self):
    return hash((Type.__hash__(self), self.result_type))

  def GetJsSpelling(self):
    return 'type.FunctionNoproto(%s)' % self.result_type.js_spelling

  def GetMangled(self):
    return 'F%sE' % self.result_type.mangled

  def VisitTypes(self, visitor):
    if visitor.EnterType(self):
      self.result_type.VisitTypes(visitor)
      visitor.ExitType(self)


class FunctionType(Type):
  def __init__(self, cindex_type):
    Type.__init__(self, cindex_type)
    self.result_type = None
    self.arg_types = []
    self.is_variadic = cindex_type.is_function_variadic()

  def _PostInit(self, cindex_type, memo):
    self.result_type = Type.FromCindexType(cindex_type.get_result(), memo)
    self.arg_types = [Type.FromCindexType(arg_type, memo)
                      for arg_type in cindex_type.argument_types()]
    Type._PostInit(self, cindex_type, memo)
    return self

  def __eq__(self, other):
    if not Type.__eq__(self, other):
      return False
    return (self.result_type == other.result_type and
            len(self.arg_types) == len(other.arg_types) and
            all(x == y for x, y in zip(self.arg_types, other.arg_types)) and
            self.is_variadic == other.is_variadic)

  def __hash__(self):
    return hash((Type.__hash__(self),
                 self.result_type,
                 tuple(self.arg_types),
                 self.is_variadic))

  def GetJsSpelling(self):
    s = 'type.Function(%s, [%s]' % (
        self.result_type.js_spelling,
        ', '.join(a.js_spelling for a in self.arg_types))
    if self.is_variadic:
      return s + ', type.VARIADIC)'
    else:
      return s + ')'

  def GetMangled(self):
    s = 'F'
    s += self.result_type.mangled
    if self.arg_types:
      s += ''.join(a.mangled for a in self.arg_types)
    else:
      s += 'v'
    if self.is_variadic:
      s += 'z'
    s += 'E'
    return s

  def VisitTypes(self, visitor):
    if visitor.EnterType(self):
      self.result_type.VisitTypes(visitor)
      for arg_type in self.arg_types:
        arg_type.VisitTypes(visitor)
      visitor.ExitType(self)


class ConstantArrayType(Type):
  def __init__(self, cindex_type):
    Type.__init__(self, cindex_type)
    self.element_type = None
    self.array_size = cindex_type.get_array_size()

  def _PostInit(self, cindex_type, memo):
    element_type = cindex_type.get_array_element_type()
    self.element_type = Type.FromCindexType(element_type, memo)
    Type._PostInit(self, cindex_type, memo)
    return self

  def __eq__(self, other):
    if not Type.__eq__(self, other):
      return False
    return (self.element_type == other.element_type and
            self.array_size == other.array_size)

  def __hash__(self):
    return hash((Type.__hash__(self), self.element_type, self.array_size))

  def GetJsSpelling(self):
    return 'type.Array(%s, %d%s)' % (self.element_type.js_spelling,
                                      self.array_size,
                                      self.qualifiers.js_spelling_with_comma)

  def GetMangled(self):
    return '%sA%d_%s' % (Type.GetMangled(self),
                         self.array_size, self.element_type.mangled)

  def VisitTypes(self, visitor):
    if visitor.EnterType(self):
      self.element_type.VisitTypes(visitor)
      visitor.ExitType(self)


class IncompleteArrayType(Type):
  def __init__(self, cindex_type):
    Type.__init__(self, cindex_type)
    self.element_type = None

  def _PostInit(self, cindex_type, memo):
    element_type = cindex_type.get_array_element_type()
    self.element_type = Type.FromCindexType(element_type, memo)
    Type._PostInit(self, cindex_type, memo)
    return self

  def __eq__(self, other):
    if not Type.__eq__(self, other):
      return False
    return self.element_type == other.element_type

  def __hash__(self):
    return hash((Type.__hash__(self), self.element_type))

  def GetJsSpelling(self):
    return 'type.IncompleteArray(%s, %s)' % (
        self.element_type.js_spelling, self.qualifiers.js_spelling_with_comma)

  def GetMangled(self):
    return '%sP%s' % (Type.GetMangled(self), self.element_type.mangled)

  def VisitTypes(self, visitor):
    if visitor.EnterType(self):
      self.element_type.VisitTypes(visitor)
      visitor.ExitType(self)


class Cursor(object):
  def __eq__(self, other):
    if not isinstance(other, self.__class__):
      return False
    return self.spelling == other.spelling

  def __ne__(self, other):
    return not self.__eq__(other)

  def __hash__(self):
    return hash(self.spelling)


class FunctionDecl(Cursor):
  def __init__(self, cindex_cursor):
    Cursor.__init__(self)
    self.type = Type.FromCindexType(cindex_cursor.type)
    self.spelling = cindex_cursor.spelling
    self.displayname = cindex_cursor.displayname
    self.fn_id = None

  @staticmethod
  def FromCindexCursor(cindex_cursor):
    return FunctionDecl(cindex_cursor)

  def VisitTypes(self, visitor):
    self.type.VisitTypes(visitor)

class EnumDecl(Cursor):
  def __init__(self, cindex_cursor):
    Cursor.__init__(self)
    self.type = Type.FromCindexType(cindex_cursor.type)
    # Use js_tag for spelling; this ensures that anonymous enums get a unique
    # name.
    self.spelling = self.type.js_tag
    self.constants = []
    for child in cindex_cursor.get_children():
      if child.kind != CursorKind.ENUM_CONSTANT_DECL:
        continue
      self.constants.append((child.spelling, child.enum_value))

  @staticmethod
  def FromCindexCursor(cindex_cursor):
    return EnumDecl(cindex_cursor)


def Iter(cindex_cursor):
  for child in cindex_cursor.get_children():
    if child.kind == CursorKind.FUNCTION_DECL:
      yield child
    elif child.kind == CursorKind.ENUM_DECL:
      yield child
    else:
      for f in Iter(child):
        yield f
